/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_filesystem_impl.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_utils.h"
#include "components/prefs/pref_service.h"

namespace adblock {

using AdblockPlus::IFileSystem;

namespace {

void NotifyFileRead(const IFileSystem::ReadCallback& done_callback,
                    const std::string& data) {
  IFileSystem::IOBuffer buffer;
  std::copy(data.begin(), data.end(), std::back_inserter(buffer));
  done_callback(std::move(buffer));
}

void NotifyFileStated(const IFileSystem::StatCallback& callback,
                      IFileSystem::StatResult result) {
  callback(result, {});
}

void NotifyOperationEnded(const IFileSystem::Callback& callback, bool success) {
  callback(success ? "" : "Error");
}
bool RemoveFileInBackground(const base::FilePath& path) {
  return base::DeleteFile(path);
}

bool MoveFileInBackground(const base::FilePath& from,
                          const base::FilePath& to) {
  return base::Move(from, to);
}

bool WriteFileInBackground(const base::FilePath& path,
                           const IFileSystem::IOBuffer& data) {
  return base::WriteFile(path, reinterpret_cast<const char*>(data.data()),
                         data.size());
}

IFileSystem::StatResult StatFileInBackground(const base::FilePath& path) {
  IFileSystem::StatResult result;
  base::File::Info info;
  if (GetFileInfo(path, &info)) {
    result.lastModified = info.last_modified.ToInternalValue();
  }
  result.exists = base::PathExists(path);
  return result;
}

std::string ReadFileInBackground(const base::FilePath& path) {
  std::string content;
  base::ReadFileToString(path, &content);
  return content;
}

bool IsMigrationAttemptRequired(PrefService* pref_service) {
  return pref_service->GetBoolean(
      prefs::kAdblockFileSystemMigrationAttemptRequired);
}

void SetMigrationAttemptRequired(PrefService* pref_service, bool required) {
  pref_service->SetBoolean(prefs::kAdblockFileSystemMigrationAttemptRequired,
                           required);
}

void DoMigrationInBackground(const base::FilePath& storage_dir) {
  std::vector<base::FilePath> files = {
      base::FilePath(FILE_PATH_LITERAL("patterns.ini")),
      base::FilePath(FILE_PATH_LITERAL("prefs.json"))};

  base::FilePath old_dir;
  base::PathService::Get(base::DIR_HOME, &old_dir);
  const base::FilePath& new_dir = storage_dir;

  for (auto& file : files) {
    base::FilePath old_file = old_dir.Append(file);

    if (base::PathExists(old_file)) {
      base::FilePath new_file = new_dir.Append(file);
      if (!base::PathExists(new_file)) {
        LOG(INFO) << "[ABP] Migrating " << old_file << " to " << new_file;
        base::Move(old_file, new_file);
      }
    }
  }
}

}  // namespace

AdblockFileSystemImpl::AdblockFileSystemImpl(
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
    const base::FilePath& storage_dir)
    : abp_runner_(abp_runner), storage_dir_(storage_dir) {}

AdblockFileSystemImpl::~AdblockFileSystemImpl() = default;

void AdblockFileSystemImpl::Read(
    const std::string& path,
    const IFileSystem::ReadCallback& done_callback,
    const IFileSystem::Callback& error_callback) const {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  VLOG(1) << "[ABP] Read " << path;

  base::FilePath destination = storage_dir_.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&ReadFileInBackground, destination),
      base::BindOnce(&AdblockFileSystemImpl::OnFileRead,
                     weak_ptr_factory_.GetWeakPtr(), path, done_callback,
                     error_callback));
}

void AdblockFileSystemImpl::Write(const std::string& path,
                                  const IFileSystem::IOBuffer& data,
                                  const IFileSystem::Callback& callback) {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  VLOG(1) << "[ABP] Write " << path;
  base::FilePath destination = storage_dir_.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&WriteFileInBackground, destination, data),
      base::BindOnce(&AdblockFileSystemImpl::OnFileOperationEnded,
                     weak_ptr_factory_.GetWeakPtr(), "write", path, callback));
}

void AdblockFileSystemImpl::Move(const std::string& from,
                                 const std::string& to,
                                 const IFileSystem::Callback& callback) {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  VLOG(1) << "[ABP] Move from " << from << " to " << to;

  base::FilePath path_from = storage_dir_.AppendASCII(from);
  base::FilePath path_to = storage_dir_.AppendASCII(to);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&MoveFileInBackground, path_from, path_to),
      base::BindOnce(&AdblockFileSystemImpl::OnFileOperationEnded,
                     weak_ptr_factory_.GetWeakPtr(), "move", from, callback));
}

void AdblockFileSystemImpl::Remove(const std::string& path,
                                   const IFileSystem::Callback& callback) {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  VLOG(1) << "[ABP] Remove " << path;
  base::FilePath destination = storage_dir_.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&RemoveFileInBackground, destination),
      base::BindOnce(&AdblockFileSystemImpl::OnFileOperationEnded,
                     weak_ptr_factory_.GetWeakPtr(), "remove", path, callback));
}

void AdblockFileSystemImpl::Stat(
    const std::string& path,
    const IFileSystem::StatCallback& callback) const {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  VLOG(1) << "[ABP] Stat " << path;
  base::FilePath destination = storage_dir_.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&StatFileInBackground, destination),
      base::BindOnce(&AdblockFileSystemImpl::OnFileStated,
                     weak_ptr_factory_.GetWeakPtr(), path, callback));
}

// static
bool AdblockFileSystemImpl::AttemptMigrationIfRequired(
    PrefService* pref_service,
    const base::FilePath& storage_dir,
    base::OnceClosure reply_closure) {
  if (!IsMigrationAttemptRequired(pref_service)) {
    std::move(reply_closure).Run();
    return false;
  }

  LOG(INFO) << "[ABP] File system migration required";

  SetMigrationAttemptRequired(pref_service, false);

  base::ThreadPool::PostTaskAndReply(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&DoMigrationInBackground, storage_dir),
      std::move(reply_closure));

  return true;
}

void AdblockFileSystemImpl::OnFileOperationEnded(
    const std::string& type,
    const std::string& path,
    const IFileSystem::Callback& callback,
    bool success) const {
  VLOG(1) << "[ABP] End " << type << " for " << path;
  utils::BenchmarkOperation(
      "Notification after file " + type + " for " + path,
      base::BindOnce(&NotifyOperationEnded, callback, success));
}

void AdblockFileSystemImpl::OnFileStated(
    const std::string& path,
    const IFileSystem::StatCallback& callback,
    IFileSystem::StatResult result) const {
  VLOG(1) << "[ABP] Stated " << path;
  utils::BenchmarkOperation(
      "Notification after file stat for " + path,
      base::BindOnce(&NotifyFileStated, callback, result));
}

void AdblockFileSystemImpl::OnFileRead(
    const std::string& path,
    const IFileSystem::ReadCallback& done_callback,
    const IFileSystem::Callback& error_callback,
    const std::string& data) const {
  utils::BenchmarkOperation(
      "Notification after file read for " + path,
      base::BindOnce(&NotifyFileRead, done_callback, data));
}

}  // namespace adblock

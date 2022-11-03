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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_FILESYSTEM_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_FILESYSTEM_IMPL_H_

#include "third_party/libadblockplus/src/include/AdblockPlus/IFileSystem.h"

#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"

class PrefService;

namespace adblock {
/**
 * @brief Implements libadblockplus filesystem so that libabp can perform file
 * IO operations. Uses abp runner to perform these operations as it gets
 * created with MayBlock() trait and is allowed to access disk. Lives in abp
 * dedicated thread in the browser process.
 */
class AdblockFileSystemImpl final : public AdblockPlus::IFileSystem {
 public:
  explicit AdblockFileSystemImpl(
      scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
      const base::FilePath& storage_dir);
  ~AdblockFileSystemImpl() final;
  AdblockFileSystemImpl& operator=(const AdblockFileSystemImpl& other) = delete;
  AdblockFileSystemImpl(const AdblockFileSystemImpl& other) = delete;
  AdblockFileSystemImpl& operator=(AdblockFileSystemImpl&& other) = delete;
  AdblockFileSystemImpl(AdblockFileSystemImpl&& other) = delete;

  // AdblockPlus::IFileSystem overrides:
  void Read(
      const std::string& path,
      const AdblockPlus::IFileSystem::ReadCallback& done_callback,
      const AdblockPlus::IFileSystem::Callback& error_callback) const final;
  void Write(const std::string& path,
             const AdblockPlus::IFileSystem::IOBuffer& data,
             const AdblockPlus::IFileSystem::Callback& callback) final;
  void Move(const std::string& from,
            const std::string& to,
            const AdblockPlus::IFileSystem::Callback& callback) final;
  void Remove(const std::string& path,
              const AdblockPlus::IFileSystem::Callback& callback) final;
  void Stat(const std::string& path,
            const AdblockPlus::IFileSystem::StatCallback& callback) const final;

  static bool AttemptMigrationIfRequired(PrefService* pref_service,
                                         const base::FilePath& storage_dir,
                                         base::OnceClosure reply_closure);

 private:
  void OnFileRead(const std::string& path,
                  const IFileSystem::ReadCallback& done_callback,
                  const IFileSystem::Callback& error_callback,
                  const std::string& data) const;
  void OnFileStated(const std::string& path,
                    const IFileSystem::StatCallback& callback,
                    IFileSystem::StatResult result) const;
  void OnFileOperationEnded(const std::string& type,
                            const std::string& path,
                            const IFileSystem::Callback& callback,
                            bool success) const;

  scoped_refptr<base::SingleThreadTaskRunner> abp_runner_;
  base::FilePath storage_dir_;
  base::WeakPtrFactory<AdblockFileSystemImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_FILESYSTEM_IMPL_H_

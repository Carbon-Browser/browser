// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/fusebox/fusebox_server.h"

#include <sys/stat.h>

#include "base/callback.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "chrome/browser/ash/file_manager/fileapi_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "storage/browser/file_system/async_file_util.h"
#include "storage/browser/file_system/file_stream_reader.h"
#include "storage/browser/file_system/file_system_context.h"
#include "storage/browser/file_system/file_system_url.h"
#include "storage/common/file_system/file_system_util.h"

// This file provides the "business logic" half of the FuseBox server, coupled
// with the "D-Bus protocol logic" half in fusebox_service_provider.cc.

namespace fusebox {

namespace {

Server* g_server_instance = nullptr;

// ParseResult is the type returned by ParseFileSystemURL. It is a result type
// (see https://en.wikipedia.org/wiki/Result_type), being either an error or a
// value. In this case, the error type is a base::File::Error (a numeric code)
// and the value type is a pair of storage::FileSystemContext and
// storage::FileSystemURL.
struct ParseResult {
  explicit ParseResult(base::File::Error error_code_arg);
  ParseResult(scoped_refptr<storage::FileSystemContext> fs_context_arg,
              storage::FileSystemURL fs_url_arg);
  ~ParseResult();

  base::File::Error error_code;
  scoped_refptr<storage::FileSystemContext> fs_context;
  storage::FileSystemURL fs_url;

  // is_moniker_root is used for the special case where
  // fusebox::kMonikerFileSystemURL (also known as "dummy://moniker", with no
  // trailing slash) is passed to ReadDir. There is no FileSystemURL linked to
  // that fs_url_as_string (there is no base::Token in the string), so
  // ParseFileSystemURL (which returns a valid FileSystemURL on success) must
  // return an error. However, ReadDir on "dummy://moniker" should succeed (but
  // send an empty directory listing back over D-Bus).
  bool is_moniker_root = false;
};

ParseResult::ParseResult(base::File::Error error_code_arg)
    : error_code(error_code_arg) {}

ParseResult::ParseResult(
    scoped_refptr<storage::FileSystemContext> fs_context_arg,
    storage::FileSystemURL fs_url_arg)
    : error_code(base::File::Error::FILE_OK),
      fs_context(std::move(fs_context_arg)),
      fs_url(std::move(fs_url_arg)) {}

ParseResult::~ParseResult() = default;

// All of the Server methods' arguments start with a FileSystemURL (as a
// string). This function parses that first argument as well as finding the
// FileSystemContext we will need to serve those methods.
ParseResult ParseFileSystemURL(fusebox::MonikerMap& moniker_map,
                               std::string fs_url_as_string) {
  scoped_refptr<storage::FileSystemContext> fs_context =
      file_manager::util::GetFileManagerFileSystemContext(
          ProfileManager::GetActiveUserProfile());
  if (fs_url_as_string.empty()) {
    LOG(ERROR) << "No FileSystemURL";
    return ParseResult(base::File::Error::FILE_ERROR_INVALID_URL);
  } else if (!fs_context) {
    LOG(ERROR) << "No FileSystemContext";
    return ParseResult(base::File::Error::FILE_ERROR_FAILED);
  }

  storage::FileSystemURL fs_url;

  // Intercept any moniker names and replace them by their linked target.
  using ResultType = fusebox::MonikerMap::ExtractTokenResult::ResultType;
  auto extract_token_result =
      fusebox::MonikerMap::ExtractToken(fs_url_as_string);
  switch (extract_token_result.result_type) {
    case ResultType::OK:
      fs_url = moniker_map.Resolve(extract_token_result.token);
      if (!fs_url.is_valid()) {
        LOG(ERROR) << "Unresolvable Moniker";
        return ParseResult(base::File::Error::FILE_ERROR_NOT_FOUND);
      }
      break;
    case ResultType::NOT_A_MONIKER_FS_URL:
      fs_url = fs_context->CrackURLInFirstPartyContext(GURL(fs_url_as_string));
      if (!fs_url.is_valid()) {
        LOG(ERROR) << "Invalid FileSystemURL";
        return ParseResult(base::File::Error::FILE_ERROR_INVALID_URL);
      }
      break;
    case ResultType::MONIKER_FS_URL_BUT_ONLY_ROOT: {
      ParseResult result = ParseResult(base::File::Error::FILE_ERROR_NOT_FOUND);
      result.is_moniker_root = true;
      return result;
    }
    case ResultType::MONIKER_FS_URL_BUT_NOT_WELL_FORMED:
      return ParseResult(base::File::Error::FILE_ERROR_NOT_FOUND);
  }

  if (!fs_context->external_backend()->CanHandleType(fs_url.type())) {
    LOG(ERROR) << "Backend cannot handle "
               << storage::GetFileSystemTypeString(fs_url.type());
    return ParseResult(base::File::Error::FILE_ERROR_INVALID_URL);
  }
  return ParseResult(std::move(fs_context), std::move(fs_url));
}

// Some functions (marked with a §) below, take an fs_context argument that
// looks unused, but we need to keep the storage::FileSystemContext reference
// alive until the callbacks are run.

void RunReadCallbackFailure(Server::ReadCallback callback,
                            base::File::Error error_code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::move(callback).Run(base::File::Error::FILE_ERROR_INVALID_URL, nullptr,
                          0);
}

void RunReadCallbackTypical(
    Server::ReadCallback callback,
    scoped_refptr<storage::FileSystemContext> fs_context,  // See § above.
    std::unique_ptr<storage::FileStreamReader> fs_reader,
    scoped_refptr<net::IOBuffer> buffer,
    int length) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (length < 0) {
    std::move(callback).Run(storage::NetErrorToFileError(length), nullptr, 0);
  } else {
    std::move(callback).Run(base::File::Error::FILE_OK,
                            reinterpret_cast<uint8_t*>(buffer->data()), length);
  }

  auto task_runner = content::GetIOThreadTaskRunner({});
  task_runner->DeleteSoon(FROM_HERE, fs_reader.release());
  task_runner->ReleaseSoon(FROM_HERE, std::move(buffer));
}

void ReadOnIOThread(scoped_refptr<storage::FileSystemContext> fs_context,
                    storage::FileSystemURL fs_url,
                    int64_t offset,
                    int64_t length,
                    Server::ReadCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  std::unique_ptr<storage::FileStreamReader> fs_reader =
      fs_context->CreateFileStreamReader(fs_url, offset, length, base::Time());
  if (!fs_reader) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&RunReadCallbackFailure, std::move(callback),
                                  base::File::Error::FILE_ERROR_INVALID_URL));
    return;
  }

  scoped_refptr<net::IOBuffer> buffer =
      base::MakeRefCounted<net::IOBuffer>(length);

  // Save the pointer before we std::move fs_reader into a base::OnceCallback.
  // The std::move keeps the underlying storage::FileStreamReader alive while
  // any network I/O is pending. Without the std::move, the underlying
  // storage::FileStreamReader would get destroyed at the end of this function.
  auto* saved_fs_reader = fs_reader.get();

  auto pair = base::SplitOnceCallback(base::BindPostTask(
      content::GetUIThreadTaskRunner({}),
      base::BindOnce(&RunReadCallbackTypical, std::move(callback), fs_context,
                     std::move(fs_reader), buffer)));

  int result =
      saved_fs_reader->Read(buffer.get(), length, std::move(pair.first));
  if (result != net::ERR_IO_PENDING) {  // The read was synchronous.
    std::move(pair.second).Run(result);
  }
}

void RunReadDirCallback(
    Server::ReadDirCallback callback,
    scoped_refptr<storage::FileSystemContext> fs_context,  // See § above.
    uint64_t cookie,
    base::File::Error error_code,
    storage::AsyncFileUtil::EntryList entry_list,
    bool has_more) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  fusebox::DirEntryListProto protos;
  for (const auto& entry : entry_list) {
    auto* proto = protos.add_entries();
    proto->set_is_directory(entry.type ==
                            filesystem::mojom::FsFileType::DIRECTORY);
    proto->set_name(entry.name.value());
  }

  callback.Run(cookie, error_code, std::move(protos), has_more);
}

void RunStatCallback(
    Server::StatCallback callback,
    scoped_refptr<storage::FileSystemContext> fs_context,  // See § above.
    base::File::Error error_code,
    const base::File::Info& info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::move(callback).Run(error_code, info);
}

}  // namespace

// static
Server* Server::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return g_server_instance;
}

Server::Server() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!g_server_instance);
  g_server_instance = this;
}

Server::~Server() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(g_server_instance);
  g_server_instance = nullptr;
}

fusebox::Moniker Server::CreateMoniker(storage::FileSystemURL target) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return moniker_map_.CreateMoniker(target);
}

void Server::DestroyMoniker(fusebox::Moniker moniker) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  moniker_map_.DestroyMoniker(moniker);
}

void Server::Close(std::string fs_url_as_string, CloseCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto common = ParseFileSystemURL(moniker_map_, fs_url_as_string);
  if (common.error_code != base::File::Error::FILE_OK) {
    std::move(callback).Run(common.error_code);
    return;
  }

  // Fail with an invalid operation error for now. TODO(crbug.com/1249754)
  // implement MTP device writing.
  std::move(callback).Run(base::File::Error::FILE_ERROR_INVALID_OPERATION);
}

void Server::Open(std::string fs_url_as_string, OpenCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto common = ParseFileSystemURL(moniker_map_, fs_url_as_string);
  if (common.error_code != base::File::Error::FILE_OK) {
    std::move(callback).Run(common.error_code);
    return;
  }

  // Fail with an invalid operation error for now. TODO(crbug.com/1249754)
  // implement MTP device writing.
  std::move(callback).Run(base::File::Error::FILE_ERROR_INVALID_OPERATION);
}

void Server::Read(std::string fs_url_as_string,
                  int64_t offset,
                  int32_t length,
                  ReadCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto common = ParseFileSystemURL(moniker_map_, fs_url_as_string);
  if (common.error_code != base::File::Error::FILE_OK) {
    std::move(callback).Run(common.error_code, nullptr, 0);
    return;
  }

  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&ReadOnIOThread, common.fs_context, common.fs_url, offset,
                     static_cast<int64_t>(length), std::move(callback)));
}

void Server::ReadDir(std::string fs_url_as_string,
                     uint64_t cookie,
                     ReadDirCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto common = ParseFileSystemURL(moniker_map_, fs_url_as_string);
  if (common.is_moniker_root ||
      (common.error_code != base::File::Error::FILE_OK)) {
    constexpr bool has_more = false;
    callback.Run(cookie, common.error_code, fusebox::DirEntryListProto(),
                 has_more);
    return;
  }

  auto outer_callback =
      base::BindPostTask(base::SequencedTaskRunnerHandle::Get(),
                         base::BindRepeating(&RunReadDirCallback, callback,
                                             common.fs_context, cookie));

  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindRepeating(
          base::IgnoreResult(
              &storage::FileSystemOperationRunner::ReadDirectory),
          // Unretained is safe: common.fs_context owns its operation_runner.
          base::Unretained(common.fs_context->operation_runner()),
          common.fs_url, std::move(outer_callback)));
}

void Server::Stat(std::string fs_url_as_string, StatCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto common = ParseFileSystemURL(moniker_map_, fs_url_as_string);
  if (common.error_code != base::File::Error::FILE_OK) {
    std::move(callback).Run(common.error_code, base::File::Info());
    return;
  }

  constexpr auto metadata_fields =
      storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY |
      storage::FileSystemOperation::GET_METADATA_FIELD_SIZE |
      storage::FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED;

  auto outer_callback = base::BindPostTask(
      base::SequencedTaskRunnerHandle::Get(),
      base::BindOnce(&RunStatCallback, std::move(callback), common.fs_context));

  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&storage::FileSystemOperationRunner::GetMetadata),
          // Unretained is safe: common.fs_context owns its operation_runner.
          base::Unretained(common.fs_context->operation_runner()),
          common.fs_url, metadata_fields, std::move(outer_callback)));
}

}  // namespace fusebox

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/device_signals/core/common/common_types.h"

namespace device_signals {

ExecutableMetadata::ExecutableMetadata() = default;

ExecutableMetadata::ExecutableMetadata(const ExecutableMetadata&) = default;
ExecutableMetadata& ExecutableMetadata::operator=(const ExecutableMetadata&) =
    default;

ExecutableMetadata::~ExecutableMetadata() = default;

bool ExecutableMetadata::operator==(const ExecutableMetadata& other) const {
  return is_running == other.is_running &&
         public_key_sha256 == other.public_key_sha256 &&
         product_name == other.product_name && version == other.version;
}

FileSystemItem::FileSystemItem() = default;

FileSystemItem::FileSystemItem(const FileSystemItem&) = default;
FileSystemItem& FileSystemItem::operator=(const FileSystemItem&) = default;

FileSystemItem::~FileSystemItem() = default;

bool FileSystemItem::operator==(const FileSystemItem& other) const {
  return file_path == other.file_path && presence == other.presence &&
         sha256_hash == other.sha256_hash &&
         executable_metadata == other.executable_metadata;
}

GetFileSystemInfoOptions::GetFileSystemInfoOptions() = default;

GetFileSystemInfoOptions::GetFileSystemInfoOptions(
    const GetFileSystemInfoOptions&) = default;
GetFileSystemInfoOptions& GetFileSystemInfoOptions::operator=(
    const GetFileSystemInfoOptions&) = default;

GetFileSystemInfoOptions::~GetFileSystemInfoOptions() = default;

bool GetFileSystemInfoOptions::operator==(
    const GetFileSystemInfoOptions& other) const {
  return file_path == other.file_path &&
         compute_sha256 == other.compute_sha256 &&
         compute_is_executable == other.compute_is_executable;
}

}  // namespace device_signals

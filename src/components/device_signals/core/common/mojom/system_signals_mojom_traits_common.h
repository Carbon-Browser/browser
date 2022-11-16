// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_MOJOM_SYSTEM_SIGNALS_MOJOM_TRAITS_COMMON_H_
#define COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_MOJOM_SYSTEM_SIGNALS_MOJOM_TRAITS_COMMON_H_

#include <string>

#include "base/files/file_path.h"
#include "components/device_signals/core/common/common_types.h"
#include "components/device_signals/core/common/mojom/system_signals.mojom-shared.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace mojo {

template <>
struct EnumTraits<device_signals::mojom::PresenceValue,
                  device_signals::PresenceValue> {
  static device_signals::mojom::PresenceValue ToMojom(
      device_signals::PresenceValue input);
  static bool FromMojom(device_signals::mojom::PresenceValue input,
                        device_signals::PresenceValue* output);
};

template <>
struct StructTraits<device_signals::mojom::ExecutableMetadataDataView,
                    device_signals::ExecutableMetadata> {
  static bool is_executable(const device_signals::ExecutableMetadata& input) {
    return input.is_executable;
  }

  static bool is_running(const device_signals::ExecutableMetadata& input) {
    return input.is_running.has_value() && input.is_running.value();
  }

  static absl::optional<std::string> public_key_sha256(
      const device_signals::ExecutableMetadata& input) {
    return input.public_key_sha256;
  }

  static absl::optional<std::string> product_name(
      const device_signals::ExecutableMetadata& input) {
    return input.product_name;
  }

  static absl::optional<std::string> version(
      const device_signals::ExecutableMetadata& input) {
    return input.version;
  }

  static bool Read(device_signals::mojom::ExecutableMetadataDataView data,
                   device_signals::ExecutableMetadata* output);
};

template <>
struct StructTraits<device_signals::mojom::FileSystemItemDataView,
                    device_signals::FileSystemItem> {
  static const base::FilePath& file_path(
      const device_signals::FileSystemItem& input) {
    return input.file_path;
  }

  static device_signals::PresenceValue presence(
      const device_signals::FileSystemItem& input) {
    return input.presence;
  }

  static absl::optional<std::string> sha256_hash(
      const device_signals::FileSystemItem& input) {
    return input.sha256_hash;
  }

  static absl::optional<device_signals::ExecutableMetadata> executable_metadata(
      const device_signals::FileSystemItem& input) {
    return input.executable_metadata;
  }

  static bool Read(device_signals::mojom::FileSystemItemDataView input,
                   device_signals::FileSystemItem* output);
};

template <>
struct StructTraits<device_signals::mojom::FileSystemItemRequestDataView,
                    device_signals::GetFileSystemInfoOptions> {
  static const base::FilePath& file_path(
      const device_signals::GetFileSystemInfoOptions& input) {
    return input.file_path;
  }

  static bool compute_sha256(
      const device_signals::GetFileSystemInfoOptions& input) {
    return input.compute_sha256;
  }

  static bool compute_executable_metadata(
      const device_signals::GetFileSystemInfoOptions& input) {
    return input.compute_is_executable;
  }

  static bool Read(device_signals::mojom::FileSystemItemRequestDataView input,
                   device_signals::GetFileSystemInfoOptions* output);
};

}  // namespace mojo

#endif  // COMPONENTS_DEVICE_SIGNALS_CORE_COMMON_MOJOM_SYSTEM_SIGNALS_MOJOM_TRAITS_COMMON_H_

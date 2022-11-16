// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENTERPRISE_CONNECTORS_SERVICE_PROVIDER_CONFIG_H_
#define CHROME_BROWSER_ENTERPRISE_CONNECTORS_SERVICE_PROVIDER_CONFIG_H_

#include <array>
#include <map>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/values.h"

namespace enterprise_connectors {

// An interface used to determine if file extensions or mimetypes are supported
// for a specific tag. This interface is meant to be implemented by constexpr
// objects, so it has no virtual destructor.
class SupportedFiles {
 public:
  virtual bool MimeTypeSupported(const std::string& mime_type) const = 0;
  virtual bool FileExtensionSupported(const base::FilePath& path) const = 0;
};

struct SupportedTag {
  const char* name = nullptr;
  const char* display_name = nullptr;
  size_t max_file_size = -1;
  const SupportedFiles* const supported_files = nullptr;
};

struct AnalysisConfig {
  // Only 1 of `url` and `local_path` should be populated to differentiate
  // between cloud analysis providers and local analysis providers.
  const char* url = nullptr;
  const char* local_path = nullptr;

  const base::span<const SupportedTag> supported_tags;
  const bool user_specific = false;
};

struct ReportingConfig {
  const char* url = nullptr;
};

struct FileSystemConfig {
  const char* home = nullptr;
  const char* authorization_endpoint = nullptr;
  const char* token_endpoint = nullptr;
  size_t max_direct_size = -1;
  std::array<const char*, 0> scopes;
  std::array<const char*, 2> disable;
  const char* client_id = nullptr;
  const char* client_secret = nullptr;
};

struct ServiceProvider {
  const char* display_name;
  // The fields below are not a raw_ptr<...> because they are initialized with
  // a non-nullptr value in constexpr.
  RAW_PTR_EXCLUSION const AnalysisConfig* analysis = nullptr;
  RAW_PTR_EXCLUSION const ReportingConfig* reporting = nullptr;
  RAW_PTR_EXCLUSION const FileSystemConfig* file_system = nullptr;
};

using ServiceProviderConfig =
    base::fixed_flat_map<base::StringPiece, ServiceProvider, 3>;

// Returns the global service provider configuration, containing every service
// provider and each of their supported Connector configs.
const ServiceProviderConfig* GetServiceProviderConfig();

}  // namespace enterprise_connectors

#endif  // CHROME_BROWSER_ENTERPRISE_CONNECTORS_SERVICE_PROVIDER_CONFIG_H_

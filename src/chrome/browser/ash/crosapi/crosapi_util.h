// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_CROSAPI_CROSAPI_UTIL_H_
#define CHROME_BROWSER_ASH_CROSAPI_CROSAPI_UTIL_H_

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/files/platform_file.h"
#include "base/files/scoped_file.h"
#include "base/token.h"
#include "chrome/browser/ash/crosapi/browser_util.h"
#include "chromeos/crosapi/mojom/crosapi.mojom.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/cloud/component_cloud_policy_service.h"
#include "components/user_manager/user.h"
#include "url/gurl.h"

class Profile;

// These methods are used by ash-chrome.
namespace crosapi {
namespace browser_util {

// Checks for the given profile if the user is affiliated or belongs to the
// sign-in profile.
bool IsSigninProfileOrBelongsToAffiliatedUser(Profile* profile);

// Returns the UUID and version for all tracked interfaces. Exposed for testing.
const base::flat_map<base::Token, uint32_t>& GetInterfaceVersions();

// Returns the device settings needed for Lacros.
mojom::DeviceSettingsPtr GetDeviceSettings();

// Returns the CloudPolicyCore for the given user.
policy::CloudPolicyCore* GetCloudPolicyCoreForUser(
    const user_manager::User& user);

// Returns the ComponentCloudPolicyService for the given user.
policy::ComponentCloudPolicyService* GetComponentCloudPolicyServiceForUser(
    const user_manager::User& user);

// Returns the list of Ash capabilities to publish to Lacros.
base::span<const std::string_view> GetAshCapabilities();

}  // namespace browser_util
}  // namespace crosapi

#endif  // CHROME_BROWSER_ASH_CROSAPI_CROSAPI_UTIL_H_

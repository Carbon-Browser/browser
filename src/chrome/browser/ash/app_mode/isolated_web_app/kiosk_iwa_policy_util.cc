// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/app_mode/isolated_web_app/kiosk_iwa_policy_util.h"

#include <optional>
#include <utility>
#include <vector>

#include "ash/constants/ash_features.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_op.h"
#include "base/notreached.h"
#include "chrome/browser/ash/policy/core/device_local_account.h"
#include "chromeos/ash/components/settings/cros_settings.h"
#include "components/account_id/account_id.h"
#include "components/policy/core/common/device_local_account_type.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "components/web_package/signed_web_bundles/signed_web_bundle_id.h"
#include "url/gurl.h"

namespace {

bool IsIwaKioskSession() {
  return user_manager::UserManager::IsInitialized() &&
         user_manager::UserManager::Get()->IsLoggedInAsKioskIWA();
}

policy::DeviceLocalAccount GetCurrentDeviceLocalAccount() {
  const user_manager::User& current_user =
      CHECK_DEREF(user_manager::UserManager::Get()->GetPrimaryUser());
  const AccountId& account_id = current_user.GetAccountId();

  const std::vector<policy::DeviceLocalAccount> device_local_accounts =
      policy::GetDeviceLocalAccounts(ash::CrosSettings::Get());

  // The current device local account should exist and match the type as already
  // checked via UserManager.
  for (const auto& account : device_local_accounts) {
    if (AccountId::FromUserEmail(account.user_id) == account_id) {
      CHECK_EQ(account.type,
               policy::DeviceLocalAccountType::kKioskIsolatedWebApp);
      return account;
    }
  }

  NOTREACHED();
}

}  // namespace

namespace ash {

std::optional<KioskIwaPolicyData> GetCurrentKioskIwaPolicyData() {
  if (!ash::features::IsIsolatedWebAppKioskEnabled() || !IsIwaKioskSession()) {
    return std::nullopt;
  }

  auto current_kiosk_policy = GetCurrentDeviceLocalAccount();

  // Web bundle id and update manifest URL in the current IWA kiosk account
  // should be valid.
  auto current_web_bundle_id = web_package::SignedWebBundleId::Create(
      current_kiosk_policy.kiosk_iwa_info.web_bundle_id());
  CHECK(current_web_bundle_id.has_value());

  GURL current_update_manifest_url(
      current_kiosk_policy.kiosk_iwa_info.update_manifest_url());
  CHECK(current_update_manifest_url.is_valid());

  KioskIwaPolicyData result = {
      .web_bundle_id = current_web_bundle_id.value(),
      .update_manifest_url = std::move(current_update_manifest_url)};
  return result;
}

}  // namespace ash

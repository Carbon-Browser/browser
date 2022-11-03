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

#include "components/adblock/adblock_prefs.h"

#include "base/logging.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/allowed_connection_type.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace adblock {
namespace prefs {

// Whether to block ads
const char kEnableAdblock[] = "adblock.enable";

// Whether to allow acceptable ads or block them all
const char kEnableAcceptableAds[] = "adblock.aa_enable";

// List of domains ad blocking will be disabled for
const char kAdblockAllowedDomains[] = "adblock.allowed_domains";

// List of custom filters added explicitly by the user
const char kAdblockCustomFilters[] = "adblock.custom_filters";

// List of recommended subscriptions
const char kAdblockSubscriptions[] = "adblock.subscriptions";

// List of custom (user defined) subscriptions
const char kAdblockCustomSubscriptions[] = "adblock.custom_subscriptions";

// List of recommended subscriptions
const char kAdblockRecommendedSubscriptions[] =
    "adblock.recommended_subscriptions";

// Version for which the reccomended subscriptions were set
const char kAdblockRecommendedSubscriptionsVersion[] =
    "adblock.recommended_subscriptions_version";

// Connection type allowed to be used for updates
const char kAdblockAllowedConnectionType[] = "adblock.allowed_connection_type";

// Whether more options item is enabled in the UI
const char kAdblockMoreOptionsEnabled[] = "adblock.more_options";

// Whether an attemp to migrate preferences compatible with the old architecture
// (based on libabp-android) is required or not on startup
const char kAdblockPrefsMigrationAttemptRequired[] =
    "adblock.prefs_migration_attempt_required";

// Whether to do the file system migration
const char kAdblockFileSystemMigrationAttemptRequired[] =
    "adblock.file_system_migration_attempt_required";

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kEnableAdblock, true);
  registry->RegisterBooleanPref(kEnableAcceptableAds, true);
  registry->RegisterBooleanPref(kAdblockMoreOptionsEnabled, false);
  registry->RegisterBooleanPref(kAdblockPrefsMigrationAttemptRequired, true);
  registry->RegisterBooleanPref(kAdblockFileSystemMigrationAttemptRequired,
                                true);
  registry->RegisterListPref(kAdblockAllowedDomains, {});
  registry->RegisterListPref(kAdblockCustomFilters, {});
  registry->RegisterListPref(kAdblockSubscriptions, {});
  registry->RegisterListPref(kAdblockCustomSubscriptions, {});
  registry->RegisterListPref(kAdblockRecommendedSubscriptions, {});
  registry->RegisterStringPref(kAdblockRecommendedSubscriptionsVersion,
                               "0.0.0.0");
  registry->RegisterStringPref(
      kAdblockAllowedConnectionType,
      AllowedConnectionTypeToString(AllowedConnectionType::kWiFi));
  VLOG(3) << "[ABP] Registered prefs";
}

}  // namespace prefs
}  // namespace adblock

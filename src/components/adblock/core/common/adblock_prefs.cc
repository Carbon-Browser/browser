/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/core/common/adblock_prefs.h"

#include "base/logging.h"
#include "components/adblock/core/common/allowed_connection_type.h"
#include "components/prefs/pref_registry_simple.h"

namespace adblock::prefs {

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

// Connection type allowed to be used for updates
const char kAdblockAllowedConnectionType[] = "adblock.allowed_connection_type";

// Whether more options item is enabled in the UI
const char kAdblockMoreOptionsEnabled[] = "adblock.more_options";

// Whether a first-run installation of preloaded subscriptions and
// language-based recommended subscriptions is necessary.
const char kInstallFirstStartSubscriptions[] =
    "adblock.install_first_run_subscriptions";

// Dictionary mapping subscription filename to subscription content hash,
// stored in Tracked Preferences to ensure untrusted subscription files aren't
// added, or existing subscription files aren't modified.
const char kSubscriptionSignatures[] = "adblock.subscription_signatures";

// Stores the schema version used to create currently installed subscriptions.
// Allows discovering a need to re-install subscriptions when the schema
// version used by this browser build is newer.
const char kLastUsedSchemaVersion[] = "adblock.last_used_schema_version";

// Map of subscription URL into subscription metadata, containing ex. expiration
// time, download count etc. Used for driving the subscription update process
// and for setting query parameters in subscription download requests.
const char kSubscriptionMetadata[] = "adblock.subscription_metadata";

// Client-generated UUID4 that uniquely identifies the server response that
// sent kTelemetryLastPingTime. Sent along with other ping times to
// disambiguate between other clients who send ping requests the same day.
// Regenerated on every successful response.
const char kTelemetryLastPingTag[] =
    "adblock.telemetry.activeping.last_ping_tag";

// Server UTC time of last ping response, updated with every successful
// response. Shall not be compared to client time (even UTC). Sent by the
// telemetry server, stored as unparsed string (ex. "2022-02-08T09:30:00Z").
const char kTelemetryLastPingTime[] =
    "adblock.telemetry.activeping.last_ping_time";

// Previous last ping time, gets replaced by kTelemetryLastPingTime when a new
// successful ping response arrives. Sent in a ping request.
const char kTelemetryPreviousLastPingTime[] =
    "adblock.telemetry.activeping.previous_last_ping_time";

// Time of first recorded response for a telemetry ping request, sent along
// with future ping requests, to further disambiguate
// user-counting without being able to uniquely track a user.
const char kTelemetryFirstPingTime[] =
    "adblock.telemetry.activeping.first_ping_time";

// Client time, when to perform the next ping?
// Not sent, used locally to ensure we don't ping too often.
const char kTelemetryNextPingTime[] =
    "adblock.telemetry.activeping.next_ping_time";

void RegisterTelemetryPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kTelemetryLastPingTag, "");
  registry->RegisterStringPref(kTelemetryLastPingTime, "");
  registry->RegisterStringPref(kTelemetryPreviousLastPingTime, "");
  registry->RegisterStringPref(kTelemetryFirstPingTime, "");
  registry->RegisterTimePref(kTelemetryNextPingTime, base::Time());
}

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kEnableAdblock, true);
  registry->RegisterBooleanPref(kEnableAcceptableAds, true);
  registry->RegisterBooleanPref(kAdblockMoreOptionsEnabled, false);
  registry->RegisterListPref(kAdblockAllowedDomains, {});
  registry->RegisterListPref(kAdblockCustomFilters, {});
  registry->RegisterListPref(kAdblockSubscriptions, {});
  registry->RegisterListPref(kAdblockCustomSubscriptions, {});
  registry->RegisterStringPref(
      kAdblockAllowedConnectionType,
      AllowedConnectionTypeToString(AllowedConnectionType::kDefault));
  registry->RegisterBooleanPref(kInstallFirstStartSubscriptions, true);
  registry->RegisterDictionaryPref(kSubscriptionSignatures);
  registry->RegisterStringPref(kLastUsedSchemaVersion, "");
  registry->RegisterDictionaryPref(kSubscriptionMetadata);
  RegisterTelemetryPrefs(registry);

  VLOG(3) << "[eyeo] Registered prefs";
}

}  // namespace adblock::prefs

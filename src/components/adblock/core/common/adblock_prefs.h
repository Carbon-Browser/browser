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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_

class PrefRegistrySimple;

namespace adblock::prefs {

extern const char kEnableAdblock[];
extern const char kEnableAcceptableAds[];
extern const char kAdblockAllowedDomains[];
extern const char kAdblockCustomFilters[];
extern const char kAdblockSubscriptions[];
extern const char kAdblockCustomSubscriptions[];
extern const char kAdblockMoreOptionsEnabled[];
extern const char kAdblockAllowedConnectionType[];
extern const char kInstallFirstStartSubscriptions[];
extern const char kSubscriptionSignatures[];
extern const char kLastUsedSchemaVersion[];
extern const char kSubscriptionMetadata[];
extern const char kTelemetryLastPingTag[];
extern const char kTelemetryLastPingTime[];
extern const char kTelemetryPreviousLastPingTime[];
extern const char kTelemetryFirstPingTime[];
extern const char kTelemetryNextPingTime[];

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace adblock::prefs

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_PREFS_H_

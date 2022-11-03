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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_PREFS_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace adblock {
namespace prefs {

extern const char kEnableAdblock[];
extern const char kEnableAcceptableAds[];
extern const char kAdblockAllowedDomains[];
extern const char kAdblockCustomFilters[];
extern const char kAdblockSubscriptions[];
extern const char kAdblockCustomSubscriptions[];
extern const char kAdblockRecommendedSubscriptions[];
extern const char kAdblockRecommendedSubscriptionsVersion[];
extern const char kAdblockMoreOptionsEnabled[];
extern const char kAdblockAllowedConnectionType[];
extern const char kAdblockPrefsMigrationAttemptRequired[];
extern const char kAdblockFileSystemMigrationAttemptRequired[];

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace prefs
}  // namespace adblock

#endif

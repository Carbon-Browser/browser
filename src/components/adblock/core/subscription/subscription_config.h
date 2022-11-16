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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_CONFIG_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_CONFIG_H_

#include <vector>

#include "base/strings/string_piece_forward.h"
#include "url/gurl.h"

namespace adblock {

enum class SubscriptionUiVisibility { Visible, Invisible };

enum class SubscriptionFirstRunBehavior {
  // Download and install as soon as possible.
  Subscribe,
  // Download and install as soon as possible but only if the device's region
  // matches one of the |languages| defined in KnownSubscriptionInfo.
  SubscribeIfLocaleMatch,
  // Do not install automatically.
  Ignore
};

// Privileged filters include:
// - Snippet filters
// - Header filters
enum class SubscriptionPrivilegedFilterStatus { Allowed, Forbidden };

// Description of a subscription that's known to exist in the Internet.
// Can be used to populate a list of proposed or recommended subscriptions in
// UI.
struct KnownSubscriptionInfo {
  KnownSubscriptionInfo();
  KnownSubscriptionInfo(GURL url,
                        std::string title,
                        std::vector<std::string> languages,
                        SubscriptionUiVisibility ui_visibility,
                        SubscriptionFirstRunBehavior first_run,
                        SubscriptionPrivilegedFilterStatus privileged_status);
  ~KnownSubscriptionInfo();
  KnownSubscriptionInfo(const KnownSubscriptionInfo&);
  KnownSubscriptionInfo(KnownSubscriptionInfo&&);
  KnownSubscriptionInfo& operator=(const KnownSubscriptionInfo&);
  KnownSubscriptionInfo& operator=(KnownSubscriptionInfo&&);

  GURL url;
  std::string title;
  std::vector<std::string> languages;
  SubscriptionUiVisibility ui_visibility = SubscriptionUiVisibility::Visible;
  SubscriptionFirstRunBehavior first_run =
      SubscriptionFirstRunBehavior::Subscribe;
  SubscriptionPrivilegedFilterStatus privileged_status =
      SubscriptionPrivilegedFilterStatus::Forbidden;
};

// Describes an available preloaded subscription that will be used to provide
// some level of ad-filtering while a desired subscription is being downloaded
// from the Internet.
// Preloaded subscriptions are bundled with the browser and stored in the
// ResourceBundle. They might be out-of-date and have reduced quality, but they
// allow some level of ad-filtering immediately upon first start.
struct PreloadedSubscriptionInfo {
  // Wildcard-aware pattern that matches subscription URL. Examples:
  // "*easylist.txt" (will match URLs like
  // https://easylist-downloads.adblockplus.org/easylist.txt or
  // https://easylist-downloads.adblockplus.org/easylistchina+easylist.txt).
  // This preloaded subscription will be used as a substitute for a
  // subscription with a URL that matches |url_pattern|.
  base::StringPiece url_pattern;

  // Resource ID containing the binary flatbuffer data that defines this
  // preloaded subscription. Examples:
  // IDR_ADBLOCK_FLATBUFFER_EASYLIST
  int flatbuffer_resource_id;
};

namespace config {

// Returns the list of all known subscriptions. This list is static
// and may change with browser updates, but not with filter list updates.
// The list contains recommendations for all languages.
const std::vector<KnownSubscriptionInfo>& GetKnownSubscriptions();

// Returns whether a subscription from |url| is allowed to provide
// privileged filters.
bool AllowPrivilegedFilters(const GURL& url);

// Returns the configuration of available preloaded subscriptions. Used by
// PreloadedSubscriptionProvider.
const std::vector<PreloadedSubscriptionInfo>&
GetPreloadedSubscriptionConfiguration();

}  // namespace config

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_CONFIG_H_

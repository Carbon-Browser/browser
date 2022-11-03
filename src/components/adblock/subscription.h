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

#ifndef COMPONENTS_ADBLOCK_SUBSCRIPTION_H_
#define COMPONENTS_ADBLOCK_SUBSCRIPTION_H_

#include <string>
#include <vector>

#include "url/gurl.h"

namespace adblock {

struct Subscription {
  Subscription(const GURL& url_param,
               const std::string& title_param,
               const std::vector<std::string>& languages_param);
  Subscription(const Subscription&);
  Subscription(Subscription&&);
  Subscription& operator=(const Subscription&);
  Subscription& operator=(Subscription&&);
  ~Subscription();

  GURL url;
  std::string title;
  std::vector<std::string> languages;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_SUBSCRIPTION_H_

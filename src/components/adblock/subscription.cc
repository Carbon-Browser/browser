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

#include "components/adblock/subscription.h"

namespace adblock {

Subscription::Subscription(const GURL& url_param,
                           const std::string& title_param,
                           const std::vector<std::string>& languages_param)
    : url(url_param), title(title_param), languages(languages_param) {}

Subscription::Subscription(const Subscription&) = default;
Subscription::Subscription(Subscription&&) = default;
Subscription& Subscription::operator=(const Subscription&) = default;
Subscription& Subscription::operator=(Subscription&&) = default;
Subscription::~Subscription() = default;

}  // namespace adblock

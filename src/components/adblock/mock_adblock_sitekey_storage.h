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

#ifndef COMPONENTS_ADBLOCK_MOCK_ADBLOCK_SITEKEY_STORAGE_H_
#define COMPONENTS_ADBLOCK_MOCK_ADBLOCK_SITEKEY_STORAGE_H_

#include <string>
#include <vector>

#include "components/adblock/adblock_sitekey_storage.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockAdblockSitekeyStorage : public AdblockSitekeyStorage {
 public:
  MockAdblockSitekeyStorage();
  ~MockAdblockSitekeyStorage() override;
  MOCK_METHOD(
      void,
      ProcessResponseHeaders,
      (const GURL& request_url,
       const scoped_refptr<net::HttpResponseHeaders>& headers,
       const std::string& user_agent,
       mojom::AdblockInterface::ProcessResponseHeadersCallback callback),
      (override));

  MOCK_METHOD((absl::optional<std::pair<GURL, SiteKey>>),
              FindSiteKeyForAnyUrl,
              (const std::vector<GURL>& urls),
              (const, override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_MOCK_ADBLOCK_SITEKEY_STORAGE_H_

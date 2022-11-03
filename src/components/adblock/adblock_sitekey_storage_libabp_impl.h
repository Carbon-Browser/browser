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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_SITEKEY_STORAGE_LIBABP_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_SITEKEY_STORAGE_LIBABP_IMPL_H_

#include <string>

#include "absl/types/optional.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_sitekey_storage.h"

namespace adblock {

class AdblockSitekeyStorageLibabpImpl final : public AdblockSitekeyStorage {
 public:
  explicit AdblockSitekeyStorageLibabpImpl(
      scoped_refptr<AdblockPlatformEmbedder> embedder);
  ~AdblockSitekeyStorageLibabpImpl() final;

  void ProcessResponseHeaders(
      const GURL& request_url,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent,
      mojom::AdblockInterface::ProcessResponseHeadersCallback callback) final;

  absl::optional<std::pair<GURL, SiteKey>> FindSiteKeyForAnyUrl(
      const std::vector<GURL>& urls) const final;

 private:
  // Adds sitekey if url is valid and public_key_stripped is not empty
  void AddSiteKey(const GURL& url, const SiteKey& public_key_stripped);
  void ProcessSiteKey(
      const GURL& request_url,
      const std::string& site_key,
      const std::string& user_agent,
      mojom::AdblockInterface::ProcessResponseHeadersCallback callback);
  void OnSiteKeyProcessed(
      mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
      const GURL& request_url,
      const SiteKey& site_key);

  SEQUENCE_CHECKER(sequence_checker_);
  scoped_refptr<AdblockPlatformEmbedder> embedder_;
  std::map<GURL, SiteKey> url_sitekey_mapping_;
  base::WeakPtrFactory<AdblockSitekeyStorageLibabpImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_SITEKEY_STORAGE_LIBABP_IMPL_H_

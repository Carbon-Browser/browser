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

#include "components/adblock/adblock_sitekey_storage_libabp_impl.h"

#include "base/task/post_task.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/adblock_utils.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

namespace adblock {
namespace {

SiteKey ProcessSiteKeyImpl(scoped_refptr<AdblockPlatformEmbedder> embedder,
                           const GURL& request_url,
                           const SiteKey& site_key,
                           const std::string& user_agent) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  TRACE_EVENT1("adblockplus", "ProcessSiteKeyImpl", "url", request_url.spec());
  auto* filter_engine = embedder->GetFilterEngine();
  VLOG(1) << "[ABP] ProcessSiteKeyImpl start";

  GURL url = request_url.GetAsReferrer();

  size_t delimiter = site_key.value().find("_");
  if ((delimiter == std::string::npos) ||
      (delimiter >= (site_key.value().length() - 1))) {
    LOG(ERROR) << "[ABP] Wrong format of site key header value: "
               << site_key.value();
    return {};
  }

  std::string public_key = site_key.value().substr(0, delimiter);
  std::string public_key_stripped = public_key.substr(0, public_key.find("=="));
  std::string signature = site_key.value().substr(delimiter + 1);
  DVLOG(1) << "[ABP] Found site key header, public key: " << public_key
           << ", signature: " << signature;

  std::string url_string = url.spec();
  auto path_with_query = url.path();
  if (url.has_query()) {
    path_with_query += "?" + url.query();
  }
  DLOG(INFO) << "[ABP] Calling VerifySignature(publicKey, signature, uri, host,"
                " userAgent) with arguments: ("
             << public_key << ", " << signature << ", " << path_with_query
             << ", " << url.host() << ", " << user_agent << ")";
  if (!filter_engine->VerifySignature(public_key, signature, path_with_query,
                                      url.host(), user_agent)) {
    LOG(ERROR) << "[ABP] Sitekey verification failed";
    public_key_stripped.clear();
  }

  return SiteKey(public_key_stripped);
}
}  // namespace

AdblockSitekeyStorageLibabpImpl::AdblockSitekeyStorageLibabpImpl(
    scoped_refptr<AdblockPlatformEmbedder> embedder)
    : embedder_(std::move(embedder)) {}
AdblockSitekeyStorageLibabpImpl::~AdblockSitekeyStorageLibabpImpl() = default;

absl::optional<std::pair<GURL, SiteKey>>
AdblockSitekeyStorageLibabpImpl::FindSiteKeyForAnyUrl(
    const std::vector<GURL>& urls) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& url : urls) {
    auto elem = url_sitekey_mapping_.find(url);
    if (elem != url_sitekey_mapping_.cend()) {
      return {*elem};
    }
  }
  return {};
}

void AdblockSitekeyStorageLibabpImpl::AddSiteKey(
    const GURL& url,
    const SiteKey& public_key_stripped) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (url.is_empty() || !url.is_valid() || public_key_stripped.value().empty())
    return;

  url_sitekey_mapping_[url] = public_key_stripped;
}

void AdblockSitekeyStorageLibabpImpl::ProcessResponseHeaders(
    const GURL& request_url,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& user_agent,
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback) {
  if (user_agent.empty()) {
    LOG(WARNING) << "[ABP] No user agent info";
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        std::move(callback));
    return;
  }

  auto site_key = adblock::utils::GetSitekeyHeader(headers);
  if (site_key.empty()) {
    VLOG(1) << "[ABP] No site key header";
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        std::move(callback));
    return;
  }

  ProcessSiteKey(request_url, site_key, user_agent, std::move(callback));
}

void AdblockSitekeyStorageLibabpImpl::ProcessSiteKey(
    const GURL& request_url,
    const std::string& site_key,
    const std::string& user_agent,
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!site_key.empty());
  auto site_key_pair = FindSiteKeyForAnyUrl({request_url});
  if (site_key_pair.has_value() && site_key == site_key_pair->second.value()) {
    DVLOG(3) << "[ABP] Public key already stored for url: "
             << site_key_pair->first;
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        std::move(callback));
    return;
  }

  if (!embedder_->GetFilterEngine()) {
    VLOG(1) << "[ABP] ProcessSiteKey no engine yet";
    embedder_->RunAfterFilterEngineCreated(
        base::BindOnce(&AdblockSitekeyStorageLibabpImpl::ProcessSiteKey,
                       weak_ptr_factory_.GetWeakPtr(), request_url, site_key,
                       user_agent, std::move(callback)));
    return;
  }

  embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ProcessSiteKeyImpl, embedder_, request_url,
                     SiteKey(site_key), user_agent),
      base::BindOnce(&AdblockSitekeyStorageLibabpImpl::OnSiteKeyProcessed,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     request_url));
}

void AdblockSitekeyStorageLibabpImpl::OnSiteKeyProcessed(
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
    const GURL& request_url,
    const SiteKey& site_key) {
  if (!site_key.value().empty()) {
    AddSiteKey(request_url.GetAsReferrer(), site_key);
  }
  std::move(callback).Run();
}

}  // namespace adblock

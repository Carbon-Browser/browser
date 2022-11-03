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

#include "chrome/common/adblock/adblock_url_loader_throttle.h"

#include <memory>
#include <string>

#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {

AdblockURLLoaderThrottle::AdblockURLLoaderThrottle(
    adblock::mojom::AdblockInterface* adblock,
    int render_frame_id,
    base::RepeatingCallback<void(std::string*)> user_agent_getter)
    : adblock_(adblock),
      render_frame_id_(render_frame_id),
      user_agent_getter_(user_agent_getter) {}

AdblockURLLoaderThrottle::~AdblockURLLoaderThrottle() = default;

void AdblockURLLoaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  if (!request->url.SchemeIsHTTPOrHTTPS() && !request->url.SchemeIsWSOrWSS()) {
    VLOG(1) << "[ABP] Ignoring URL with unsupported scheme, allowing load.";
    return;
  }
  if (net::IsLocalhost(request->url)) {
    VLOG(1) << "[ABP] Ignoring localhost URL, allowing load.";
    return;
  }
  VLOG(1) << "[ABP] Checking filter match for: " << request->url << " ("
          << request->resource_type << ")";
  if (adblock_pending_remote_.is_valid()) {
    adblock_remote_.Bind(std::move(adblock_pending_remote_));
    adblock_ = adblock_remote_.get();
  }

  adblock_->CheckFilterMatch(
      request->url, request->resource_type, render_frame_id_,
      base::BindOnce(&AdblockURLLoaderThrottle::OnFilterMatchResult,
                     weak_ptr_factory_.GetWeakPtr(), request->url,
                     request->resource_type));
  *defer = true;
}

void AdblockURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  if (!response_url.SchemeIsHTTPOrHTTPS()) {
    VLOG(1) << "[ABP] Ignoring URL with unsupported scheme, not processing "
               "headers.";
    return;
  }
  if (net::IsLocalhost(response_url)) {
    VLOG(1) << "[ABP] Ignoring localhost URL, not processing headers.";
    return;
  }
  if (response_head && response_head->headers) {
    std::string us_string;
    user_agent_getter_.Run(&us_string);
    VLOG(1) << "[ABP] Sending headers for processing: " << response_url;
    adblock_->ProcessResponseHeaders(
        response_url, response_head->headers, us_string,
        base::BindOnce(&AdblockURLLoaderThrottle::OnProcessHeadersResult,
                       weak_ptr_factory_.GetWeakPtr()));
    *defer = true;
    return;
  }

  *defer = false;
}

void AdblockURLLoaderThrottle::DetachFromCurrentSequence() {
  adblock_->Clone(adblock_pending_remote_.InitWithNewPipeAndPassReceiver());
  adblock_ = nullptr;
}

void AdblockURLLoaderThrottle::OnFilterMatchResult(
    const GURL& url,
    int resource_type,
    adblock::mojom::FilterMatchResult result) {
  if (result == adblock::mojom::FilterMatchResult::kBlockRule) {
    DLOG(INFO) << "[ABP] Blocking: " << url << " (" << resource_type << ")";
    delegate_->CancelWithError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }

  DLOG(INFO) << "[ABP] Passing: " << url << " (" << resource_type << ")";
  delegate_->Resume();
}

void AdblockURLLoaderThrottle::OnProcessHeadersResult() {
  delegate_->Resume();
}

}  // namespace adblock

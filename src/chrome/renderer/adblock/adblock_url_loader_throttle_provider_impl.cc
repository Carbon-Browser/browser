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

#include "chrome/renderer/adblock/adblock_url_loader_throttle_provider_impl.h"

#include <memory>
#include <string>

#include "chrome/common/adblock/adblock_url_loader_throttle.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

AdblockURLLoaderThrottleProviderImpl::AdblockURLLoaderThrottleProviderImpl(
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
    blink::URLLoaderThrottleProviderType type,
    ChromeContentRendererClient* chrome_content_renderer_client,
    base::RepeatingCallback<void(std::string*)> user_agent_getter)
    : URLLoaderThrottleProviderImpl(broker,
                                    type,
                                    chrome_content_renderer_client),
      user_agent_getter_(user_agent_getter) {
  broker->GetInterface(adblock_remote_.InitWithNewPipeAndPassReceiver());
}

AdblockURLLoaderThrottleProviderImpl::~AdblockURLLoaderThrottleProviderImpl() =
    default;

AdblockURLLoaderThrottleProviderImpl::AdblockURLLoaderThrottleProviderImpl(
    const AdblockURLLoaderThrottleProviderImpl& other)
    : URLLoaderThrottleProviderImpl(other),
      user_agent_getter_(other.user_agent_getter_) {
  if (other.adblock_) {
    other.adblock_->Clone(adblock_remote_.InitWithNewPipeAndPassReceiver());
  }
}

std::unique_ptr<blink::URLLoaderThrottleProvider>
AdblockURLLoaderThrottleProviderImpl::Clone() {
  if (adblock_remote_)
    adblock_.Bind(std::move(adblock_remote_));
  return base::WrapUnique(new AdblockURLLoaderThrottleProviderImpl(*this));
}

blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>>
AdblockURLLoaderThrottleProviderImpl::CreateThrottles(
    int render_frame_id,
    const blink::WebURLRequest& request) {
  auto throttles =
      URLLoaderThrottleProviderImpl::CreateThrottles(render_frame_id, request);

  if (adblock_remote_)
    adblock_.Bind(std::move(adblock_remote_));

  throttles.emplace_back(std::make_unique<adblock::AdblockURLLoaderThrottle>(
      adblock_.get(), render_frame_id, user_agent_getter_));

  return throttles;
}

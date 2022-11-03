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

#ifndef CHROME_RENDERER_ADBLOCK_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define CHROME_RENDERER_ADBLOCK_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include "chrome/renderer/url_loader_throttle_provider_impl.h"

#include <memory>
#include <string>

#include "components/adblock/adblock.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/url_loader_throttle_provider.h"

class ChromeContentRendererClient;

class AdblockURLLoaderThrottleProviderImpl
    : public URLLoaderThrottleProviderImpl {
 public:
  AdblockURLLoaderThrottleProviderImpl(
      blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
      blink::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client,
      base::RepeatingCallback<void(std::string*)> user_agent_getter);

  ~AdblockURLLoaderThrottleProviderImpl() override;

  // blink::URLLoaderThrottleProvider implementation.
  std::unique_ptr<blink::URLLoaderThrottleProvider> Clone() override;
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURLRequest& request) override;

 private:
  // Use Clone() instead:
  AdblockURLLoaderThrottleProviderImpl(
      const AdblockURLLoaderThrottleProviderImpl& other);
  mojo::PendingRemote<adblock::mojom::AdblockInterface> adblock_remote_;
  mojo::Remote<adblock::mojom::AdblockInterface> adblock_;
  base::RepeatingCallback<void(std::string*)> user_agent_getter_;
};

#endif  // CHROME_RENDERER_ADBLOCK_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

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

#include "chrome/renderer/adblock/adblock_content_renderer_client.h"

#include <memory>

#include "base/bind.h"
#include "chrome/renderer/adblock/adblock_url_loader_throttle_provider_impl.h"
#include "third_party/blink/public/platform/platform.h"

AdblockContentRendererClient::AdblockContentRendererClient() = default;

AdblockContentRendererClient::~AdblockContentRendererClient() = default;

std::unique_ptr<blink::URLLoaderThrottleProvider>
AdblockContentRendererClient::CreateURLLoaderThrottleProvider(
    blink::URLLoaderThrottleProviderType provider_type) {
  return std::make_unique<AdblockURLLoaderThrottleProviderImpl>(
      browser_interface_broker_.get(), provider_type, this,
      base::BindRepeating(&AdblockContentRendererClient::GetUserAgent,
                          // This is a global in chrome_main_delegate.
                          base::Unretained(this)));
}

void AdblockContentRendererClient::DidSetUserAgent(
    const std::string& user_agent) {
  ChromeContentRendererClient::DidSetUserAgent(user_agent);
  user_agent_ = user_agent;
}

void AdblockContentRendererClient::RenderThreadStarted() {
  ChromeContentRendererClient::RenderThreadStarted();
  browser_interface_broker_ =
      blink::Platform::Current()->GetBrowserInterfaceBroker();
}

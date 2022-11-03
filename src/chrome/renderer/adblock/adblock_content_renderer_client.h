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

#ifndef CHROME_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_
#define CHROME_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_

#include "chrome/renderer/chrome_content_renderer_client.h"

#include <string>

namespace blink {
class URLLoaderThrottleProvider;
enum class URLLoaderThrottleProviderType;
}  // namespace blink

class AdblockContentRendererClient : public ChromeContentRendererClient {
 public:
  AdblockContentRendererClient();
  ~AdblockContentRendererClient() override;

  std::unique_ptr<blink::URLLoaderThrottleProvider>
  CreateURLLoaderThrottleProvider(
      blink::URLLoaderThrottleProviderType provider_type) override;
  void DidSetUserAgent(const std::string& user_agent) override;
  void RenderThreadStarted() override;

  void GetUserAgent(std::string* out) const { *out = user_agent_; }

 private:
  std::string user_agent_;
  scoped_refptr<blink::ThreadSafeBrowserInterfaceBrokerProxy>
      browser_interface_broker_;
};

#endif  // CHROME_RENDERER_ADBLOCK_CONTENT_RENDERER_CLIENT_H_

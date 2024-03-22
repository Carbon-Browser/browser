/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CHROME_BROWSER_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_
#define CHROME_BROWSER_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_

#include "build/buildflag.h"
#include "chrome/browser/chrome_content_browser_client.h"

namespace adblock {
enum class FilterMatchResult;
}  // namespace adblock

/**
 * @brief Intercepts network and UI events to inject ad-filtering.
 * Provides ad-filtering implementations of URLLoaderThrottles.
 * Binds a mojo connection between Renderer processes and the
 * Browser-process-based ResourceClassificationRunner.
 * Lives in browser process UI thread.
 */
class AdblockContentBrowserClient : public ChromeContentBrowserClient {
 public:
  AdblockContentBrowserClient();
  ~AdblockContentBrowserClient() override;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Enable ad filtering also for requests initiated by extensions.
  // This allows implementing extension-driven browser tests.
  // In production code, requests from extensions are not blocked.
  static void ForceAdblockProxyForTesting();
#endif

  bool WillInterceptWebSocket(content::RenderFrameHost* frame) override;
  void CreateWebSocket(
      content::RenderFrameHost* frame,
      WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client) override;

  bool WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      absl::optional<int64_t> navigation_id,
      ukm::SourceIdObj ukm_source_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks,
      bool* disable_secure_dns,
      network::mojom::URLLoaderFactoryOverridePtr* factory_override,
      scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner)
      override;

 private:
  void CreateWebSocketInternal(
      content::GlobalRenderFrameHostId render_frame_host_id,
      WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client);
  void OnWebSocketFilterCheckCompleted(
      content::GlobalRenderFrameHostId render_frame_host_id,
      ChromeContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      adblock::FilterMatchResult result);

  base::WeakPtrFactory<AdblockContentBrowserClient> weak_factory_{this};

#if BUILDFLAG(ENABLE_EXTENSIONS)
  static bool force_adblock_proxy_for_testing_;
#endif
};

#endif  // CHROME_BROWSER_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_

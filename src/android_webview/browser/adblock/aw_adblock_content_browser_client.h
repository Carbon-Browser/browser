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

#ifndef ANDROID_WEBVIEW_BROWSER_AW_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_
#define ANDROID_WEBVIEW_BROWSER_AW_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_

#include "android_webview/browser/aw_content_browser_client.h"
#include "build/buildflag.h"

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
class AwAdblockContentBrowserClient
    : public android_webview::AwContentBrowserClient {
 public:
  // |aw_feature_list_creator| should not be null.
  explicit AwAdblockContentBrowserClient(
      android_webview::AwFeatureListCreator* aw_feature_list_creator);

  AwAdblockContentBrowserClient(const AwAdblockContentBrowserClient&) = delete;
  AwAdblockContentBrowserClient& operator=(
      const AwAdblockContentBrowserClient&) = delete;

  ~AwAdblockContentBrowserClient() override;

  bool CanCreateWindow(content::RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const url::Origin& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;
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
  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override;

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
      android_webview::AwContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      adblock::FilterMatchResult result);

  base::WeakPtrFactory<AwAdblockContentBrowserClient> weak_factory_{this};
};

#endif  // ANDROID_WEBVIEW_BROWSER_AW_ADBLOCK_ADBLOCK_CONTENT_BROWSER_CLIENT_H_

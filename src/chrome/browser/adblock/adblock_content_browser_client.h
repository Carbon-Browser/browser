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

#ifndef ADBLOCK_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
#define ADBLOCK_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_

#include "chrome/browser/chrome_content_browser_client.h"

namespace adblock {
class AdblockMojoInterfaceImpl;
namespace mojom {
enum class FilterMatchResult;
}
}  // namespace adblock

/**
 * @brief Intercepts network and UI events to inject ad-filtering.
 * Provides ad-filtering implementations of URLLoaderThrottles.
 * Binds a mojo connection between Renderer processes and the
 * Browser-process-based AdblockRequestClassifier. Lives in browser process UI
 * thread.
 */
class AdblockContentBrowserClient : public ChromeContentBrowserClient {
 public:
  AdblockContentBrowserClient();
  ~AdblockContentBrowserClient() override;

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
  void ExposeInterfacesToRenderer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      content::RenderProcessHost* render_process_host) override;
  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override;

 private:
  void OnWebSocketFilterCheckCompleted(
      content::RenderFrameHost* frame,
      ChromeContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      adblock::mojom::FilterMatchResult result);

  base::WeakPtrFactory<AdblockContentBrowserClient> weak_factory_{this};
};

#endif  // ADBLOCK_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_

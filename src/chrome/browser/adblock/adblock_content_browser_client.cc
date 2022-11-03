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

#include "chrome/browser/adblock/adblock_content_browser_client.h"

#include "chrome/browser/adblock/adblock_mojo_interface_impl.h"
#include "chrome/browser/adblock/adblock_request_classifier_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/adblock/adblock_url_loader_throttle.h"
#include "components/adblock/adblock.mojom.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_request_classifier.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace {

void CreateAdblockInterfaceForRenderer(
    int32_t render_process_id,
    mojo::PendingReceiver<adblock::mojom::AdblockInterface> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<adblock::AdblockMojoInterfaceImpl>(render_process_id),
      std::move(receiver));
}

void UserAgentGetter(std::string* user_agent) {
  *user_agent = embedder_support::GetUserAgent();
}

class AdblockURLLoaderThrottleProxy : public blink::URLLoaderThrottle {
 public:
  AdblockURLLoaderThrottleProxy(
      int process_id,
      int render_frame_id,
      base::RepeatingCallback<void(std::string*)> ua_getter)
      : interface_(process_id),
        throttle_(&interface_, render_frame_id, ua_getter) {}
  ~AdblockURLLoaderThrottleProxy() override {}

  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override {
    throttle_.set_delegate(delegate_);
    throttle_.WillStartRequest(request, defer);
  }

  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override {
    throttle_.set_delegate(delegate_);
    throttle_.WillProcessResponse(response_url, response_head, defer);
  }

  void DetachFromCurrentSequence() override {
    throttle_.set_delegate(delegate_);
    throttle_.DetachFromCurrentSequence();
  }

 private:
  adblock::AdblockMojoInterfaceImpl interface_;
  adblock::AdblockURLLoaderThrottle throttle_;
};

}  // namespace

AdblockContentBrowserClient::AdblockContentBrowserClient() = default;

AdblockContentBrowserClient::~AdblockContentBrowserClient() = default;

bool AdblockContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
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
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(opener);

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(opener);
  GURL popup_url(target_url);
  web_contents->GetMainFrame()->GetProcess()->FilterURL(false, &popup_url);
  auto* request_classifier =
      adblock::AdblockRequestClassifierFactory::GetForBrowserContext(
          web_contents->GetBrowserContext());
  const auto popup_blocking_decision = request_classifier->ShouldBlockPopup(
      opener_top_level_frame_url, popup_url, opener);
  if (popup_blocking_decision == adblock::mojom::FilterMatchResult::kAllowRule)
    return true;
  if (popup_blocking_decision == adblock::mojom::FilterMatchResult::kBlockRule)
    return false;
  // Otherwise, if ABP adblocking is disabled or there is no rule that
  // explicitly allows or blocks a popup, fall back on Chromium's built-in popup
  // blocker.
  DCHECK(popup_blocking_decision ==
             adblock::mojom::FilterMatchResult::kDisabled ||
         popup_blocking_decision == adblock::mojom::FilterMatchResult::kNoRule);

  return ChromeContentBrowserClient::CanCreateWindow(
      opener, opener_url, opener_top_level_frame_url, source_origin,
      container_type, target_url, referrer, frame_name, disposition, features,
      user_gesture, opener_suppressed, no_javascript_access);
}

bool AdblockContentBrowserClient::WillInterceptWebSocket(
    content::RenderFrameHost* frame) {
  if (frame) {
    auto* profile =
        Profile::FromBrowserContext(frame->GetProcess()->GetBrowserContext());
    if (profile) {
      auto abp_enabled =
          profile->GetPrefs()->GetBoolean(adblock::prefs::kEnableAdblock);
      if (abp_enabled)
        return true;
    }
  }

  return ChromeContentBrowserClient::WillInterceptWebSocket(frame);
}

void AdblockContentBrowserClient::CreateWebSocket(
    content::RenderFrameHost* frame,
    WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  auto* profile =
      Profile::FromBrowserContext(frame->GetProcess()->GetBrowserContext());
  if (profile &&
      profile->GetPrefs()->GetBoolean(adblock::prefs::kEnableAdblock)) {
    auto* request_classifier =
        adblock::AdblockRequestClassifierFactory::GetForBrowserContext(profile);
    request_classifier->CheckFilterMatchForWebSocket(
        url, frame,
        base::BindOnce(
            &AdblockContentBrowserClient::OnWebSocketFilterCheckCompleted,
            weak_factory_.GetWeakPtr(), frame, std::move(factory), url,
            site_for_cookies, user_agent, std::move(handshake_client)));
  } else {
    DCHECK(ChromeContentBrowserClient::WillInterceptWebSocket(frame));
    ChromeContentBrowserClient::CreateWebSocket(frame, std::move(factory), url,
                                                site_for_cookies, user_agent,
                                                std::move(handshake_client));
  }
}

void AdblockContentBrowserClient::OnWebSocketFilterCheckCompleted(
    content::RenderFrameHost* frame,
    ChromeContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    adblock::mojom::FilterMatchResult result) {
  const bool has_blocking_filter =
      result == adblock::mojom::FilterMatchResult::kBlockRule;
  if (!has_blocking_filter) {
    LOG(INFO) << "[ABP] Web socket allowed for " << url;
    if (ChromeContentBrowserClient::WillInterceptWebSocket(frame)) {
      ChromeContentBrowserClient::CreateWebSocket(
          frame, std::move(factory), url, site_for_cookies, user_agent,
          std::move(handshake_client));
      return;
    }

    std::vector<network::mojom::HttpHeaderPtr> headers;
    if (user_agent) {
      headers.push_back(network::mojom::HttpHeader::New(
          net::HttpRequestHeaders::kUserAgent, *user_agent));
    }
    std::move(factory).Run(url, std::move(headers), std::move(handshake_client),
                           mojo::NullRemote(), mojo::NullRemote());
  }

  LOG(INFO) << "[ABP] Web socket blocked for " << url;
}

void AdblockContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  ChromeContentBrowserClient::ExposeInterfacesToRenderer(
      registry, associated_registry, render_process_host);

  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner =
      content::GetUIThreadTaskRunner({});

  registry->AddInterface(base::BindRepeating(&CreateAdblockInterfaceForRenderer,
                                             render_process_host->GetID()),
                         ui_task_runner);
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
AdblockContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const base::RepeatingCallback<content::WebContents*()>& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  auto throttlers = ChromeContentBrowserClient::CreateURLLoaderThrottles(
      request, browser_context, wc_getter, navigation_ui_data,
      frame_tree_node_id);

  auto* web_contents = wc_getter.Run();
  if (!web_contents)
    return throttlers;
  int process_id = web_contents->GetRenderViewHost()->GetProcess()->GetID();
  // TODO According to commit d1fa70a5fd6235bcb2fe9aa67afadf60c80edf10, this is
  // generally unsafe to use because a frame's RFH may change over its lifetime.
  // RenderFrameHost::FromID should be used instead, but our Android tests
  // currently fail with that.
  content::RenderFrameHost* frame =
      web_contents->UnsafeFindFrameByFrameTreeNodeId(frame_tree_node_id);
  if (!frame)
    return throttlers;
  throttlers.emplace_back(std::make_unique<AdblockURLLoaderThrottleProxy>(
      process_id, frame->GetRoutingID(),
      base::BindRepeating(&UserAgentGetter)));
  return throttlers;
}

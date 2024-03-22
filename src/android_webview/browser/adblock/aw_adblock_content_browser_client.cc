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

#include "android_webview/browser/adblock/aw_adblock_content_browser_client.h"

#include "android_webview/browser/adblock/aw_adblock_internals_ui.h"
#include "android_webview/browser/aw_browser_context.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/ranges/algorithm.h"
#include "components/adblock/content/browser/adblock_url_loader_factory.h"
#include "components/adblock/content/browser/factories/content_security_policy_injector_factory.h"
#include "components/adblock/content/browser/factories/element_hider_factory.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/sitekey_storage_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

#ifdef EYEO_INTERCEPT_DEBUG_URL
#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#endif

namespace {

bool IsFilteringNeeded(content::RenderFrameHost* frame) {
  if (frame) {
    auto* context = android_webview::AwBrowserContext::GetDefault();
    if (context) {
      return base::ranges::any_of(
          adblock::SubscriptionServiceFactory::GetForBrowserContext(context)
              ->GetInstalledFilteringConfigurations(),
          &adblock::FilteringConfiguration::IsEnabled);
    }
  }
  return false;
}

// Owns all of the AdblockURLLoaderFactory for a given webview BrowserContext.
class AdblockContextData : public base::SupportsUserData::Data {
 public:
  AdblockContextData(const AdblockContextData&) = delete;
  AdblockContextData& operator=(const AdblockContextData&) = delete;
  ~AdblockContextData() override = default;

  static void StartProxying(
      content::RenderFrameHost* frame,
      int render_process_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      bool use_test_loader) {
    const void* const kAdblockContextUserDataKey = &kAdblockContextUserDataKey;
    auto* browser_context = android_webview::AwBrowserContext::GetDefault();
    auto* self = static_cast<AdblockContextData*>(
        browser_context->GetUserData(kAdblockContextUserDataKey));
    if (!self) {
      self = new AdblockContextData();
      browser_context->SetUserData(kAdblockContextUserDataKey,
                                   base::WrapUnique(self));
    }
    adblock::AdblockURLLoaderFactoryConfig config{
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            browser_context),
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser_context),
        adblock::ElementHiderFactory::GetForBrowserContext(browser_context),
        adblock::SitekeyStorageFactory::GetForBrowserContext(browser_context),
        adblock::ContentSecurityPolicyInjectorFactory::GetForBrowserContext(
            browser_context)};
#ifdef EYEO_INTERCEPT_DEBUG_URL
    if (use_test_loader) {
      auto proxy = std::make_unique<adblock::AdblockURLLoaderFactoryForTest>(
          std::move(config),
          content::GlobalRenderFrameHostId(render_process_id,
                                           frame->GetRoutingID()),
          std::move(receiver), std::move(target_factory),
          android_webview::GetUserAgent(),
          base::BindOnce(&AdblockContextData::RemoveProxy,
                         self->weak_factory_.GetWeakPtr()),
          adblock::SubscriptionServiceFactory::GetForBrowserContext(
              browser_context));
      self->proxies_.emplace(std::move(proxy));
      return;
    }
#endif
    auto proxy = std::make_unique<adblock::AdblockURLLoaderFactory>(
        std::move(config),
        content::GlobalRenderFrameHostId(render_process_id,
                                         frame->GetRoutingID()),
        std::move(receiver), std::move(target_factory),
        android_webview::GetUserAgent(),
        base::BindOnce(&AdblockContextData::RemoveProxy,
                       self->weak_factory_.GetWeakPtr()));
    self->proxies_.emplace(std::move(proxy));
  }

 private:
  void RemoveProxy(adblock::AdblockURLLoaderFactory* proxy) {
    auto it = proxies_.find(proxy);
    DCHECK(it != proxies_.end());
    proxies_.erase(it);
  }

  AdblockContextData() = default;

  std::set<std::unique_ptr<adblock::AdblockURLLoaderFactory>,
           base::UniquePtrComparator>
      proxies_;

  base::WeakPtrFactory<AdblockContextData> weak_factory_{this};
};

}  // namespace

AwAdblockContentBrowserClient::AwAdblockContentBrowserClient(
    android_webview::AwFeatureListCreator* aw_feature_list_creator)
    : android_webview::AwContentBrowserClient(aw_feature_list_creator) {}

AwAdblockContentBrowserClient::~AwAdblockContentBrowserClient() = default;

bool AwAdblockContentBrowserClient::CanCreateWindow(
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

  if (IsFilteringNeeded(opener)) {
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(opener);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            android_webview::AwBrowserContext::GetDefault());

    GURL popup_url(target_url);
    web_contents->GetPrimaryMainFrame()->GetProcess()->FilterURL(false,
                                                                 &popup_url);
    auto* classification_runner =
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            android_webview::AwBrowserContext::GetDefault());
    const auto popup_blocking_decision =
        classification_runner->ShouldBlockPopup(
            subscription_service->GetCurrentSnapshot(), popup_url, opener);
    if (popup_blocking_decision == adblock::FilterMatchResult::kAllowRule) {
      return true;
    }
    if (popup_blocking_decision == adblock::FilterMatchResult::kBlockRule) {
      return false;
    }
    // Otherwise, if eyeo adblocking is disabled or there is no rule that
    // explicitly allows or blocks a popup, fall back on Chromium's built-in
    // popup blocker.
    DCHECK(popup_blocking_decision == adblock::FilterMatchResult::kDisabled ||
           popup_blocking_decision == adblock::FilterMatchResult::kNoRule);
  }

  return android_webview::AwContentBrowserClient::CanCreateWindow(
      opener, opener_url, opener_top_level_frame_url, source_origin,
      container_type, target_url, referrer, frame_name, disposition, features,
      user_gesture, opener_suppressed, no_javascript_access);
}

bool AwAdblockContentBrowserClient::WillInterceptWebSocket(
    content::RenderFrameHost* frame) {
  if (IsFilteringNeeded(frame)) {
    return true;
  }

  return android_webview::AwContentBrowserClient::WillInterceptWebSocket(frame);
}

void AwAdblockContentBrowserClient::CreateWebSocket(
    content::RenderFrameHost* frame,
    WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  if (IsFilteringNeeded(frame)) {
    CreateWebSocketInternal(frame->GetGlobalId(), std::move(factory), url,
                            site_for_cookies, user_agent,
                            std::move(handshake_client));
  } else {
    DCHECK(
        android_webview::AwContentBrowserClient::WillInterceptWebSocket(frame));
    android_webview::AwContentBrowserClient::CreateWebSocket(
        frame, std::move(factory), url, site_for_cookies, user_agent,
        std::move(handshake_client));
  }
}

void AwAdblockContentBrowserClient::CreateWebSocketInternal(
    content::GlobalRenderFrameHostId render_frame_host_id,
    WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  auto* frame = content::RenderFrameHost::FromID(render_frame_host_id);
  if (!frame) {
    return;
  }
  auto* browser_context = android_webview::AwBrowserContext::GetDefault();
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(
          browser_context);
  auto* classification_runner =
      adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
          browser_context);
  classification_runner->CheckRequestFilterMatchForWebSocket(
      subscription_service->GetCurrentSnapshot(), url, render_frame_host_id,
      base::BindOnce(
          &AwAdblockContentBrowserClient::OnWebSocketFilterCheckCompleted,
          weak_factory_.GetWeakPtr(), render_frame_host_id, std::move(factory),
          url, site_for_cookies, user_agent, std::move(handshake_client)));
}

void AwAdblockContentBrowserClient::OnWebSocketFilterCheckCompleted(
    content::GlobalRenderFrameHostId render_frame_host_id,
    android_webview::AwContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const absl::optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    adblock::FilterMatchResult result) {
  auto* frame = content::RenderFrameHost::FromID(render_frame_host_id);
  if (!frame) {
    return;
  }
  const bool has_blocking_filter =
      result == adblock::FilterMatchResult::kBlockRule;
  if (!has_blocking_filter) {
    VLOG(1) << "[eyeo] Web socket allowed for " << url;
    if (android_webview::AwContentBrowserClient::WillInterceptWebSocket(
            frame)) {
      android_webview::AwContentBrowserClient::CreateWebSocket(
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

  VLOG(1) << "[eyeo] Web socket blocked for " << url;
}

bool AwAdblockContentBrowserClient::WillCreateURLLoaderFactory(
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
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner) {
  // Create Chromium proxy first as WebRequestProxyingURLLoaderFactory logic
  // depends on being first proxy
  bool use_chrome_proxy = AwContentBrowserClient::WillCreateURLLoaderFactory(
      browser_context, frame, render_process_id, type, request_initiator,
      navigation_id, ukm_source_id, factory_receiver, header_client,
      bypass_redirect_checks, disable_secure_dns, factory_override,
      navigation_response_task_runner);

  bool use_adblock_proxy =
      (type == URLLoaderFactoryType::kDocumentSubResource ||
       type == URLLoaderFactoryType::kNavigation) &&
      IsFilteringNeeded(frame);

  bool use_test_loader = false;
#ifdef EYEO_INTERCEPT_DEBUG_URL
  content::WebContents* wc = content::WebContents::FromRenderFrameHost(frame);
  use_test_loader =
      (type ==
       content::ContentBrowserClient::URLLoaderFactoryType::kNavigation) &&
      wc->GetVisibleURL().is_valid() &&
      wc->GetVisibleURL().host() ==
          adblock::AdblockURLLoaderFactoryForTest::kAdblockDebugDataHostName;
  use_adblock_proxy |= use_test_loader;
#endif

  if (use_adblock_proxy) {
    auto proxied_receiver = std::move(*factory_receiver);
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote;
    *factory_receiver = target_factory_remote.InitWithNewPipeAndPassReceiver();
    AdblockContextData::StartProxying(
        frame, render_process_id, std::move(proxied_receiver),
        std::move(target_factory_remote), use_test_loader);
  }
  return use_adblock_proxy || use_chrome_proxy;
}

void AwAdblockContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  AwContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
      render_frame_host, map);
  content::RegisterWebUIControllerInterfaceBinder<
      ::mojom::adblock_internals::AdblockInternalsPageHandler,
      adblock::AwAdblockInternalsUI>(map);
}

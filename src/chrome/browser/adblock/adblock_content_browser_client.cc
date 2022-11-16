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

#include "chrome/browser/adblock/adblock_content_browser_client.h"

#include "base/containers/unique_ptr_adapters.h"
#include "chrome/browser/adblock/adblock_mojo_interface_impl.h"
#include "chrome/browser/adblock/resource_classification_runner_factory.h"
#include "chrome/browser/adblock/subscription_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/content/common/adblock_url_loader_factory.h"
#include "components/adblock/content/common/mojom/adblock.mojom.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/subscription_service.h"
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
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace {

void CreateAdblockInterfaceForRenderer(
    int32_t render_process_id,
    mojo::PendingReceiver<adblock::mojom::AdblockInterface> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<adblock::AdblockMojoInterfaceImpl>(render_process_id),
      std::move(receiver));
}

bool IsAdblockEnabled(content::RenderFrameHost* frame) {
  if (frame) {
    auto* profile =
        Profile::FromBrowserContext(frame->GetProcess()->GetBrowserContext());
    if (profile) {
      return profile->GetPrefs()->GetBoolean(adblock::prefs::kEnableAdblock);
    }
  }
  return false;
}

// Owns all of the AdblockURLLoaderFactory for a given Profile.
class AdblockContextData : public base::SupportsUserData::Data {
 public:
  AdblockContextData(const AdblockContextData&) = delete;
  AdblockContextData& operator=(const AdblockContextData&) = delete;
  ~AdblockContextData() override = default;

  static void StartProxying(
      Profile* profile,
      content::RenderFrameHost* frame,
      int render_process_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory) {
    const void* const kAdblockContextUserDataKey = &kAdblockContextUserDataKey;
    auto* self = static_cast<AdblockContextData*>(
        profile->GetUserData(kAdblockContextUserDataKey));
    if (!self) {
      self = new AdblockContextData();
      profile->SetUserData(kAdblockContextUserDataKey, base::WrapUnique(self));
    }
    auto proxy = std::make_unique<adblock::AdblockURLLoaderFactory>(
        std::make_unique<adblock::AdblockMojoInterfaceImpl>(render_process_id),
        frame->GetRoutingID(), std::move(receiver), std::move(target_factory),
        embedder_support::GetUserAgent(),
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

  if (IsAdblockEnabled(opener)) {
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(opener);
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(
            web_contents->GetBrowserContext());
    if (subscription_service->IsInitialized()) {
      GURL popup_url(target_url);
      web_contents->GetPrimaryMainFrame()->GetProcess()->FilterURL(false,
                                                                   &popup_url);
      auto* classification_runner =
          adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
              web_contents->GetBrowserContext());
      const auto popup_blocking_decision =
          classification_runner->ShouldBlockPopup(
              subscription_service->GetCurrentSnapshot(),
              opener_top_level_frame_url, popup_url, opener);
      if (popup_blocking_decision ==
          adblock::mojom::FilterMatchResult::kAllowRule)
        return true;
      if (popup_blocking_decision ==
          adblock::mojom::FilterMatchResult::kBlockRule)
        return false;
      // Otherwise, if eyeo adblocking is disabled or there is no rule that
      // explicitly allows or blocks a popup, fall back on Chromium's built-in
      // popup blocker.
      DCHECK(popup_blocking_decision ==
                 adblock::mojom::FilterMatchResult::kDisabled ||
             popup_blocking_decision ==
                 adblock::mojom::FilterMatchResult::kNoRule);
    }
  }

  return ChromeContentBrowserClient::CanCreateWindow(
      opener, opener_url, opener_top_level_frame_url, source_origin,
      container_type, target_url, referrer, frame_name, disposition, features,
      user_gesture, opener_suppressed, no_javascript_access);
}

bool AdblockContentBrowserClient::WillInterceptWebSocket(
    content::RenderFrameHost* frame) {
  if (IsAdblockEnabled(frame)) {
    return true;
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
    auto* subscription_service =
        adblock::SubscriptionServiceFactory::GetForBrowserContext(profile);
    if (!subscription_service->IsInitialized()) {
      subscription_service->RunWhenInitialized(base::BindOnce(
          &AdblockContentBrowserClient::CreateWebSocket,
          weak_factory_.GetWeakPtr(), frame, std::move(factory), url,
          site_for_cookies, user_agent, std::move(handshake_client)));
      return;
    }
    auto* classification_runner =
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            profile);
    classification_runner->CheckRequestFilterMatchForWebSocket(
        subscription_service->GetCurrentSnapshot(), url, frame,
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
    LOG(INFO) << "[eyeo] Web socket allowed for " << url;
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

  LOG(INFO) << "[eyeo] Web socket blocked for " << url;
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

bool AdblockContentBrowserClient::WillCreateURLLoaderFactory(
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
    network::mojom::URLLoaderFactoryOverridePtr* factory_override) {
  // Create Chromium proxy first as WebRequestProxyingURLLoaderFactory logic
  // depends on being first proxy
  bool use_chrome_proxy =
      ChromeContentBrowserClient::WillCreateURLLoaderFactory(
          browser_context, frame, render_process_id, type, request_initiator,
          navigation_id, ukm_source_id, factory_receiver, header_client,
          bypass_redirect_checks, disable_secure_dns, factory_override);
  auto* profile = frame ? Profile::FromBrowserContext(
                              frame->GetProcess()->GetBrowserContext())
                        : nullptr;
  bool use_adblock_proxy =
      (type == URLLoaderFactoryType::kDocumentSubResource ||
       type == URLLoaderFactoryType::kNavigation) &&
      profile &&
      profile->GetPrefs()->GetBoolean(adblock::prefs::kEnableAdblock);
  if (use_adblock_proxy) {
    auto proxied_receiver = std::move(*factory_receiver);
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote;
    *factory_receiver = target_factory_remote.InitWithNewPipeAndPassReceiver();
    AdblockContextData::StartProxying(profile, frame, render_process_id,
                                      std::move(proxied_receiver),
                                      std::move(target_factory_remote));
  }
  return use_adblock_proxy || use_chrome_proxy;
}

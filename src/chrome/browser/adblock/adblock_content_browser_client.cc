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
#include "base/ranges/algorithm.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
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
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

#ifdef EYEO_INTERCEPT_DEBUG_URL
#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_util.h"
#endif

namespace {

bool IsFilteringNeeded(content::RenderFrameHost* frame) {
  if (frame) {
    auto* profile =
        Profile::FromBrowserContext(frame->GetProcess()->GetBrowserContext())
            ->GetOriginalProfile();
    if (profile) {
      // Filtering may be needed if there's at least one enabled
      // FilteringConfiguration.
      return base::ranges::any_of(
          adblock::SubscriptionServiceFactory::GetForBrowserContext(profile)
              ->GetInstalledFilteringConfigurations(),
          &adblock::FilteringConfiguration::IsEnabled);
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
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      bool use_test_loader) {
    const void* const kAdblockContextUserDataKey = &kAdblockContextUserDataKey;
    auto* self = static_cast<AdblockContextData*>(
        profile->GetUserData(kAdblockContextUserDataKey));
    if (!self) {
      self = new AdblockContextData();
      profile->SetUserData(kAdblockContextUserDataKey, base::WrapUnique(self));
    }
    adblock::AdblockURLLoaderFactoryConfig config{
        adblock::SubscriptionServiceFactory::GetForBrowserContext(profile),
        adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
            profile),
        adblock::ElementHiderFactory::GetForBrowserContext(profile),
        adblock::SitekeyStorageFactory::GetForBrowserContext(profile),
        adblock::ContentSecurityPolicyInjectorFactory::GetForBrowserContext(
            profile)};
#ifdef EYEO_INTERCEPT_DEBUG_URL
    if (use_test_loader) {
      auto proxy = std::make_unique<adblock::AdblockURLLoaderFactoryForTest>(
          std::move(config),
          content::GlobalRenderFrameHostId(render_process_id,
                                           frame->GetRoutingID()),
          std::move(receiver), std::move(target_factory),
          embedder_support::GetUserAgent(),
          base::BindOnce(&AdblockContextData::RemoveProxy,
                         self->weak_factory_.GetWeakPtr()),
          adblock::SubscriptionServiceFactory::GetForBrowserContext(profile));
      self->proxies_.emplace(std::move(proxy));
      return;
    }
#endif
    auto proxy = std::make_unique<adblock::AdblockURLLoaderFactory>(
        std::move(config),
        content::GlobalRenderFrameHostId(render_process_id,
                                         frame->GetRoutingID()),
        std::move(receiver), std::move(target_factory),
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

#if BUILDFLAG(ENABLE_EXTENSIONS)
// static
bool AdblockContentBrowserClient::force_adblock_proxy_for_testing_ = false;

// static
void AdblockContentBrowserClient::ForceAdblockProxyForTesting() {
  force_adblock_proxy_for_testing_ = true;
}
#endif

bool AdblockContentBrowserClient::WillInterceptWebSocket(
    content::RenderFrameHost* frame) {
  if (IsFilteringNeeded(frame)) {
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
  if (IsFilteringNeeded(frame)) {
    CreateWebSocketInternal(frame->GetGlobalId(), std::move(factory), url,
                            site_for_cookies, user_agent,
                            std::move(handshake_client));
  } else {
    DCHECK(ChromeContentBrowserClient::WillInterceptWebSocket(frame));
    ChromeContentBrowserClient::CreateWebSocket(frame, std::move(factory), url,
                                                site_for_cookies, user_agent,
                                                std::move(handshake_client));
  }
}

void AdblockContentBrowserClient::CreateWebSocketInternal(
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
  auto* profile =
      Profile::FromBrowserContext(frame->GetProcess()->GetBrowserContext())
          ->GetOriginalProfile();
  auto* subscription_service =
      adblock::SubscriptionServiceFactory::GetForBrowserContext(profile);
  auto* classification_runner =
      adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
          profile);
  classification_runner->CheckRequestFilterMatchForWebSocket(
      subscription_service->GetCurrentSnapshot(), url, render_frame_host_id,
      base::BindOnce(
          &AdblockContentBrowserClient::OnWebSocketFilterCheckCompleted,
          weak_factory_.GetWeakPtr(), render_frame_host_id, std::move(factory),
          url, site_for_cookies, user_agent, std::move(handshake_client)));
}

void AdblockContentBrowserClient::OnWebSocketFilterCheckCompleted(
    content::GlobalRenderFrameHostId render_frame_host_id,
    ChromeContentBrowserClient::WebSocketFactory factory,
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

  VLOG(1) << "[eyeo] Web socket blocked for " << url;
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
    network::mojom::URLLoaderFactoryOverridePtr* factory_override,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner) {
  // Create Chromium proxy first as WebRequestProxyingURLLoaderFactory logic
  // depends on being first proxy
  bool use_chrome_proxy =
      ChromeContentBrowserClient::WillCreateURLLoaderFactory(
          browser_context, frame, render_process_id, type, request_initiator,
          navigation_id, ukm_source_id, factory_receiver, header_client,
          bypass_redirect_checks, disable_secure_dns, factory_override,
          navigation_response_task_runner);
  auto* profile = frame ? Profile::FromBrowserContext(
                              frame->GetProcess()->GetBrowserContext())
                              ->GetOriginalProfile()
                        : nullptr;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!force_adblock_proxy_for_testing_ &&
      request_initiator.scheme() == extensions::kExtensionScheme) {
    VLOG(1) << "[eyeo] Do not use adblock proxy for extensions requests "
               "[extension id:"
            << request_initiator.host() << "].";
    return use_chrome_proxy;
  }
#endif

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
        profile, frame, render_process_id, std::move(proxied_receiver),
        std::move(target_factory_remote), use_test_loader);
  }
  return use_adblock_proxy || use_chrome_proxy;
}

// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

#include "chrome/browser/extensions/api/adblock_private/adblock_private_api.h"

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/time/time_to_iso8601.h"
#include "base/values.h"
#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/adblock/resource_classification_runner_factory.h"
#include "chrome/browser/adblock/session_stats_factory.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/common/extensions/api/tabs.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/allowed_connection_type.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/session_stats.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace extensions {

namespace {

enum class BuiltInSubscriptionAction { kSelect, kUnselect };

std::string RunBuiltInSubscriptionAction(
    BuiltInSubscriptionAction action,
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (!url.is_valid())
    return "Invalid URL";
  auto* controller =
      adblock::AdblockControllerFactory::GetForBrowserContext(browser_context);
  auto recommended = controller->GetKnownSubscriptions();
  if (!recommended.empty()) {
    auto item =
        std::find_if(recommended.begin(), recommended.end(),
                     [&url](const adblock::KnownSubscriptionInfo& recommended) {
                       return recommended.url == url;
                     });
    if (item == recommended.end())
      return "Not a built-in subscription";
  }

  switch (action) {
    case BuiltInSubscriptionAction::kSelect:
      controller->SelectBuiltInSubscription(url);
      break;
    case BuiltInSubscriptionAction::kUnselect:
      controller->UnselectBuiltInSubscription(url);
      break;
    default:
      NOTREACHED();
  }

  return {};
}

enum class CustomSubscriptionAction { kAdd, kRemove };

std::string RunCustomSubscriptionAction(
    CustomSubscriptionAction action,
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (!url.is_valid())
    return "Invalid URL";
  auto* controller =
      adblock::AdblockControllerFactory::GetForBrowserContext(browser_context);
  switch (action) {
    case CustomSubscriptionAction::kAdd:
      controller->AddCustomSubscription(url);
      break;
    case CustomSubscriptionAction::kRemove:
      controller->RemoveCustomSubscription(url);
      break;
    default:
      NOTREACHED();
  }

  return {};
}

std::vector<api::adblock_private::SessionStatsEntry> CopySessionsStats(
    const std::map<GURL, long>& source) {
  std::vector<api::adblock_private::SessionStatsEntry> result;
  for (auto& entry : source) {
    api::adblock_private::SessionStatsEntry js_entry;
    js_entry.url = entry.first.spec();
    js_entry.count = entry.second;
    result.emplace_back(std::move(js_entry));
  }
  return result;
}

std::string SubscriptionInstallationStateToString(
    adblock::Subscription::InstallationState state) {
  using State = adblock::Subscription::InstallationState;
  switch (state) {
    case State::Installed:
      return "Installed";
    case State::Installing:
      return "Installing";
    case State::Preloaded:
      return "Preloaded";
  }
  NOTREACHED();
  return "";
}

std::vector<api::adblock_private::Subscription> CopySubscriptions(
    const std::vector<scoped_refptr<adblock::Subscription>>
        current_subscriptions) {
  std::vector<api::adblock_private::Subscription> result;
  for (auto& sub : current_subscriptions) {
    api::adblock_private::Subscription js_sub;
    js_sub.url = sub->GetSourceUrl().spec();
    js_sub.title = sub->GetTitle();
    js_sub.current_version = sub->GetCurrentVersion();
    js_sub.installation_state =
        SubscriptionInstallationStateToString(sub->GetInstallationState());
    js_sub.last_installation_time =
        base::TimeToISO8601(sub->GetInstallationTime());
    result.emplace_back(std::move(js_sub));
  }
  return result;
}

}  // namespace

template <>
void BrowserContextKeyedAPIFactory<
    AdblockPrivateAPI>::DeclareFactoryDependencies() {
  DependsOn(adblock::AdblockControllerFactory::GetInstance());
  DependsOn(adblock::ResourceClassificationRunnerFactory::GetInstance());
  DependsOn(adblock::SessionStatsFactory::GetInstance());
}

// static
BrowserContextKeyedAPIFactory<AdblockPrivateAPI>*
AdblockPrivateAPI::GetFactoryInstance() {
  static base::NoDestructor<BrowserContextKeyedAPIFactory<AdblockPrivateAPI>>
      instance;
  return instance.get();
}

class AdblockPrivateAPI::AdblockAPIEventRouter
    : public adblock::ResourceClassificationRunner::Observer,
      public adblock::AdblockController::Observer {
 public:
  explicit AdblockAPIEventRouter(content::BrowserContext* context)
      : context_(context) {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context)
        ->AddObserver(this);
    adblock::AdblockControllerFactory::GetForBrowserContext(context)
        ->AddObserver(this);
  }

  ~AdblockAPIEventRouter() override {
    adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(context_)
        ->RemoveObserver(this);
    adblock::AdblockControllerFactory::GetForBrowserContext(context_)
        ->RemoveObserver(this);
  }

  // adblock::ResourceClassificationRunner::Observer:
  void OnAdMatched(const GURL& url,
                   adblock::mojom::FilterMatchResult match_result,
                   const std::vector<GURL>& parent_frame_urls,
                   adblock::ContentType content_type,
                   content::RenderFrameHost* render_frame_host,
                   const GURL& subscription) override {
    std::unique_ptr<Event> event;
    api::adblock_private::AdInfo info =
        CreateAdInfoObject(url, subscription, render_frame_host);
    info.parent_frame_urls = adblock::utils::ConvertURLs(parent_frame_urls);
    info.content_type = adblock::ContentTypeToString(content_type);

    if (match_result == adblock::mojom::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::ADBLOCK_PRIVATE_AD_BLOCKED,
          api::adblock_private::OnAdBlocked::kEventName,
          api::adblock_private::OnAdBlocked::Create(info), context_);
    } else {
      DCHECK(match_result == adblock::mojom::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::ADBLOCK_PRIVATE_AD_ALLOWED,
          api::adblock_private::OnAdAllowed::kEventName,
          api::adblock_private::OnAdAllowed::Create(info), context_);
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription) override {
    api::adblock_private::AdInfo info =
        CreateAdInfoObject(url, subscription, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{};
    info.content_type = "";

    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::ADBLOCK_PRIVATE_PAGE_ALLOWED,
        api::adblock_private::OnPageAllowed::kEventName,
        api::adblock_private::OnPageAllowed::Create(info), context_);

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  void OnPopupMatched(const GURL& url,
                      adblock::mojom::FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription) override {
    std::unique_ptr<Event> event;
    api::adblock_private::AdInfo info =
        CreateAdInfoObject(url, subscription, render_frame_host);
    info.parent_frame_urls = std::vector<std::string>{opener_url.spec()};
    info.content_type = "";

    if (match_result == adblock::mojom::FilterMatchResult::kBlockRule) {
      event = std::make_unique<Event>(
          events::ADBLOCK_PRIVATE_POPUP_BLOCKED,
          api::adblock_private::OnPopupBlocked::kEventName,
          api::adblock_private::OnPopupBlocked::Create(info), context_);
    } else {
      DCHECK(match_result == adblock::mojom::FilterMatchResult::kAllowRule);
      event = std::make_unique<Event>(
          events::ADBLOCK_PRIVATE_POPUP_ALLOWED,
          api::adblock_private::OnPopupAllowed::kEventName,
          api::adblock_private::OnPopupAllowed::Create(info), context_);
    }

    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

  // adblock::AdblockController::Observer:
  void OnSubscriptionUpdated(const GURL& url) override {
    std::unique_ptr<Event> event = std::make_unique<Event>(
        events::ADBLOCK_PRIVATE_SUBSCRIPTION_UPDATED,
        api::adblock_private::OnSubscriptionUpdated::kEventName,
        api::adblock_private::OnSubscriptionUpdated::Create(url.spec()),
        context_);
    extensions::EventRouter::Get(context_)->BroadcastEvent(std::move(event));
  }

 private:
  api::adblock_private::AdInfo CreateAdInfoObject(
      const GURL& url,
      const GURL& subscription,
      content::RenderFrameHost* render_frame_host) {
    DCHECK(render_frame_host);
    api::adblock_private::AdInfo info;
    info.url = url.spec();
    info.subscription = subscription.spec();
    info.tab_id = api::tabs::TAB_ID_NONE;
    info.window_id = SessionID::InvalidValue().id();
    const content::WebContents* wc =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    if (wc) {
      info.tab_id = ExtensionTabUtil::GetTabId(wc);
      info.window_id = ExtensionTabUtil::GetWindowIdOfTab(wc);
    }
    return info;
  }

  content::BrowserContext* context_;
};

void AdblockPrivateAPI::Shutdown() {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->UnregisterObserver(this);
  }
  event_router_.reset();
}

// static
AdblockPrivateAPI* AdblockPrivateAPI::Get(content::BrowserContext* context) {
  return GetFactoryInstance()->Get(context);
}

AdblockPrivateAPI::AdblockPrivateAPI(content::BrowserContext* context)
    : context_(context) {
  // EventRouter can be null in tests
  if (EventRouter::Get(context_)) {
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnAdAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnAdBlocked::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPageAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPopupAllowed::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnPopupBlocked::kEventName);
    EventRouter::Get(context_)->RegisterObserver(
        this, api::adblock_private::OnSubscriptionUpdated::kEventName);
  }
  adblock::SessionStatsFactory::GetForBrowserContext(context)
      ->StartCollectingStats();
}

AdblockPrivateAPI::~AdblockPrivateAPI() = default;

void AdblockPrivateAPI::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  event_router_ =
      std::make_unique<AdblockPrivateAPI::AdblockAPIEventRouter>(context_);
  EventRouter::Get(context_)->UnregisterObserver(this);
}

namespace api {

AdblockPrivateSetUpdateConsentFunction::
    AdblockPrivateSetUpdateConsentFunction() {}

AdblockPrivateSetUpdateConsentFunction::
    ~AdblockPrivateSetUpdateConsentFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateSetUpdateConsentFunction::Run() {
  std::unique_ptr<api::adblock_private::SetUpdateConsent::Params> params(
      api::adblock_private::SetUpdateConsent::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  switch (params->consent) {
    case api::adblock_private::UpdateConsent::UPDATE_CONSENT_WIFI_ONLY:
      controller->SetUpdateConsent(adblock::AllowedConnectionType::kWiFi);
      break;
    case api::adblock_private::UpdateConsent::UPDATE_CONSENT_ALWAYS:
      controller->SetUpdateConsent(adblock::AllowedConnectionType::kAny);
      break;
    default:
      NOTREACHED();
  }

  return RespondNow(NoArguments());
}

AdblockPrivateGetUpdateConsentFunction::
    AdblockPrivateGetUpdateConsentFunction() {}

AdblockPrivateGetUpdateConsentFunction::
    ~AdblockPrivateGetUpdateConsentFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetUpdateConsentFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());

  auto consent = controller->GetUpdateConsent();
  api::adblock_private::UpdateConsent result =
      api::adblock_private::UpdateConsent::UPDATE_CONSENT_NONE;
  switch (consent) {
    case adblock::AllowedConnectionType::kWiFi:
      result = api::adblock_private::UpdateConsent::UPDATE_CONSENT_WIFI_ONLY;
      break;
    case adblock::AllowedConnectionType::kAny:
      result = api::adblock_private::UpdateConsent::UPDATE_CONSENT_ALWAYS;
      break;
    // TODO what about NONE? This will trigger a dcheck...
    default:
      NOTREACHED();
  }

  return RespondNow(ArgumentList(
      api::adblock_private::GetUpdateConsent::Results::Create(result)));
}

AdblockPrivateSetEnabledFunction::AdblockPrivateSetEnabledFunction() {}

AdblockPrivateSetEnabledFunction::~AdblockPrivateSetEnabledFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateSetEnabledFunction::Run() {
  std::unique_ptr<api::adblock_private::SetEnabled::Params> params(
      api::adblock_private::SetEnabled::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->SetAdblockEnabled(params->enabled);
  return RespondNow(NoArguments());
}

AdblockPrivateIsEnabledFunction::AdblockPrivateIsEnabledFunction() {}

AdblockPrivateIsEnabledFunction::~AdblockPrivateIsEnabledFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateIsEnabledFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  return RespondNow(
      ArgumentList(api::adblock_private::IsEnabled::Results::Create(
          controller->IsAdblockEnabled())));
}

AdblockPrivateSetAcceptableAdsEnabledFunction::
    AdblockPrivateSetAcceptableAdsEnabledFunction() {}

AdblockPrivateSetAcceptableAdsEnabledFunction::
    ~AdblockPrivateSetAcceptableAdsEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateSetAcceptableAdsEnabledFunction::Run() {
  std::unique_ptr<api::adblock_private::SetAcceptableAdsEnabled::Params> params(
      api::adblock_private::SetAcceptableAdsEnabled::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->SetAcceptableAdsEnabled(params->enabled);

  return RespondNow(NoArguments());
}

AdblockPrivateIsAcceptableAdsEnabledFunction::
    AdblockPrivateIsAcceptableAdsEnabledFunction() {}

AdblockPrivateIsAcceptableAdsEnabledFunction::
    ~AdblockPrivateIsAcceptableAdsEnabledFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateIsAcceptableAdsEnabledFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  return RespondNow(ArgumentList(
      api::adblock_private::IsAcceptableAdsEnabled::Results::Create(
          controller->IsAcceptableAdsEnabled())));
}

AdblockPrivateGetBuiltInSubscriptionsFunction::
    AdblockPrivateGetBuiltInSubscriptionsFunction() {}

AdblockPrivateGetBuiltInSubscriptionsFunction::
    ~AdblockPrivateGetBuiltInSubscriptionsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetBuiltInSubscriptionsFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  auto recommended = controller->GetKnownSubscriptions();
  std::vector<api::adblock_private::BuiltInSubscription> result;
  for (auto& recommended_one : recommended) {
    if (recommended_one.ui_visibility ==
        adblock::SubscriptionUiVisibility::Visible) {
      api::adblock_private::BuiltInSubscription js_recommended;
      js_recommended.url = recommended_one.url.spec();
      js_recommended.title = recommended_one.title;
      js_recommended.languages = recommended_one.languages;
      result.emplace_back(std::move(js_recommended));
    }
  }
  return RespondNow(ArgumentList(
      api::adblock_private::GetBuiltInSubscriptions::Results::Create(result)));
}

AdblockPrivateSelectBuiltInSubscriptionFunction::
    AdblockPrivateSelectBuiltInSubscriptionFunction() {}

AdblockPrivateSelectBuiltInSubscriptionFunction::
    ~AdblockPrivateSelectBuiltInSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateSelectBuiltInSubscriptionFunction::Run() {
  std::unique_ptr<api::adblock_private::SelectBuiltInSubscription::Params>
      params(api::adblock_private::SelectBuiltInSubscription::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status = RunBuiltInSubscriptionAction(BuiltInSubscriptionAction::kSelect,
                                             browser_context(), url);
  if (!status.empty())
    return RespondNow(Error(status));

  return RespondNow(NoArguments());
}

AdblockPrivateUnselectBuiltInSubscriptionFunction::
    AdblockPrivateUnselectBuiltInSubscriptionFunction() {}

AdblockPrivateUnselectBuiltInSubscriptionFunction::
    ~AdblockPrivateUnselectBuiltInSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateUnselectBuiltInSubscriptionFunction::Run() {
  std::unique_ptr<api::adblock_private::UnselectBuiltInSubscription::Params>
      params(api::adblock_private::UnselectBuiltInSubscription::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status = RunBuiltInSubscriptionAction(
      BuiltInSubscriptionAction::kUnselect, browser_context(), url);
  if (!status.empty())
    return RespondNow(Error(status));

  return RespondNow(NoArguments());
}

AdblockPrivateGetSelectedBuiltInSubscriptionsFunction::
    AdblockPrivateGetSelectedBuiltInSubscriptionsFunction() {}

AdblockPrivateGetSelectedBuiltInSubscriptionsFunction::
    ~AdblockPrivateGetSelectedBuiltInSubscriptionsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetSelectedBuiltInSubscriptionsFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());

  return RespondNow(ArgumentList(
      api::adblock_private::GetSelectedBuiltInSubscriptions::Results::Create(
          CopySubscriptions(controller->GetSelectedBuiltInSubscriptions()))));
}

AdblockPrivateAddCustomSubscriptionFunction::
    AdblockPrivateAddCustomSubscriptionFunction() {}

AdblockPrivateAddCustomSubscriptionFunction::
    ~AdblockPrivateAddCustomSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateAddCustomSubscriptionFunction::Run() {
  std::unique_ptr<api::adblock_private::AddCustomSubscription::Params> params(
      api::adblock_private::AddCustomSubscription::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status = RunCustomSubscriptionAction(CustomSubscriptionAction::kAdd,
                                            browser_context(), url);
  if (!status.empty())
    return RespondNow(Error(status));

  return RespondNow(NoArguments());
}

AdblockPrivateRemoveCustomSubscriptionFunction::
    AdblockPrivateRemoveCustomSubscriptionFunction() {}

AdblockPrivateRemoveCustomSubscriptionFunction::
    ~AdblockPrivateRemoveCustomSubscriptionFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateRemoveCustomSubscriptionFunction::Run() {
  std::unique_ptr<api::adblock_private::RemoveCustomSubscription::Params>
      params(api::adblock_private::RemoveCustomSubscription::Params::Create(
          args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto url = GURL{params->url};
  auto status = RunCustomSubscriptionAction(CustomSubscriptionAction::kRemove,
                                            browser_context(), url);
  if (!status.empty())
    return RespondNow(Error(status));

  return RespondNow(NoArguments());
}

AdblockPrivateGetCustomSubscriptionsFunction::
    AdblockPrivateGetCustomSubscriptionsFunction() {}

AdblockPrivateGetCustomSubscriptionsFunction::
    ~AdblockPrivateGetCustomSubscriptionsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetCustomSubscriptionsFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  return RespondNow(ArgumentList(
      api::adblock_private::GetCustomSubscriptions::Results::Create(
          CopySubscriptions(controller->GetCustomSubscriptions()))));
}

AdblockPrivateAddAllowedDomainFunction::
    AdblockPrivateAddAllowedDomainFunction() {}

AdblockPrivateAddAllowedDomainFunction::
    ~AdblockPrivateAddAllowedDomainFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateAddAllowedDomainFunction::Run() {
  std::unique_ptr<api::adblock_private::AddAllowedDomain::Params> params(
      api::adblock_private::AddAllowedDomain::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->AddAllowedDomain(params->domain);
  return RespondNow(NoArguments());
}

AdblockPrivateRemoveAllowedDomainFunction::
    AdblockPrivateRemoveAllowedDomainFunction() {}

AdblockPrivateRemoveAllowedDomainFunction::
    ~AdblockPrivateRemoveAllowedDomainFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateRemoveAllowedDomainFunction::Run() {
  std::unique_ptr<api::adblock_private::RemoveAllowedDomain::Params> params(
      api::adblock_private::RemoveAllowedDomain::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->RemoveAllowedDomain(params->domain);

  return RespondNow(NoArguments());
}

AdblockPrivateGetAllowedDomainsFunction::
    AdblockPrivateGetAllowedDomainsFunction() {}

AdblockPrivateGetAllowedDomainsFunction::
    ~AdblockPrivateGetAllowedDomainsFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetAllowedDomainsFunction::Run() {
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  return RespondNow(
      ArgumentList(api::adblock_private::GetAllowedDomains::Results::Create(
          controller->GetAllowedDomains())));
}

AdblockPrivateAddCustomFilterFunction::AdblockPrivateAddCustomFilterFunction() {
}

AdblockPrivateAddCustomFilterFunction::
    ~AdblockPrivateAddCustomFilterFunction() {}

ExtensionFunction::ResponseAction AdblockPrivateAddCustomFilterFunction::Run() {
  std::unique_ptr<api::adblock_private::AddCustomFilter::Params> params(
      api::adblock_private::AddCustomFilter::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->AddCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

AdblockPrivateRemoveCustomFilterFunction::
    AdblockPrivateRemoveCustomFilterFunction() {}

AdblockPrivateRemoveCustomFilterFunction::
    ~AdblockPrivateRemoveCustomFilterFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateRemoveCustomFilterFunction::Run() {
  std::unique_ptr<api::adblock_private::RemoveCustomFilter::Params> params(
      api::adblock_private::RemoveCustomFilter::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser_context());
  controller->RemoveCustomFilter(params->filter);
  return RespondNow(NoArguments());
}

AdblockPrivateGetSessionAllowedAdsCountFunction::
    AdblockPrivateGetSessionAllowedAdsCountFunction() {}

AdblockPrivateGetSessionAllowedAdsCountFunction::
    ~AdblockPrivateGetSessionAllowedAdsCountFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetSessionAllowedAdsCountFunction::Run() {
  auto* session_stats_ =
      adblock::SessionStatsFactory::GetForBrowserContext(browser_context());
  return RespondNow(ArgumentList(
      api::adblock_private::GetSessionAllowedAdsCount::Results::Create(
          CopySessionsStats(session_stats_->GetSessionAllowedAdsCount()))));
}

AdblockPrivateGetSessionBlockedAdsCountFunction::
    AdblockPrivateGetSessionBlockedAdsCountFunction() {}

AdblockPrivateGetSessionBlockedAdsCountFunction::
    ~AdblockPrivateGetSessionBlockedAdsCountFunction() {}

ExtensionFunction::ResponseAction
AdblockPrivateGetSessionBlockedAdsCountFunction::Run() {
  auto* session_stats_ =
      adblock::SessionStatsFactory::GetForBrowserContext(browser_context());
  return RespondNow(ArgumentList(
      api::adblock_private::GetSessionAllowedAdsCount::Results::Create(
          CopySessionsStats(session_stats_->GetSessionBlockedAdsCount()))));
}

}  // namespace api
}  // namespace extensions

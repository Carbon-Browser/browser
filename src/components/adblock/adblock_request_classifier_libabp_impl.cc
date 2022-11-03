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

#include "components/adblock/adblock_request_classifier_libabp_impl.h"

#include "base/task/post_task.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/adblock_content_type.h"
#include "components/adblock/adblock_utils.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"

namespace adblock {
namespace {

std::vector<GURL> GetSubscriptionsContainingFilter(
    AdblockPlus::IFilterEngine* engine,
    const AdblockPlus::Filter& filter) {
  std::vector<GURL> subscriptions;
  auto abp_subscriptions = engine->GetSubscriptionsFromFilter(filter);
  subscriptions.reserve(abp_subscriptions.size());
  std::transform(abp_subscriptions.begin(), abp_subscriptions.end(),
                 std::back_inserter(subscriptions),
                 [](const AdblockPlus::Subscription& subscription) {
                   return GURL{subscription.GetUrl()};
                 });

  return subscriptions;
}

bool HasMatchingFilterHelper(scoped_refptr<AdblockPlatformEmbedder> embedder,
                             const GURL& url,
                             ContentType content_type,
                             const GURL& parent_url,
                             const adblock::SiteKey& sitekey,
                             bool specific_only) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  DCHECK(embedder->GetFilterEngine());

  DLOG(INFO) << "[ABP] Check match for " << url.spec();
  auto filter = embedder->GetFilterEngine()->Matches(
      url.spec(), content_type, parent_url.spec(), sitekey.value(),
      specific_only);
  DLOG(INFO) << "[ABP] match found: " << (filter.IsValid() ? "true" : "false");
  if (filter.IsValid()) {
    DLOG(INFO) << "[ABP] matching filter: " << filter.GetRaw();
  }
  return filter.IsValid();
}
}  // namespace

AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::
    CheckFilterMatchResult(mojom::FilterMatchResult status,
                           const std::vector<GURL>& subscriptions)
    : status(status), subscriptions(subscriptions) {}
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::
    ~CheckFilterMatchResult() = default;
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::
    CheckFilterMatchResult(const CheckFilterMatchResult&) = default;
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult&
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::operator=(
    const CheckFilterMatchResult&) = default;
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::
    CheckFilterMatchResult(CheckFilterMatchResult&&) = default;
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult&
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult::operator=(
    CheckFilterMatchResult&&) = default;

AdblockRequestClassifierLibabpImpl::AdblockRequestClassifierLibabpImpl(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    std::unique_ptr<AdblockFrameHierarchyBuilder> frame_hierarchy_builder,
    AdblockElementHider* element_hider,
    AdblockController* controller,
    AdblockSitekeyStorage* sitekey_storage)
    : embedder_(std::move(embedder)),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)),
      element_hider_(element_hider),
      controller_(controller),
      sitekey_storage_(sitekey_storage),
      weak_ptr_factory_(this) {}
AdblockRequestClassifierLibabpImpl::~AdblockRequestClassifierLibabpImpl() =
    default;

void AdblockRequestClassifierLibabpImpl::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void AdblockRequestClassifierLibabpImpl::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

mojom::FilterMatchResult AdblockRequestClassifierLibabpImpl::ShouldBlockPopup(
    const GURL& opener,
    const GURL& url,
    content::RenderFrameHost* frame_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT1("adblockplus",
               "AdblockRequestClassifierLibabpImpl::ShouldBlockPopup", "url",
               url.spec());

  mojom::FilterMatchResult result = mojom::FilterMatchResult::kNoRule;

  if (!controller_->IsAdblockEnabled()) {
    result = mojom::FilterMatchResult::kDisabled;
    return result;
  }

  auto* engine = embedder_->GetFilterEngine();
  if (engine) {
    AdblockPlus::Filter filter = engine->Matches(
        url.spec(), ContentType::CONTENT_TYPE_POPUP, opener.spec());
    if (filter.IsValid()) {
      result = filter.GetType() == AdblockPlus::Filter::Type::TYPE_EXCEPTION
                   ? mojom::FilterMatchResult::kAllowRule
                   : mojom::FilterMatchResult::kBlockRule;
      if (result == mojom::FilterMatchResult::kBlockRule)
        LOG(INFO) << "[ABP] !!! Prevented loading of pop-up " << url.spec();
      else
        LOG(INFO) << "[ABP] Pop-up allowed (exception)";
      NotifyAdMatched(
          url, result, std::vector<GURL>{},
          AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_POPUP,
          frame_host, GetSubscriptionsContainingFilter(engine, filter));
      return result;
    }
  }

  result = mojom::FilterMatchResult::kNoRule;
  LOG(INFO) << "[ABP] Pop-up allowed (no filter)";
  return result;
}

void AdblockRequestClassifierLibabpImpl::CheckFilterMatchForWebSocket(
    const GURL& request_url,
    content::RenderFrameHost* render_frame_host,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(request_url.SchemeIsWSOrWSS());
  CheckFilterMatchImpl(
      request_url,
      AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_WEBSOCKET,
      render_frame_host->GetGlobalId(), std::move(callback));
}

void AdblockRequestClassifierLibabpImpl::CheckFilterMatch(
    const GURL& request_url,
    int32_t resource_type,
    int32_t process_id,
    int32_t render_frame_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!request_url.SchemeIsWSOrWSS())
      << "Use CheckFilterMatchForWebSocket()";
  if (resource_type ==
      static_cast<int32_t>(blink::mojom::ResourceType::kMainFrame)) {
    DLOG(INFO) << "[ABP] Main document. Passing it through: " << request_url;
    base::PostTask(
        FROM_HERE,
        // Without USER_BLOCKING priority -  in session restore case - posted
        // callbacks can take over a minute to trigger. This is why such high
        // priority is applied everywhere.
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        base::BindOnce(std::move(callback), mojom::FilterMatchResult::kNoRule));
    return;
  }

  const auto adblock_resource_type =
      utils::ConvertToAdblockResourceType(request_url, resource_type);
  DVLOG(3) << "[ABP] Mapped type=" << resource_type
           << " to adblock type=" << adblock_resource_type;
  content::RenderFrameHost* host =
      frame_hierarchy_builder_->FindRenderFrameHost(process_id,
                                                    render_frame_id);
  if (!host) {
    // We're unable to verify if the resource is allowed or blocked without a
    // frame hierarchy, so err on the side of safety and allow the load.
    // This happens for Ping-type requests, for reasons unknown.
    DLOG(INFO) << "Unable to build frame hierarchy for " << request_url
               << " \t: process id " << process_id << ", render frame id "
               << render_frame_id << ", adblock_resource_type "
               << adblock_resource_type << ": Allowing load";
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        base::BindOnce(std::move(callback), mojom::FilterMatchResult::kNoRule));
    return;
  }

  if (!controller_->IsAdblockEnabled()) {
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        base::BindOnce(std::move(callback),
                       mojom::FilterMatchResult::kDisabled));
    return;
  }

  CheckFilterMatchImpl(request_url, adblock_resource_type, host->GetGlobalId(),
                       std::move(callback));
}

void AdblockRequestClassifierLibabpImpl::CheckFilterMatchImpl(
    const GURL& request_url,
    AdblockPlus::IFilterEngine::ContentType adblock_resource_type,
    content::GlobalRenderFrameHostId frame_host_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!embedder_->GetFilterEngine()) {
    VLOG(1) << "[ABP] CheckFilterMatch no engine yet";
    embedder_->RunAfterFilterEngineCreated(base::BindOnce(
        &AdblockRequestClassifierLibabpImpl::CheckFilterMatchImpl,
        weak_ptr_factory_.GetWeakPtr(), request_url, adblock_resource_type,
        frame_host_id, std::move(callback)));
    return;
  }

  auto* host = content::RenderFrameHost::FromID(frame_host_id);
  if (!host) {
    // Host has died, likely because this is a deferred execution. It does not
    // matter anymore whether the resource is blocked, the page is gone.
    base::PostTask(
        FROM_HERE,
        {content::BrowserThread::UI, base::TaskPriority::USER_BLOCKING},
        base::BindOnce(std::move(callback), mojom::FilterMatchResult::kNoRule));
    return;
  }
  const std::vector<GURL> referrers_chain =
      frame_hierarchy_builder_->BuildFrameHierarchy(host);

  DLOG(INFO) << "[ABP] Got " << referrers_chain.size() << " referrers for "
             << request_url.spec();

  auto site_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(referrers_chain);
  SiteKey site_key;
  if (site_key_pair.has_value()) {
    site_key = site_key_pair->second;
    DLOG(INFO) << "[ABP] Found site key: " << site_key.value()
               << " for url: " << site_key_pair->first;
  }

  embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          &AdblockRequestClassifierLibabpImpl::CheckFilterMatchInternal,
          embedder_, request_url, adblock_resource_type, referrers_chain,
          site_key),
      base::BindOnce(
          &AdblockRequestClassifierLibabpImpl::OnCheckFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), request_url, adblock_resource_type,
          referrers_chain, frame_host_id, std::move(callback)));
}

// static
AdblockRequestClassifierLibabpImpl::CheckFilterMatchResult
AdblockRequestClassifierLibabpImpl::CheckFilterMatchInternal(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    const GURL& request_url,
    AdblockPlus::IFilterEngine::ContentType adblock_resource_type,
    const std::vector<GURL>& referrers,
    const SiteKey& sitekey) {
  TRACE_EVENT1("adblockplus",
               "AdblockRequestClassifierLibabpImpl::CheckFilterMatchInternal",
               "url", request_url.spec());
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());

  VLOG(1) << "[ABP] CheckFilterMatchInternal start";

  std::string url = request_url.spec();
  DLOG(INFO) << "[ABP] Loading url " << url;

  auto* filter_engine = embedder->GetFilterEngine();
  DCHECK(filter_engine);

  std::vector<std::string> referrers_chain;
  for (auto& referer : referrers)
    referrers_chain.push_back(referer.spec());

  bool specific_only = false;
  if (!referrers_chain.empty()) {
    specific_only = filter_engine->IsContentAllowlisted(
        url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
        referrers_chain, sitekey.value());
    if (specific_only) {
      DLOG(INFO) << "[ABP] Found genericblock match for url: " << url;
    }
  }

  std::string doc_url = referrers_chain.empty() ? "" : referrers_chain.front();
  AdblockPlus::Filter filter = filter_engine->Matches(
      url, adblock_resource_type, doc_url, sitekey.value(), specific_only);
  DLOG(INFO) << "[ABP] Found filter: "
             << (filter.IsValid() ? filter.GetRaw() : "NULL");
  if (filter.IsValid() &&
      filter.GetType() != AdblockPlus::Filter::Type::TYPE_EXCEPTION) {
    auto sources = GetSubscriptionsContainingFilter(filter_engine, filter);

    if (filter_engine->IsContentAllowlisted(
            url, AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT,
            referrers_chain, sitekey.value())) {
      LOG(INFO) << "[ABP] Document allowed " << url << " subscription "
                << (sources.empty() ? "unknown" : sources.front().spec());
      return CheckFilterMatchResult(mojom::FilterMatchResult::kAllowRule,
                                    sources);
    }

    LOG(INFO) << "[ABP] !!! Prevented loading " << url << " subscription "
              << (sources.empty() ? "unknown" : sources.front().spec());

    return CheckFilterMatchResult(mojom::FilterMatchResult::kBlockRule,
                                  sources);
  }

  if (filter.IsValid() &&
      filter.GetType() == AdblockPlus::Filter::Type::TYPE_EXCEPTION) {
    auto sources = GetSubscriptionsContainingFilter(filter_engine, filter);
    LOG(INFO) << "[ABP] Allowing loading " << url << " subscription "
              << (sources.empty() ? "unknown" : sources.front().spec());
    return CheckFilterMatchResult(mojom::FilterMatchResult::kAllowRule,
                                  sources);
  }

  return CheckFilterMatchResult(mojom::FilterMatchResult::kNoRule, {});
}

void AdblockRequestClassifierLibabpImpl::OnCheckFilterMatchComplete(
    const GURL& request_url,
    AdblockPlus::IFilterEngine::ContentType adblock_resource_type,
    const std::vector<GURL>& referrers,
    content::GlobalRenderFrameHostId render_frame_host_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback,
    const CheckFilterMatchResult& result) {
  // Notify |callback| as soon as we know whether we should block, as this
  // unblocks loading of network resources.
  std::move(callback).Run(result.status);
  auto* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (render_frame_host &&
      result.status == mojom::FilterMatchResult::kBlockRule) {
    // To avoid blank spaces left by blocked resources, collapse them.
    // TODO move element hiding of blocked elements away from this class,
    // perhaps to AdblockMojoInterfaceImpl. RequestClassifier should classify
    // requests, not execute hiding while assuming the caller will act upon
    // kBlockRule.
    if (element_hider_->IsElementTypeHideable(
            static_cast<ContentType>(adblock_resource_type)))
      element_hider_->HideBlockedElement(request_url, render_frame_host);
  }
  // Only notify the UI if we explicitly blocked or allowed the resource, not
  // when there was NO_RULE.
  if (result.status == mojom::FilterMatchResult::kAllowRule ||
      result.status == mojom::FilterMatchResult::kBlockRule) {
    NotifyAdMatched(request_url, result.status, referrers,
                    adblock_resource_type, render_frame_host,
                    result.subscriptions);
  }
}

void AdblockRequestClassifierLibabpImpl::HasMatchingFilter(
    const GURL& url,
    ContentType content_type,
    const GURL& parent_url,
    const SiteKey& sitekey,
    bool specific_only,
    base::OnceCallback<void(bool)> result) {
  if (!embedder_->GetFilterEngine()) {
    VLOG(1) << "[ABP] HasMatchingFilter no engine yet";
    embedder_->RunAfterFilterEngineCreated(
        base::BindOnce(&AdblockRequestClassifierLibabpImpl::HasMatchingFilter,
                       weak_ptr_factory_.GetWeakPtr(), url, content_type,
                       parent_url, sitekey, specific_only, std::move(result)));
    return;
  }

  embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&HasMatchingFilterHelper, embedder_, url, content_type,
                     parent_url, sitekey, specific_only),
      std::move(result));
}

void AdblockRequestClassifierLibabpImpl::NotifyAdMatched(
    const GURL& url,
    mojom::FilterMatchResult result,
    const std::vector<GURL>& parent_frame_urls,
    AdblockPlus::IFilterEngine::ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const std::vector<GURL>& subscriptions) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DLOG(INFO) << "[ABP] NotifyAdMatched() called for " << url;

  for (auto& observer : observers_) {
    observer.OnAdMatched(url, result, parent_frame_urls,
                         static_cast<ContentType>(content_type),
                         render_frame_host, subscriptions);
  }
}

}  // namespace adblock

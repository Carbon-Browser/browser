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

#include "components/adblock/content/browser/resource_classification_runner_impl.h"

#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/content/browser/adblock_content_utils.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/sitekey.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace adblock {

using ClassificationDecision =
    ResourceClassifier::ClassificationResult::Decision;

namespace {

absl::optional<GURL> HasRewriteHelper(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) {
  return subscription_collection->GetRewriteUrl(request_url, frame_hierarchy);
}

}  // namespace

ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::
    CheckResourceFilterMatchResult(mojom::FilterMatchResult status,
                                   const GURL& subscription)
    : status(status), subscription(subscription) {}
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::
    ~CheckResourceFilterMatchResult() = default;
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::
    CheckResourceFilterMatchResult(const CheckResourceFilterMatchResult&) =
        default;
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult&
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::operator=(
    const CheckResourceFilterMatchResult&) = default;
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::
    CheckResourceFilterMatchResult(CheckResourceFilterMatchResult&&) = default;
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult&
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult::operator=(
    CheckResourceFilterMatchResult&&) = default;

ResourceClassificationRunnerImpl::ResourceClassificationRunnerImpl(
    scoped_refptr<ResourceClassifier> resource_classifier,
    std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder,
    SitekeyStorage* sitekey_storage)
    : resource_classifier_(std::move(resource_classifier)),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)),
      sitekey_storage_(sitekey_storage),
      weak_ptr_factory_(this) {}

ResourceClassificationRunnerImpl::~ResourceClassificationRunnerImpl() = default;

void ResourceClassificationRunnerImpl::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void ResourceClassificationRunnerImpl::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

mojom::FilterMatchResult ResourceClassificationRunnerImpl::ShouldBlockPopup(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& opener,
    const GURL& url,
    content::RenderFrameHost* frame_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(frame_host);
  TRACE_EVENT1("eyeo", "ResourceClassificationRunnerImpl::ShouldBlockPopup",
               "url", url.spec());

  auto sitekey = sitekey_storage_->FindSiteKeyForAnyUrl({url, opener});

  auto classification_result = resource_classifier_->ClassifyPopup(
      *subscription_collection, url, opener,
      sitekey ? sitekey->second : SiteKey());
  if (classification_result.decision == ClassificationDecision::Ignored) {
    return mojom::FilterMatchResult::kNoRule;
  }
  const mojom::FilterMatchResult result =
      classification_result.decision == ClassificationDecision::Allowed
          ? mojom::FilterMatchResult::kAllowRule
          : mojom::FilterMatchResult::kBlockRule;
  if (result == mojom::FilterMatchResult::kBlockRule)
    LOG(INFO) << "[eyeo] !!! Prevented loading of pop-up " << url.spec();
  else
    LOG(INFO) << "[eyeo] Pop-up allowed (exception)";
  for (auto& observer : observers_) {
    observer.OnPopupMatched(url, result, opener, frame_host,
                            classification_result.decisive_subscription);
  }
  return result;
}

void ResourceClassificationRunnerImpl::CheckRequestFilterMatchForWebSocket(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    content::RenderFrameHost* render_frame_host,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(request_url.SchemeIsWSOrWSS());
  CheckRequestFilterMatchImpl(
      std::move(subscription_collection), request_url, ContentType::Websocket,
      render_frame_host->GetGlobalId(), std::move(callback));
}

absl::optional<GURL> ResourceClassificationRunnerImpl::CheckDocumentAllowlisted(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url) {
  return subscription_collection->FindBySpecialFilter(
      SpecialFilterType::Document, request_url, std::vector<GURL>(), SiteKey());
}

void ResourceClassificationRunnerImpl::ProcessDocumentAllowlistedResponse(
    const GURL& request_url,
    int32_t process_id,
    int32_t render_frame_id,
    absl::optional<GURL> allowing_subscription) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::RenderFrameHost* host =
      content::RenderFrameHost::FromID(process_id, render_frame_id);
  if (allowing_subscription && host) {
    VLOG(1) << "[eyeo] Calling OnPageAllowed() for " << request_url;
    for (auto& observer : observers_) {
      observer.OnPageAllowed(request_url, host, allowing_subscription.value());
    }
  }
}

void ResourceClassificationRunnerImpl::CheckRequestFilterMatch(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    int32_t resource_type,
    int32_t process_id,
    int32_t render_frame_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!request_url.SchemeIsWSOrWSS())
      << "Use CheckRequestFilterMatchForWebSocket()";
  if (resource_type ==
      static_cast<int32_t>(blink::mojom::ResourceType::kMainFrame)) {
    // We pass main frame no matter what
    DVLOG(1) << "[eyeo] Main document. Passing it through: " << request_url;
    std::move(callback).Run(mojom::FilterMatchResult::kNoRule);
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {},
        base::BindOnce(
            &ResourceClassificationRunnerImpl::CheckDocumentAllowlisted,
            std::move(subscription_collection), request_url),
        base::BindOnce(&ResourceClassificationRunnerImpl::
                           ProcessDocumentAllowlistedResponse,
                       weak_ptr_factory_.GetWeakPtr(), request_url, process_id,
                       render_frame_id));
    return;
  }

  const ContentType adblock_resource_type =
      utils::ConvertToAdblockResourceType(request_url, resource_type);
  DVLOG(3) << "[eyeo] Mapped type=" << resource_type
           << " to adblock type=" << adblock_resource_type;
  content::RenderFrameHost* host =
      frame_hierarchy_builder_->FindRenderFrameHost(process_id,
                                                    render_frame_id);
  if (!host) {
    // We're unable to verify if the resource is allowed or blocked without a
    // frame hierarchy, so err on the side of safety and allow the load.
    // This happens for Ping-type requests, for reasons unknown.
    VLOG(1) << "[eyeo] Unable to build frame hierarchy for " << request_url
            << " \t: process id " << process_id << ", render frame id "
            << render_frame_id << ", adblock_resource_type "
            << adblock_resource_type << ": Allowing load";
    std::move(callback).Run(mojom::FilterMatchResult::kNoRule);
    return;
  }

  CheckRequestFilterMatchImpl(std::move(subscription_collection), request_url,
                              adblock_resource_type, host->GetGlobalId(),
                              std::move(callback));
}

void ResourceClassificationRunnerImpl::CheckRequestFilterMatchImpl(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    ContentType adblock_resource_type,
    content::GlobalRenderFrameHostId frame_host_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] CheckRequestFilterMatchImpl for " << request_url.spec();

  auto* host = content::RenderFrameHost::FromID(frame_host_id);
  if (!host) {
    // Host has died, likely because this is a deferred execution. It does not
    // matter anymore whether the resource is blocked, the page is gone.
    std::move(callback).Run(mojom::FilterMatchResult::kNoRule);
    return;
  }
  const std::vector<GURL> frame_hierarchy_chain =
      frame_hierarchy_builder_->BuildFrameHierarchy(host);

  DVLOG(1) << "[eyeo] Got " << frame_hierarchy_chain.size()
           << " frame_hierarchy for " << request_url.spec();

  auto site_key_pair =
      sitekey_storage_->FindSiteKeyForAnyUrl(frame_hierarchy_chain);
  SiteKey site_key;
  if (site_key_pair.has_value()) {
    site_key = site_key_pair->second;
    DVLOG(1) << "[eyeo] Found site key: " << site_key.value()
             << " for url: " << site_key_pair->first;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(
          &ResourceClassificationRunnerImpl::CheckRequestFilterMatchInternal,
          resource_classifier_, std::move(subscription_collection), request_url,
          frame_hierarchy_chain, adblock_resource_type, site_key),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), request_url, frame_hierarchy_chain,
          adblock_resource_type, frame_host_id, std::move(callback)));
}

// static
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult
ResourceClassificationRunnerImpl::CheckRequestFilterMatchInternal(
    const scoped_refptr<ResourceClassifier>& resource_classifier,
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType adblock_resource_type,
    const SiteKey& sitekey) {
  TRACE_EVENT1("eyeo",
               "ResourceClassificationRunnerImpl::"
               "CheckRequestFilterMatchInternal",
               "url", request_url.spec());

  DVLOG(1) << "[eyeo] CheckRequestFilterMatchInternal start";

  auto classification_result = resource_classifier->ClassifyRequest(
      *subscription_collection, request_url, frame_hierarchy,
      adblock_resource_type, sitekey);

  if (classification_result.decision == ClassificationDecision::Allowed) {
    VLOG(1) << "[eyeo] Document allowed due to allowing filter " << request_url;
    return CheckResourceFilterMatchResult(
        mojom::FilterMatchResult::kAllowRule,
        {classification_result.decisive_subscription});
  }

  if (classification_result.decision == ClassificationDecision::Blocked) {
    VLOG(1) << "[eyeo] Document blocked " << request_url;
    return CheckResourceFilterMatchResult(
        mojom::FilterMatchResult::kBlockRule,
        {classification_result.decisive_subscription});
  }

  return CheckResourceFilterMatchResult(mojom::FilterMatchResult::kNoRule, {});
}

void ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType adblock_resource_type,
    content::GlobalRenderFrameHostId render_frame_host_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback,
    const CheckResourceFilterMatchResult& result) {
  // Notify |callback| as soon as we know whether we should block, as this
  // unblocks loading of network resources.
  std::move(callback).Run(result.status);
  auto* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);
  if (render_frame_host) {
    // Only notify the UI if we explicitly blocked or allowed the resource, not
    // when there was NO_RULE.
    if (result.status == mojom::FilterMatchResult::kAllowRule ||
        result.status == mojom::FilterMatchResult::kBlockRule) {
      NotifyAdMatched(request_url, result.status, frame_hierarchy,
                      adblock_resource_type, render_frame_host,
                      result.subscription);
    }
  }
}

void ResourceClassificationRunnerImpl::NotifyAdMatched(
    const GURL& url,
    mojom::FilterMatchResult result,
    const std::vector<GURL>& parent_frame_urls,
    ContentType content_type,
    content::RenderFrameHost* render_frame_host,
    const GURL& subscription) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  VLOG(1) << "[eyeo] NotifyAdMatched() called for " << url;

  for (auto& observer : observers_) {
    observer.OnAdMatched(url, result, parent_frame_urls,
                         static_cast<ContentType>(content_type),
                         render_frame_host, {subscription});
  }
}

void ResourceClassificationRunnerImpl::CheckResponseFilterMatch(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& response_url,
    int32_t process_id,
    int32_t render_frame_id,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "[eyeo] CheckResponseFilterMatch for " << response_url.spec();
  content::RenderFrameHost* host =
      frame_hierarchy_builder_->FindRenderFrameHost(process_id,
                                                    render_frame_id);
  if (!host) {
    // This request is likely dead, since there's no associated RenderFrameHost.
    std::move(callback).Run(mojom::FilterMatchResult::kNoRule);
    return;
  }

  auto adblock_resource_type = utils::DetectResourceType(response_url);
  auto frame_hierarchy = frame_hierarchy_builder_->BuildFrameHierarchy(host);
  // ResponseFilterMatch might take a while, let it run in the background.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(
          &ResourceClassificationRunnerImpl::CheckResponseFilterMatchInternal,
          resource_classifier_, std::move(subscription_collection),
          response_url, frame_hierarchy, adblock_resource_type, headers),
      base::BindOnce(
          &ResourceClassificationRunnerImpl::OnCheckResourceFilterMatchComplete,
          weak_ptr_factory_.GetWeakPtr(), response_url, frame_hierarchy,
          adblock_resource_type, host->GetGlobalId(), std::move(callback)));
}

// static
ResourceClassificationRunnerImpl::CheckResourceFilterMatchResult
ResourceClassificationRunnerImpl::CheckResponseFilterMatchInternal(
    const scoped_refptr<ResourceClassifier>& resource_classifier,
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL response_url,
    const std::vector<GURL> frame_hierarchy,
    ContentType adblock_resource_type,
    const scoped_refptr<net::HttpResponseHeaders> response_headers) {
  auto classification_result = resource_classifier->ClassifyResponse(
      *subscription_collection, response_url, frame_hierarchy,
      adblock_resource_type, response_headers);

  if (classification_result.decision == ClassificationDecision::Allowed) {
    VLOG(1) << "[eyeo] Document allowed due to allowing filter "
            << response_url;
    return CheckResourceFilterMatchResult(
        mojom::FilterMatchResult::kAllowRule,
        {classification_result.decisive_subscription});
  }

  if (classification_result.decision == ClassificationDecision::Blocked) {
    VLOG(1) << "[eyeo] Document blocked " << response_url;
    return CheckResourceFilterMatchResult(
        mojom::FilterMatchResult::kBlockRule,
        {classification_result.decisive_subscription});
  }

  return CheckResourceFilterMatchResult(mojom::FilterMatchResult::kNoRule, {});
}

void ResourceClassificationRunnerImpl::CheckRewriteFilterMatch(
    std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL& request_url,
    int32_t process_id,
    int32_t render_frame_id,
    base::OnceCallback<void(const absl::optional<GURL>&)> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << "[eyeo] CheckRewriteFilterMatch for " << request_url.spec();

  content::RenderFrameHost* host =
      frame_hierarchy_builder_->FindRenderFrameHost(process_id,
                                                    render_frame_id);
  if (!host) {
    std::move(callback).Run(absl::optional<GURL>{});
    return;
  }

  const std::vector<GURL> frame_hierarchy =
      frame_hierarchy_builder_->BuildFrameHierarchy(host);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {},
      base::BindOnce(&HasRewriteHelper, std::move(subscription_collection),
                     request_url, frame_hierarchy),
      std::move(callback));
}

}  // namespace adblock

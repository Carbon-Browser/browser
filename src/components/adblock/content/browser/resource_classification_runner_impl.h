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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_IMPL_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_IMPL_H_

#include <vector>

#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/classifier/resource_classifier.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/sitekey_storage.h"
#include "content/public/browser/global_routing_id.h"

namespace adblock {

class ResourceClassificationRunnerImpl final
    : public ResourceClassificationRunner {
 public:
  ResourceClassificationRunnerImpl(
      scoped_refptr<ResourceClassifier> resource_classifier,
      std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder,
      SitekeyStorage* sitekey_storage);
  ~ResourceClassificationRunnerImpl() final;

  void AddObserver(Observer* observer) final;
  void RemoveObserver(Observer* observer) final;

  // Performs a *synchronous* check, this can block the UI for a while!
  mojom::FilterMatchResult ShouldBlockPopup(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& opener,
      const GURL& popup_url,
      content::RenderFrameHost* render_frame_host) final;
  void CheckRequestFilterMatch(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url,
      int32_t resource_type,
      int32_t process_id,
      int32_t render_frame_id,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) final;
  void CheckRequestFilterMatchForWebSocket(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url,
      content::RenderFrameHost* render_frame_host,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) final;
  void CheckResponseFilterMatch(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& response_url,
      int32_t process_id,
      int32_t render_frame_id,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) final;
  void CheckRewriteFilterMatch(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url,
      int32_t process_id,
      int32_t render_frame_id,
      base::OnceCallback<void(const absl::optional<GURL>&)> result) final;

 private:
  void CheckRequestFilterMatchImpl(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url,
      ContentType adblock_type,
      content::GlobalRenderFrameHostId frame_host_id,
      mojom::AdblockInterface::CheckFilterMatchCallback cb);

  struct CheckResourceFilterMatchResult {
    CheckResourceFilterMatchResult(mojom::FilterMatchResult status,
                                   const GURL& subscription);
    ~CheckResourceFilterMatchResult();
    CheckResourceFilterMatchResult(const CheckResourceFilterMatchResult&);
    CheckResourceFilterMatchResult& operator=(
        const CheckResourceFilterMatchResult&);
    CheckResourceFilterMatchResult(CheckResourceFilterMatchResult&&);
    CheckResourceFilterMatchResult& operator=(CheckResourceFilterMatchResult&&);

    mojom::FilterMatchResult status;
    GURL subscription;
  };

  static CheckResourceFilterMatchResult CheckRequestFilterMatchInternal(
      const scoped_refptr<ResourceClassifier>& resource_classifier,
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType adblock_resource_type,
      const SiteKey& sitekey);

  void OnCheckResourceFilterMatchComplete(
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType adblock_resource_type,
      content::GlobalRenderFrameHostId render_frame_host_id,
      mojom::AdblockInterface::CheckFilterMatchCallback callback,
      const CheckResourceFilterMatchResult& result);

  static CheckResourceFilterMatchResult CheckResponseFilterMatchInternal(
      const scoped_refptr<ResourceClassifier>& resource_classifier,
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL response_url,
      const std::vector<GURL> frame_hierarchy,
      ContentType adblock_resource_type,
      const scoped_refptr<net::HttpResponseHeaders> response_headers);

  void NotifyAdMatched(const GURL& url,
                       mojom::FilterMatchResult result,
                       const std::vector<GURL>& parent_frame_urls,
                       ContentType content_type,
                       content::RenderFrameHost* render_frame_host,
                       const GURL& subscription);

  void PostFilterMatchCallbackToUI(
      mojom::AdblockInterface::CheckFilterMatchCallback callback,
      mojom::FilterMatchResult result);

  void PostRewriteCallbackToUI(
      base::OnceCallback<void(const absl::optional<GURL>&)> callback,
      absl::optional<GURL> url);

  static absl::optional<GURL> CheckDocumentAllowlisted(
      std::unique_ptr<SubscriptionCollection> subscription_collection,
      const GURL& request_url);

  void ProcessDocumentAllowlistedResponse(
      const GURL& request_url,
      int32_t process_id,
      int32_t render_frame_id,
      absl::optional<GURL> allowing_subscription);

  SEQUENCE_CHECKER(sequence_checker_);
  scoped_refptr<ResourceClassifier> resource_classifier_;
  std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder_;
  SitekeyStorage* sitekey_storage_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<ResourceClassificationRunnerImpl> weak_ptr_factory_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_RESOURCE_CLASSIFICATION_RUNNER_IMPL_H_

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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_LIBABP_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_LIBABP_IMPL_H_

#include <vector>

#include "components/adblock/adblock_controller.h"
#include "components/adblock/adblock_element_hider.h"
#include "components/adblock/adblock_frame_hierarchy_builder.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_request_classifier.h"
#include "components/adblock/adblock_sitekey_storage.h"
#include "content/public/browser/global_routing_id.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

namespace adblock {

class AdblockRequestClassifierLibabpImpl final : public AdblockRequestClassifier {
 public:
  AdblockRequestClassifierLibabpImpl(
      scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
      std::unique_ptr<AdblockFrameHierarchyBuilder> frame_hierarchy_builder,
      AdblockElementHider* element_hider,
      AdblockController* controller,
      AdblockSitekeyStorage* sitekey_storage);
  ~AdblockRequestClassifierLibabpImpl() final;

  void AddObserver(Observer* observer) final;
  void RemoveObserver(Observer* observer) final;

  // Performs a *synchronous* check, this can block the UI for a while!
  mojom::FilterMatchResult ShouldBlockPopup(
      const GURL& opener,
      const GURL& popup_url,
      content::RenderFrameHost* render_frame_host) final;

  void CheckFilterMatch(
      const GURL& request_url,
      int32_t resource_type,
      int32_t process_id,
      int32_t render_frame_id,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) final;
  void CheckFilterMatchForWebSocket(
      const GURL& request_url,
      content::RenderFrameHost* render_frame_host,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) final;
  void HasMatchingFilter(const GURL& url,
                         ContentType content_type,
                         const GURL& parent_url,
                         const SiteKey& sitekey,
                         bool specific_only,
                         base::OnceCallback<void(bool)> result) final;

 private:
  void CheckFilterMatchImpl(
      const GURL& request_url,
      AdblockPlus::IFilterEngine::ContentType adblock_type,
      content::GlobalRenderFrameHostId frame_host_id,
      mojom::AdblockInterface::CheckFilterMatchCallback cb);

  struct CheckFilterMatchResult {
    CheckFilterMatchResult(mojom::FilterMatchResult status,
                           const std::vector<GURL>& subscription);
    ~CheckFilterMatchResult();
    CheckFilterMatchResult(const CheckFilterMatchResult&);
    CheckFilterMatchResult& operator=(const CheckFilterMatchResult&);
    CheckFilterMatchResult(CheckFilterMatchResult&&);
    CheckFilterMatchResult& operator=(CheckFilterMatchResult&&);

    mojom::FilterMatchResult status;
    std::vector<GURL> subscriptions;
  };

  static CheckFilterMatchResult CheckFilterMatchInternal(
      scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
      const GURL& request_url,
      AdblockPlus::IFilterEngine::ContentType adblock_resource_type,
      const std::vector<GURL>& referrers,
      const SiteKey& sitekey);

  void OnCheckFilterMatchComplete(
      const GURL& request_url,
      AdblockPlus::IFilterEngine::ContentType adblock_resource_type,
      const std::vector<GURL>& referrers,
      content::GlobalRenderFrameHostId render_frame_host_id,
      mojom::AdblockInterface::CheckFilterMatchCallback callback,
      const CheckFilterMatchResult& result);

  void NotifyAdMatched(const GURL& url,
                       mojom::FilterMatchResult result,
                       const std::vector<GURL>& parent_frame_urls,
                       AdblockPlus::IFilterEngine::ContentType content_type,
                       content::RenderFrameHost* render_frame_host,
                       const std::vector<GURL>& subscriptions);

  SEQUENCE_CHECKER(sequence_checker_);
  scoped_refptr<adblock::AdblockPlatformEmbedder> embedder_;
  std::unique_ptr<AdblockFrameHierarchyBuilder> frame_hierarchy_builder_;
  AdblockElementHider* element_hider_;
  AdblockController* controller_;
  AdblockSitekeyStorage* sitekey_storage_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AdblockRequestClassifierLibabpImpl> weak_ptr_factory_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_LIBABP_IMPL_H_

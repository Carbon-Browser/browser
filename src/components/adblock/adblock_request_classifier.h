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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_H_

#include <vector>

#include "base/observer_list.h"
#include "components/adblock/adblock.mojom.h"
#include "components/adblock/adblock_content_type.h"
#include "components/adblock/sitekey.h"
#include "components/keyed_service/core/keyed_service.h"

#include "url/gurl.h"

namespace content {
class RenderFrameHost;
}
namespace adblock {

/**
 * @brief Declares whether network requests should be blocked or allowed to
 * load by comparing their URLs and other metadata against filters defined
 * in active subscriptions.
 * Lives in the UI thread.
 */
class AdblockRequestClassifier : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Will only be called when |match_result| is BLOCK_RULE or ALLOW_RULE.
    virtual void OnAdMatched(const GURL& url,
                             mojom::FilterMatchResult match_result,
                             const std::vector<GURL>& parent_frame_urls,
                             ContentType content_type,
                             content::RenderFrameHost* render_frame_host,
                             const std::vector<GURL>& subscriptions) = 0;
  };
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Performs a *synchronous* check, this can block the UI for a while!
  virtual mojom::FilterMatchResult ShouldBlockPopup(
      const GURL& opener,
      const GURL& popup_url,
      content::RenderFrameHost* render_frame_host) = 0;

  virtual void CheckFilterMatch(
      const GURL& request_url,
      int32_t resource_type,
      int32_t process_id,
      int32_t render_frame_id,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) = 0;
  virtual void CheckFilterMatchForWebSocket(
      const GURL& request_url,
      content::RenderFrameHost* render_frame_host,
      mojom::AdblockInterface::CheckFilterMatchCallback callback) = 0;
  virtual void HasMatchingFilter(const GURL& url,
                                 ContentType content_type,
                                 const GURL& parent_url,
                                 const SiteKey& sitekey,
                                 bool specific_only,
                                 base::OnceCallback<void(bool)> result) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_H_

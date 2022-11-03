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

#ifndef CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_
#define CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_

#include <string>

#include "base/android/jni_weak_ref.h"
#include "components/adblock/adblock_controller.h"
#include "components/adblock/adblock_request_classifier.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace adblock {

class AdblockJNI : public AdblockRequestClassifier::Observer,
                   public AdblockController::Observer,
                   public KeyedService {
 public:
  AdblockJNI(AdblockRequestClassifier* classifier,
             AdblockController* controller);
  ~AdblockJNI() override;

  void Bind(JavaObjectWeakGlobalRef weak_java_controller);
  void SetHasAdblockCountersObservers(bool has_observers);

  // AdblockRequestClassifier::Observer:
  void OnAdMatched(const GURL& url,
                   mojom::FilterMatchResult result,
                   const std::vector<GURL>& parent_frame_urls,
                   ContentType content_type,
                   content::RenderFrameHost* render_frame_host,
                   const std::vector<GURL>& subscriptions) override;

  // AdblockController::Observer:
  void OnRecommendedSubscriptionsAvailable(
      const std::vector<Subscription>& recommended) override;
  void OnSubscriptionUpdated(const GURL& url) override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  AdblockRequestClassifier* classifier_;
  AdblockController* controller_;
  bool has_counters_observers_{false};
  JavaObjectWeakGlobalRef weak_java_controller_;
};

}  // namespace adblock

#endif  // CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_

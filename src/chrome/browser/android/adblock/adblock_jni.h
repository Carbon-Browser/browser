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

#ifndef CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_
#define CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_

#include <string>

#include "base/android/jni_weak_ref.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace adblock {

class AdblockJNI : public ResourceClassificationRunner::Observer,
                   public AdblockController::Observer,
                   public KeyedService {
 public:
  AdblockJNI(ResourceClassificationRunner* classification_runner,
             AdblockController* controller);
  ~AdblockJNI() override;

  void Bind(JavaObjectWeakGlobalRef weak_java_controller);
  void SetHasAdblockCountersObservers(bool has_observers);

  // ResourceClassificationRunner::Observer:
  void OnAdMatched(const GURL& url,
                   mojom::FilterMatchResult result,
                   const std::vector<GURL>& parent_frame_urls,
                   ContentType content_type,
                   content::RenderFrameHost* render_frame_host,
                   const GURL& subscription) override;
  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription) final;
  void OnPopupMatched(const GURL& url,
                      mojom::FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription) override;

  // AdblockController::Observer:
  void OnSubscriptionUpdated(const GURL& url) override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  ResourceClassificationRunner* classification_runner_;
  AdblockController* controller_;
  bool has_counters_observers_{false};
  JavaObjectWeakGlobalRef weak_java_controller_;
};

}  // namespace adblock

#endif  // CHROME_BROWSER_ANDROID_ADBLOCK_ADBLOCK_JNI_H_

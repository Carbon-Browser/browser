// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PRELOADING_PRELOADING_DATA_IMPL_H_
#define CONTENT_BROWSER_PRELOADING_PRELOADING_DATA_IMPL_H_

#include <memory>
#include <vector>

#include "content/public/browser/preloading_data.h"

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace content {

class PreloadingAttemptImpl;
class PreloadingPrediction;

// The scope of current preloading logging is only limited to the same
// WebContents navigations. If the predicted URL is opened in a new tab we lose
// the data corresponding to the navigation in different WebContents.
// TODO(crbug.com/1332123): Expand PreloadingData scope to consider multiple
// WebContent navigations.
class CONTENT_EXPORT PreloadingDataImpl
    : public PreloadingData,
      public WebContentsUserData<PreloadingDataImpl>,
      public WebContentsObserver {
 public:
  ~PreloadingDataImpl() override;

  static PreloadingDataImpl* GetOrCreateForWebContents(
      WebContents* web_contents);

  // Disallow copy and assign.
  PreloadingDataImpl(const PreloadingDataImpl& other) = delete;
  PreloadingDataImpl& operator=(const PreloadingDataImpl& other) = delete;

  // PreloadingDataImpl implementation:
  PreloadingAttempt* AddPreloadingAttempt(
      PreloadingPredictor predictor,
      PreloadingType preloading_type,
      PreloadingURLMatchCallback url_match_predicate) override;
  void AddPreloadingPrediction(
      PreloadingPredictor predictor,
      int64_t confidence,
      PreloadingURLMatchCallback url_match_predicate) override;

  // WebContentsObserver override.
  void DidStartNavigation(NavigationHandle* navigation_handle) override;
  void PrimaryPageChanged(Page& page) override;
  void WebContentsDestroyed() override;

 private:
  explicit PreloadingDataImpl(WebContents* web_contents);
  friend class WebContentsUserData<PreloadingDataImpl>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  void RecordUKMForPreloadingAttempts(ukm::SourceId navigated_page_source_id);
  void RecordUKMForPreloadingPredictions(
      ukm::SourceId navigated_page_source_id);
  void SetIsAccurateTriggeringAndPrediction(const GURL& navigated_url);

  // Stores all the preloading attempts that are happening for the next
  // navigation until the navigation takes place.
  std::vector<std::unique_ptr<PreloadingAttemptImpl>> preloading_attempts_;

  // Stores all the preloading predictions that are happening for the next
  // navigation until the navigation takes place.
  std::vector<std::unique_ptr<PreloadingPrediction>> preloading_predictions_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_PRELOADING_PRELOADING_DATA_IMPL_H_

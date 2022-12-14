// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ABORTS_PAGE_LOAD_METRICS_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ABORTS_PAGE_LOAD_METRICS_OBSERVER_H_

#include "components/page_load_metrics/browser/page_load_metrics_observer.h"

namespace internal {

extern const char kHistogramAbortForwardBackBeforeCommit[];
extern const char kHistogramAbortReloadBeforeCommit[];
extern const char kHistogramAbortNewNavigationBeforeCommit[];
extern const char kHistogramAbortStopBeforeCommit[];
extern const char kHistogramAbortCloseBeforeCommit[];
extern const char kHistogramAbortBackgroundBeforeCommit[];
extern const char kHistogramAbortOtherBeforeCommit[];
extern const char kHistogramAbortForwardBackBeforePaint[];
extern const char kHistogramAbortReloadBeforePaint[];
extern const char kHistogramAbortNewNavigationBeforePaint[];
extern const char kHistogramAbortStopBeforePaint[];
extern const char kHistogramAbortCloseBeforePaint[];
extern const char kHistogramAbortBackgroundBeforePaint[];

}  // namespace internal

class AbortsPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  AbortsPageLoadMetricsObserver();

  AbortsPageLoadMetricsObserver(const AbortsPageLoadMetricsObserver&) = delete;
  AbortsPageLoadMetricsObserver& operator=(
      const AbortsPageLoadMetricsObserver&) = delete;

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnFencedFramesStart(
      content::NavigationHandle* navigation_handle,
      const GURL& currently_committed_url) override;
  void OnComplete(
      const page_load_metrics::mojom::PageLoadTiming& timing) override;
  void OnFailedProvisionalLoad(
      const page_load_metrics::FailedProvisionalLoadInfo& failed_load_info)
      override;
};

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_ABORTS_PAGE_LOAD_METRICS_OBSERVER_H_

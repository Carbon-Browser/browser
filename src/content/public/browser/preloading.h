// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PRELOADING_H_
#define CONTENT_PUBLIC_BROWSER_PRELOADING_H_

#include <stdint.h>

namespace content {

// Defines the different types of preloading speedup techniques. Preloading is a
// holistic term to define all the speculative operations the browser does for
// loading content before a page navigates to make navigation faster.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingType {
  // No PreloadingType is present. This may include other preloading operations
  // which will be added later to PreloadingType as we expand.
  kUnspecified = 0,

  // TODO(crbug.com/1309934): Add preloading types 1 and 3 as we integrate
  // Preloading logging with various preloading types.

  // Establishes a connection (including potential TLS handshake) with an
  // origin.
  kPreconnect = 2,

  // This speedup technique comes with the most impact and overhead. We preload
  // and render a page before the user navigates to it. This will make the next
  // page navigation nearly instant as we would activate a fully prepared
  // RenderFrameHost. Both resources are fetched and JS is executed.
  kPrerender = 4,
};

// Defines various triggering mechanisms which triggers different preloading
// operations mentioned in preloading.h.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingPredictor {
  // No PreloadingTrigger is present. This may include the small percentage of
  // usages of browser triggers, link-rel, OptimizationGuideService e.t.c which
  // will be added later as a separate elements.
  kUnspecified = 0,

  // TODO(crbug.com/1309934): Add more predictors as we integrate Preloading
  // logging.

  // This constant is used to define the value from which features can add more
  // enums beyond this value both inside and outside content. We mask it by 50
  // and 100 to avoid usage of the same numbers for logging.

  // >= 50 values are reserved for content-internal values, such as
  // ContentPreloadingPredictor enum.
  kPreloadingPredictorContentStart = 50,

  // >= 100 values are reserved for embedder-specific values, such as the
  // ChromePreloadingPredictor enum.
  kPreloadingPredictorContentEnd = 100,
};

// Defines if a preloading operation is eligible for a given preloading
// trigger.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingEligibility {
  // Preloading operation is not defined for a particular preloading trigger
  // prediction.
  kUnspecified = 0,

  // Preloading operation is eligible and is triggered for a preloading
  // predictor.
  kEligible = 1,

  // Preloading operation could be ineligible if it is not triggered
  // because some precondition was not satisfied. Preloading here could
  // be ineligible due to various reasons subjective to the preloading
  // operation like the following.
  // These values are used in both //chrome and //content after integration with
  // various preloading features.

  // Preloading operation was ineligible because preloading was disabled.
  kPreloadingDisabled = 2,

  // Preloading operation was ineligible because it was triggered from the
  // background or a hidden page.
  kHidden = 3,

  // Preloading operation was ineligible because it was invoked for cross origin
  // navigation while preloading was restricted to same-origin navigations.
  // (It's plausible that some preloading mechanisms in the future could work
  // for cross-origin navigations as well.)
  kCrossOrigin = 4,

  // Preloading was ineligible due to low memory restrictions.
  kLowMemory = 5,

  // TODO(crbug.com/1309934): Add more specific ineligibility reasons subject to
  // each preloading operation
  // This constant is used to define the value from which embedders can add more
  // enums beyond this value.
  kPreloadingEligibilityContentEnd = 100,
};

// The outcome of the holdback check. This is not part of eligibility status to
// clarify that this check needs to happen after we are done verifying the
// eligibility of a preloading attempt. In general, eligibility checks can be
// reordered, but the holdback check always needs to come after verifying that
// the preloading attempt was eligible.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingHoldbackStatus {
  // The preloading holdback status has not been set yet. This should only
  // happen when the preloading attempt was not eligible.
  kUnspecified = 0,

  // The preload was eligible to be triggered and was not disabled via a field
  // trial holdback. Given enough time, the preload will trigger.
  kAllowed = 1,

  // The preload was eligible to be triggered but was disabled via a field
  // trial holdback. This is useful for measuring the impact of preloading.
  kHoldback = 2,
};

// Defines the post-triggering outcome once the preloading operation is
// triggered.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingTriggeringOutcome {
  // The outcome is kUnspecified for attempts that were not triggered due to
  // various ineligibility reasons or due to a field trial holdback.
  kUnspecified = 0,

  // For attempts that we wanted to trigger, but for which we already had an
  // equivalent attempt (same preloading operation and same URL/target) in
  // progress.
  kDuplicate = 2,

  // Preloading was triggered and did not fail, but did not complete in time
  // before the user navigated away (or the browser was shut down).
  kRunning = 3,

  // Preloading triggered and is ready to be used for the next navigation. This
  // doesn't mean preloading attempt was actually used.
  kReady = 4,

  // Preloading was triggered, completed successfully and was used for the next
  // navigation.
  kSuccess = 5,

  // Preloading was triggered but encountered an error and failed.
  kFailure = 6,

  // Some preloading features don't provide a way to keep track of the
  // progression of the preloading attempt. In those cases, we just log
  // kTriggeredButOutcomeUnknown, meaning that preloading was triggered but we
  // don't know if it was successful.
  kTriggeredButOutcomeUnknown = 7,
};

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PreloadingFailureReason {
  // The failure reason is unspecified if the triggering outcome is not
  // kFailure.
  kUnspecified = 0,

  // This constant is used to define the value from which specifying preloading
  // types can add more enums beyond this value. We mask it by 100 to avoid
  // usage of the same numbers for logging. The semantics of values beyond 100
  // can vary by preloading type (for example 101 might mean "the page was
  // destroyed" for prerender, but "the user already had cookies for a
  // cross-origin prefetch"
  // for prefetch).
  kPreloadingFailureReasonCommonEnd = 100,
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PRELOADING_H_

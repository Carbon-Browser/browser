// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_PRELOADING_TEST_UTIL_H_
#define CONTENT_PUBLIC_TEST_PRELOADING_TEST_UTIL_H_

#include <vector>

#include "components/ukm/test_ukm_recorder.h"
#include "content/public/browser/preloading.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace content::test {

// The set of UKM metric names in the PreloadingAttempt and PreloadingPrediction
// UKM logs. This is useful for calling TestUkmRecorder::GetEntries.
extern const std::vector<std::string> kPreloadingAttemptUkmMetrics;
extern const std::vector<std::string> kPreloadingPredictionUkmMetrics;

// Utility class to make building expected
// TestUkmRecorder::HumanReadableUkmEntry for EXPECT_EQ for PreloadingAttempt.
class PreloadingAttemptUkmEntryBuilder {
 public:
  PreloadingAttemptUkmEntryBuilder(PreloadingType preloading_type,
                                   PreloadingPredictor predictor);

  ukm::TestUkmRecorder::HumanReadableUkmEntry BuildEntry(
      ukm::SourceId source_id,
      PreloadingEligibility eligibility,
      PreloadingHoldbackStatus holdback_status,
      PreloadingTriggeringOutcome triggering_outcome,
      PreloadingFailureReason failure_reason,
      bool accurate) const;

 private:
  PreloadingType preloading_type_;
  PreloadingPredictor predictor_;
};

// Utility class to make building expected
// TestUkmRecorder::HumanReadableUkmEntry for EXPECT_EQ for
// PreloadingPrediction.
class PreloadingPredictionUkmEntryBuilder {
 public:
  explicit PreloadingPredictionUkmEntryBuilder(PreloadingPredictor predictor);

  ukm::TestUkmRecorder::HumanReadableUkmEntry BuildEntry(
      ukm::SourceId source_id,
      int64_t confidence,
      bool accurate_prediction) const;

 private:
  PreloadingPredictor predictor_;
};

// Turns a UKM entry into a human-readable string.
std::string UkmEntryToString(
    const ukm::TestUkmRecorder::HumanReadableUkmEntry& entry);

// Turns two UKM entries into a human-readable string.
std::string ActualVsExpectedUkmEntryToString(
    const ukm::TestUkmRecorder::HumanReadableUkmEntry& actual,
    const ukm::TestUkmRecorder::HumanReadableUkmEntry& expected);

// Turns two collections of UKM entries into human-readable strings.
std::string ActualVsExpectedUkmEntriesToString(
    const std::vector<ukm::TestUkmRecorder::HumanReadableUkmEntry>& actual,
    const std::vector<ukm::TestUkmRecorder::HumanReadableUkmEntry>& expected);

}  // namespace content::test

#endif  // CONTENT_PUBLIC_TEST_PRELOADING_TEST_UTIL_H_

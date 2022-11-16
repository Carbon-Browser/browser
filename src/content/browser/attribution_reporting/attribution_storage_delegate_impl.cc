// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/attribution_reporting/attribution_storage_delegate_impl.h"

#include <stdint.h>

#include <cstdlib>
#include <utility>

#include "base/check_op.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "content/browser/attribution_reporting/attribution_default_random_generator.h"
#include "content/browser/attribution_reporting/attribution_random_generator.h"
#include "content/browser/attribution_reporting/attribution_report.h"
#include "content/browser/attribution_reporting/attribution_reporting_constants.h"
#include "content/browser/attribution_reporting/attribution_utils.h"
#include "content/browser/attribution_reporting/combinatorics.h"
#include "content/browser/attribution_reporting/common_source_info.h"
#include "content/public/browser/attribution_reporting.h"

namespace content {

AttributionStorageDelegateImpl::AttributionStorageDelegateImpl(
    AttributionNoiseMode noise_mode,
    AttributionDelayMode delay_mode)
    : AttributionStorageDelegateImpl(
          noise_mode,
          delay_mode,
          std::make_unique<AttributionDefaultRandomGenerator>()) {}

AttributionStorageDelegateImpl::AttributionStorageDelegateImpl(
    AttributionNoiseMode noise_mode,
    AttributionDelayMode delay_mode,
    std::unique_ptr<AttributionRandomGenerator> rng)
    : noise_mode_(noise_mode), delay_mode_(delay_mode), rng_(std::move(rng)) {
  DCHECK(rng_);

  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AttributionStorageDelegateImpl::~AttributionStorageDelegateImpl() = default;

int AttributionStorageDelegateImpl::GetMaxAttributionsPerSource(
    AttributionSourceType source_type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (source_type) {
    case AttributionSourceType::kNavigation:
      return kMaxAttributionsPerNavigationSource;
    case AttributionSourceType::kEvent:
      return kMaxAttributionsPerEventSource;
  }
}

int AttributionStorageDelegateImpl::GetMaxSourcesPerOrigin() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return kAttributionMaxSourcesPerOrigin;
}

int AttributionStorageDelegateImpl::GetMaxReportsPerDestination(
    AttributionReport::ReportType) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return kAttributionMaxReportsPerDestination;
}

int AttributionStorageDelegateImpl::
    GetMaxDestinationsPerSourceSiteReportingOrigin() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return kAttributionMaxDestinationsPerSourceSiteReportingOrigin;
}

AttributionRateLimitConfig AttributionStorageDelegateImpl::GetRateLimits()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return AttributionRateLimitConfig::kDefault;
}

base::TimeDelta
AttributionStorageDelegateImpl::GetDeleteExpiredSourcesFrequency() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::Minutes(5);
}

base::TimeDelta
AttributionStorageDelegateImpl::GetDeleteExpiredRateLimitsFrequency() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::Minutes(5);
}

base::Time AttributionStorageDelegateImpl::GetEventLevelReportTime(
    const CommonSourceInfo& source,
    base::Time trigger_time) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (delay_mode_) {
    case AttributionDelayMode::kDefault:
      return ComputeReportTime(source, trigger_time);
    case AttributionDelayMode::kNone:
      return trigger_time;
  }
}

base::Time AttributionStorageDelegateImpl::GetAggregatableReportTime(
    base::Time trigger_time) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (delay_mode_) {
    case AttributionDelayMode::kDefault:
      switch (noise_mode_) {
        case AttributionNoiseMode::kDefault:
          return trigger_time + kAttributionAggregatableReportMinDelay +
                 rng_->RandDouble() * kAttributionAggregatableReportDelaySpan;
        case AttributionNoiseMode::kNone:
          return trigger_time + kAttributionAggregatableReportMinDelay +
                 kAttributionAggregatableReportDelaySpan;
      }

    case AttributionDelayMode::kNone:
      return trigger_time;
  }
}

base::GUID AttributionStorageDelegateImpl::NewReportID() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::GUID::GenerateRandomV4();
}

absl::optional<AttributionStorageDelegate::OfflineReportDelayConfig>
AttributionStorageDelegateImpl::GetOfflineReportDelayConfig() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (noise_mode_ == AttributionNoiseMode::kDefault &&
      delay_mode_ == AttributionDelayMode::kDefault) {
    // Add uniform random noise in the range of [0, 1 minutes] to the report
    // time.
    // TODO(https://crbug.com/1075600): This delay is very conservative.
    // Consider increasing this delay once we can be sure reports are still
    // sent at reasonable times, and not delayed for many browser sessions due
    // to short session up-times.
    return OfflineReportDelayConfig{
        .min = base::Minutes(0),
        .max = base::Minutes(1),
    };
  }

  return absl::nullopt;
}

void AttributionStorageDelegateImpl::ShuffleReports(
    std::vector<AttributionReport>& reports) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (noise_mode_) {
    case AttributionNoiseMode::kDefault:
      rng_->RandomShuffle(reports);
      break;
    case AttributionNoiseMode::kNone:
      break;
  }
}

double AttributionStorageDelegateImpl::GetRandomizedResponseRate(
    AttributionSourceType source_type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (source_type) {
    case AttributionSourceType::kNavigation:
      return kAttributionNavigationSourceRandomizedResponseRate;
    case AttributionSourceType::kEvent:
      return kAttributionEventSourceRandomizedResponseRate;
  }
}

AttributionStorageDelegate::RandomizedResponse
AttributionStorageDelegateImpl::GetRandomizedResponse(
    const CommonSourceInfo& source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (noise_mode_) {
    case AttributionNoiseMode::kDefault: {
      double randomized_trigger_rate =
          GetRandomizedResponseRate(source.source_type());
      DCHECK_GE(randomized_trigger_rate, 0);
      DCHECK_LE(randomized_trigger_rate, 1);

      return rng_->RandDouble() < randomized_trigger_rate
                 ? absl::make_optional(GetRandomFakeReports(source))
                 : absl::nullopt;
    }
    case AttributionNoiseMode::kNone:
      return absl::nullopt;
  }
}

std::vector<AttributionStorageDelegate::FakeReport>
AttributionStorageDelegateImpl::GetRandomFakeReports(
    const CommonSourceInfo& source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(noise_mode_, AttributionNoiseMode::kDefault);

  const int num_combinations = GetNumberOfStarsAndBarsSequences(
      /*num_stars=*/GetMaxAttributionsPerSource(source.source_type()),
      /*num_bars=*/TriggerDataCardinality(source.source_type()) *
          NumReportWindows(source.source_type()));

  // Subtract 1 because `AttributionRandomGenerator::RandInt()` is inclusive.
  const int sequence_index = rng_->RandInt(0, num_combinations - 1);

  return GetFakeReportsForSequenceIndex(source, sequence_index);
}

std::vector<AttributionStorageDelegate::FakeReport>
AttributionStorageDelegateImpl::GetFakeReportsForSequenceIndex(
    const CommonSourceInfo& source,
    int random_stars_and_bars_sequence_index) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(noise_mode_, AttributionNoiseMode::kDefault);

  const int trigger_data_cardinality =
      TriggerDataCardinality(source.source_type());

  const std::vector<int> bars_preceding_each_star =
      GetBarsPrecedingEachStar(GetStarIndices(
          /*num_stars=*/GetMaxAttributionsPerSource(source.source_type()),
          /*num_bars=*/trigger_data_cardinality *
              NumReportWindows(source.source_type()),
          /*sequence_index=*/random_stars_and_bars_sequence_index));

  std::vector<FakeReport> fake_reports;

  // an output state is uniquely determined by an ordering of c stars and w*d
  // bars, where:
  // w = the number of reporting windows
  // c = the maximum number of reports for a source
  // d = the trigger data cardinality for a source
  for (int num_bars : bars_preceding_each_star) {
    if (num_bars == 0)
      continue;

    auto result = std::div(num_bars - 1, trigger_data_cardinality);

    const int trigger_data = result.rem;
    DCHECK_GE(trigger_data, 0);
    DCHECK_LT(trigger_data, trigger_data_cardinality);

    fake_reports.push_back({
        .trigger_data = static_cast<uint64_t>(trigger_data),
        .report_time = ReportTimeAtWindow(source, /*window_index=*/result.quot),
    });
  }
  return fake_reports;
}

int64_t AttributionStorageDelegateImpl::GetAggregatableBudgetPerSource() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return kAttributionAggregatableBudgetPerSource;
}

uint64_t AttributionStorageDelegateImpl::SanitizeTriggerData(
    uint64_t trigger_data,
    AttributionSourceType source_type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const uint64_t cardinality = TriggerDataCardinality(source_type);
  return trigger_data % cardinality;
}

uint64_t AttributionStorageDelegateImpl::SanitizeSourceEventId(
    uint64_t source_event_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  static_assert(!kAttributionSourceEventIdCardinality.has_value(),
                "update sanitize logic below");
  return source_event_id;
}

uint64_t AttributionStorageDelegateImpl::TriggerDataCardinality(
    AttributionSourceType source_type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (source_type) {
    case AttributionSourceType::kNavigation:
      return kAttributionNavigationSourceTriggerDataCardinality;
    case AttributionSourceType::kEvent:
      return kAttributionEventSourceTriggerDataCardinality;
  }
}

}  // namespace content

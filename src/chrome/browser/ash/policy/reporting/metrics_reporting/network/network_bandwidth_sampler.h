// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_REPORTING_METRICS_REPORTING_NETWORK_NETWORK_BANDWIDTH_SAMPLER_H_
#define CHROME_BROWSER_ASH_POLICY_REPORTING_METRICS_REPORTING_NETWORK_NETWORK_BANDWIDTH_SAMPLER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "components/reporting/metrics/sampler.h"
#include "services/network/public/cpp/network_quality_tracker.h"

namespace reporting {

// Sampler used to collect network bandwidth information like download speed
// (in kbps). This sampler relies on the `NetworkQualityTracker` defined in
// the `NetworkService` to collect said metrics.
class NetworkBandwidthSampler : public Sampler {
 public:
  NetworkBandwidthSampler(
      ::network::NetworkQualityTracker* network_quality_tracker,
      Profile* profile);
  NetworkBandwidthSampler(const NetworkBandwidthSampler& other) = delete;
  NetworkBandwidthSampler& operator=(const NetworkBandwidthSampler& other) =
      delete;
  ~NetworkBandwidthSampler() override = default;

  // Collects network bandwidth info if the corresponding user policy is set
  // and reports collected metrics using the specified callback. Reports
  // `absl::nullopt` otherwise.
  void MaybeCollect(OptionalMetricCallback callback) override;

 private:
  const raw_ptr<::network::NetworkQualityTracker> network_quality_tracker_;
  const raw_ptr<Profile> profile_;
};

}  // namespace reporting

#endif  // CHROME_BROWSER_ASH_POLICY_REPORTING_METRICS_REPORTING_NETWORK_NETWORK_BANDWIDTH_SAMPLER_H_

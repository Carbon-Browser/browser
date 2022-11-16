// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/services/secure_channel/public/cpp/client/nearby_metrics_recorder.h"

namespace ash::secure_channel {

NearbyMetricsRecorder::NearbyMetricsRecorder() = default;

NearbyMetricsRecorder::~NearbyMetricsRecorder() = default;

void NearbyMetricsRecorder::RecordConnectionSuccess(
    const base::TimeDelta latency) {
  RecordConnectionResult(true);
  RecordConnectionLatency(latency);
}

void NearbyMetricsRecorder::RecordConnectionFailure(
    mojom::ConnectionAttemptFailureReason reason) {
  RecordConnectionResult(false);
  RecordConnectionFailureReason(reason);
}

}  // namespace ash::secure_channel

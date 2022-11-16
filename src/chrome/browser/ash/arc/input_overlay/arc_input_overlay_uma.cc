// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/arc/input_overlay/arc_input_overlay_uma.h"

#include "base/metrics/histogram_functions.h"

namespace arc {
namespace input_overlay {

void RecordInputOverlayFeatureState(bool enable) {
  base::UmaHistogramBoolean("Arc.InputOverlay.FeatureState", enable);
}

void RecordInputOverlayMappingHintState(bool enable) {
  base::UmaHistogramBoolean("Arc.InputOverlay.MappingHintState", enable);
}

void RecordInputOverlayCustomizedUsage() {
  base::UmaHistogramBoolean("Arc.InputOverlay.Customized", true);
}

}  // namespace input_overlay
}  // namespace arc

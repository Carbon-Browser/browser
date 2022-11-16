// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_ARC_INPUT_OVERLAY_UMA_H_
#define CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_ARC_INPUT_OVERLAY_UMA_H_

namespace arc {
namespace input_overlay {

void RecordInputOverlayFeatureState(bool enable);

void RecordInputOverlayMappingHintState(bool enable);

void RecordInputOverlayCustomizedUsage();

}  // namespace input_overlay
}  // namespace arc

#endif  // CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_ARC_INPUT_OVERLAY_UMA_H_

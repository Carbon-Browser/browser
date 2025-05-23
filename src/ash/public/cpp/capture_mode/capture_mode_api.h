// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_CAPTURE_MODE_CAPTURE_MODE_API_H_
#define ASH_PUBLIC_CPP_CAPTURE_MODE_CAPTURE_MODE_API_H_

#include "ash/ash_export.h"

namespace ash {

// Full screen capture for each available display if no restricted content
// exists on that display, each capture is saved as an individual file.
// Note: this won't start a capture mode session.
void ASH_EXPORT CaptureScreenshotsOfAllDisplays();

// Returns true if the active account can bypass the feature key check.
bool ASH_EXPORT IsSunfishFeatureEnabledWithFeatureKey();

// Returns true if either Sunfish or Scanner is enabled.
bool ASH_EXPORT IsSunfishOrScannerEnabled();

// Returns whether the Sunfish feature is allowed and enabled by the user, i.e.
// via the user prefs.
bool ASH_EXPORT IsSunfishAllowedAndEnabled();

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_CAPTURE_MODE_CAPTURE_MODE_API_H_

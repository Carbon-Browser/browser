// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_FAVICON_SIZE_H_
#define UI_GFX_FAVICON_SIZE_H_

#include "base/component_export.h"

namespace gfx {

// Size (along each axis) of the favicon.
COMPONENT_EXPORT(GFX) extern const int kFaviconSize;

// If the width or height is bigger than the favicon size, a new width/height
// is calculated and returned in width/height that maintains the aspect
// ratio of the supplied values.
COMPONENT_EXPORT(GFX) void CalculateFaviconTargetSize(int* width, int* height);

}  // namespace gfx

#endif  // UI_GFX_FAVICON_SIZE_H_

// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_PATH_WIN_H_
#define UI_GFX_PATH_WIN_H_

#include <windows.h>

#include "base/component_export.h"

class SkPath;
class SkRegion;

namespace gfx {

// Creates a new HRGN given |region|. The caller is responsible for destroying
// the returned region.
COMPONENT_EXPORT(GFX) HRGN CreateHRGNFromSkRegion(const SkRegion& path);

// Creates a new HRGN given |path|. The caller is responsible for destroying
// the returned region. Returns empty region (not NULL) for empty path.
COMPONENT_EXPORT(GFX) HRGN CreateHRGNFromSkPath(const SkPath& path);

}  // namespace gfx

#endif  // UI_GFX_PATH_WIN_H_

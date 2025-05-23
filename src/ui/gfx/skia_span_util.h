// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SKIA_SPAN_UTIL_H_
#define UI_GFX_SKIA_SPAN_UTIL_H_

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "ui/gfx/gfx_skia_export.h"

namespace gfx {

// Returns a span to the pixel memory for pixmap.
GFX_SKIA_EXPORT base::span<const uint8_t> SkPixmapToSpan(
    const SkPixmap& pixmap LIFETIME_BOUND);
GFX_SKIA_EXPORT base::span<uint8_t> SkPixmapToWritableSpan(
    const SkPixmap& pixmap LIFETIME_BOUND);

// Returns a span to `data` owned memory.
GFX_SKIA_EXPORT base::span<const uint8_t> SkDataToSpan(
    sk_sp<SkData> data LIFETIME_BOUND);

// Wrapper around SkData::MakeWithCopy().
GFX_SKIA_EXPORT sk_sp<SkData> MakeSkDataFromSpanWithCopy(
    base::span<const uint8_t> data);

// Wrapper around SkData::MakeWithoutCopy().
GFX_SKIA_EXPORT sk_sp<SkData> MakeSkDataFromSpanWithoutCopy(
    base::span<const uint8_t> data);

}  // namespace gfx

#endif  // UI_GFX_SKIA_SPAN_UTIL_H_

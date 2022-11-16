// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgpu/texture_utils.h"

#include "third_party/blink/renderer/modules/webgpu/gpu_device.h"

namespace blink {

WTF::String StringFromASCIIAndUTF8(const char* message) {
  return WTF::String::FromUTF8WithLatin1Fallback(message, strlen(message));
}
}  // namespace blink

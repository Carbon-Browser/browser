// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This source code is a part of eyeo Chromium SDK.
// Use of this source code is governed by the GPLv3 that can be found in the
// components/adblock/LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_ISOLATED_WORLD_IDS_H_
#define CONTENT_PUBLIC_COMMON_ISOLATED_WORLD_IDS_H_

#include <cstdint>

namespace content {

// Please keep this enum in sync with IsolatedWordIds.java
// LINT.IfChange
enum IsolatedWorldIDs : int32_t {
  // The main world. Chrome cannot use ID 0 for an isolated world because 0
  // represents the main world.
  ISOLATED_WORLD_ID_GLOBAL = 0,

  // Isolated world for eyeo ad blocking (element hiding)
  ISOLATED_WORLD_ID_ADBLOCK,

  // Custom isolated world ids used by other embedders should start from here.
  ISOLATED_WORLD_ID_CONTENT_END,
  // If any embedder has more than 10 custom isolated worlds that will be run
  // via RenderFrameImpl::OnJavaScriptExecuteRequestInIsolatedWorld update this.
  ISOLATED_WORLD_ID_MAX = ISOLATED_WORLD_ID_CONTENT_END + 10,
};
// LINT.ThenChange(//content/public/android/java/src/org/chromium/content_public/common/IsolatedWorldIds.java)
}  // namespace content
#endif  // CONTENT_PUBLIC_COMMON_ISOLATED_WORLD_IDS_H_

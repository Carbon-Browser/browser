// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PRELOADING_PRELOADING_H_
#define CONTENT_BROWSER_PRELOADING_PRELOADING_H_

#include "content/public/browser/preloading.h"

#include "content/common/content_export.h"

namespace content {

// Defines various //content triggering mechanisms which trigger different
// preloading operations mentioned in content/public/browser/preloading.h.

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class ContentPreloadingPredictor {
  // Numbering starts from `kPreloadingPredictorContentStart` defined in
  // //content/public/preloading.h. Advance numbering by +1 after adding a new
  // element.

  // This API allows an origin to list possible navigation URLs that the user
  // might navigate to in order to perform preloading operations.
  // For more details please see:
  // https://wicg.github.io/nav-speculation/prerendering.html#speculation-rules
  kSpeculationRules =
      static_cast<int>(PreloadingPredictor::kPreloadingPredictorContentStart),

  // TODO(crbug.com/1309934): Add more predictors as we integrate Preloading
  // logging.
};

// Helper method to convert ContentPreloadingPredictor to
// content::PreloadingPredictor to avoid casting.
PreloadingPredictor CONTENT_EXPORT
ToPreloadingPredictor(ContentPreloadingPredictor predictor);

}  // namespace content

#endif  // CONTENT_BROWSER_PRELOADING_PRELOADING_H_

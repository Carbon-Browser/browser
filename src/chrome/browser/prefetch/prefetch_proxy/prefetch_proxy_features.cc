// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefetch/prefetch_proxy/prefetch_proxy_features.h"

namespace features {

// Forces all eligible prerenders to be done in an isolated manner such that no
// user-identifying information is used during the prefetch.
const base::Feature kIsolatePrerenders {
  "IsolatePrerenders",
#if BUILDFLAG(IS_ANDROID)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Forces Chrome to probe the origin before reusing a cached response.
const base::Feature kIsolatePrerendersMustProbeOrigin {
  "IsolatePrerendersMustProbeOrigin",
#if BUILDFLAG(IS_ANDROID)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

}  // namespace features

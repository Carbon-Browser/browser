// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DEFAULT_BROWSER_MODEL_PROMO_SOURCE_H_
#define IOS_CHROME_BROWSER_DEFAULT_BROWSER_MODEL_PROMO_SOURCE_H_

// An histogram to report the source of the default browser promo.
// Used for UMA, do not reorder.
// LINT.IfChange
enum class DefaultBrowserPromoSource {
  kSettings = 0,
  kOmnibox,
  kExternalIntent,
  kSetUpList,
  // kExternalAction refers to Chrome being opened with a "ChromeExternalAction"
  // host.
  kExternalAction,
  kMaxValue = kExternalAction,
};
// LINT.ThenChange(//tools/metrics/histograms/metadata/ios/enums.xml)

#endif  // IOS_CHROME_BROWSER_DEFAULT_BROWSER_MODEL_PROMO_SOURCE_H_

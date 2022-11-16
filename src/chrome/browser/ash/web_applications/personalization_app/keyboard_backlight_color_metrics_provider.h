// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_WEB_APPLICATIONS_PERSONALIZATION_APP_KEYBOARD_BACKLIGHT_COLOR_METRICS_PROVIDER_H_
#define CHROME_BROWSER_ASH_WEB_APPLICATIONS_PERSONALIZATION_APP_KEYBOARD_BACKLIGHT_COLOR_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

class KeyboardBacklightColorMetricsProvider : public metrics::MetricsProvider {
 public:
  KeyboardBacklightColorMetricsProvider();

  KeyboardBacklightColorMetricsProvider(
      const KeyboardBacklightColorMetricsProvider&) = delete;
  KeyboardBacklightColorMetricsProvider& operator=(
      const KeyboardBacklightColorMetricsProvider&) = delete;

  ~KeyboardBacklightColorMetricsProvider() override;

  // metrics::MetricsProvider:
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto_unused) override;
};

#endif  // CHROME_BROWSER_ASH_WEB_APPLICATIONS_PERSONALIZATION_APP_KEYBOARD_BACKLIGHT_COLOR_METRICS_PROVIDER_H_

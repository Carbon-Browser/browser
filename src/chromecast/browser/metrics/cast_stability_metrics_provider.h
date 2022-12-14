// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_METRICS_CAST_STABILITY_METRICS_PROVIDER_H_
#define CHROMECAST_BROWSER_METRICS_CAST_STABILITY_METRICS_PROVIDER_H_

#include "base/process/kill.h"
#include "components/metrics/metrics_provider.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PrefService;

namespace content {
class RenderProcessHost;
}

namespace metrics {
class MetricsService;
}

namespace chromecast {
namespace metrics {

// Responsible for gathering and logging stability-related metrics. Loosely
// based on the ContentStabilityMetricsProvider in components/metrics/content.
class CastStabilityMetricsProvider : public ::metrics::MetricsProvider,
                                     public content::NotificationObserver {
 public:
  CastStabilityMetricsProvider(::metrics::MetricsService* metrics_service,
                               PrefService* pref_service);

  CastStabilityMetricsProvider(const CastStabilityMetricsProvider&) = delete;
  CastStabilityMetricsProvider& operator=(const CastStabilityMetricsProvider&) =
      delete;

  ~CastStabilityMetricsProvider() override = default;

  // metrics::MetricsProvider implementation:
  void OnRecordingEnabled() override;
  void OnRecordingDisabled() override;

  // Logs an external crash, presumably from the ExternalMetrics service.
  void LogExternalCrash(const std::string& crash_type);

 private:
  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Records a renderer process crash.
  void LogRendererCrash(content::RenderProcessHost* host,
                        base::TerminationStatus status,
                        int exit_code);

  // Increments the specified pref by 1.
  void IncrementPrefValue(const char* path);

  // Registrar for receiving stability-related notifications.
  content::NotificationRegistrar registrar_;

  // Reference to the current MetricsService. Raw pointer is safe, since
  // MetricsService is responsible for the lifetime of
  // CastStabilityMetricsProvider.
  ::metrics::MetricsService* metrics_service_;

  PrefService* const pref_service_;
};

}  // namespace metrics
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_METRICS_CAST_STABILITY_METRICS_PROVIDER_H_

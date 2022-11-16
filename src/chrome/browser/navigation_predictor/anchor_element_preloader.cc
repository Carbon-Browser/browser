// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/navigation_predictor/anchor_element_preloader.h"
#include "base/callback.h"
#include "base/metrics/histogram_functions.h"
#include "chrome/browser/predictors/loading_predictor.h"
#include "chrome/browser/predictors/loading_predictor_factory.h"
#include "chrome/browser/prefetch/prefetch_prefs.h"
#include "chrome/browser/preloading/chrome_preloading.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/preloading.h"
#include "content/public/browser/preloading_data.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/blink/public/common/features.h"
#include "url/scheme_host_port.h"

namespace {
bool is_match_for_preconnect(const url::SchemeHostPort& preconnected_origin,
                             const GURL& visited_url) {
  return preconnected_origin == url::SchemeHostPort(visited_url);
}
}  // anonymous namespace

const char kPreloadingAnchorElementPreloaderPreloadingTriggered[] =
    "Preloading.AnchorElementPreloader.PreloadingTriggered";

content::PreloadingFailureReason ToFailureReason(
    AnchorPreloadingFailureReason reason) {
  return static_cast<content::PreloadingFailureReason>(reason);
}

AnchorElementPreloader::~AnchorElementPreloader() = default;

AnchorElementPreloader::AnchorElementPreloader(
    content::RenderFrameHost& render_frame_host,
    mojo::PendingReceiver<blink::mojom::AnchorElementInteractionHost> receiver)
    : content::DocumentService<blink::mojom::AnchorElementInteractionHost>(
          render_frame_host,
          std::move(receiver)) {}

void AnchorElementPreloader::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<blink::mojom::AnchorElementInteractionHost>
        receiver) {
  CHECK(render_frame_host);
  // The object is bound to the lifetime of the |render_frame_host| and the mojo
  // connection. See DocumentService for details.
  new AnchorElementPreloader(*render_frame_host, std::move(receiver));
}

void AnchorElementPreloader::OnPointerDown(const GURL& target) {
  content::PreloadingData* preloading_data =
      content::PreloadingData::GetOrCreateForWebContents(
          content::WebContents::FromRenderFrameHost(&render_frame_host()));
  url::SchemeHostPort scheme_host_port(target);
  content::PreloadingURLMatchCallback match_callback =
      base::BindRepeating(is_match_for_preconnect, scheme_host_port);

  // For now we add a prediction with a confidence of 100. In the future we will
  // likely compute the confidence by looking at different factors (e.g. anchor
  // element dimensions, last time since scroll, etc.).
  preloading_data->AddPreloadingPrediction(
      ToPreloadingPredictor(ChromePreloadingPredictor::kPointerDownOnAnchor),
      /*confidence=*/100, match_callback);
  content::PreloadingAttempt* attempt = preloading_data->AddPreloadingAttempt(
      ToPreloadingPredictor(ChromePreloadingPredictor::kPointerDownOnAnchor),
      content::PreloadingType::kPreconnect, match_callback);

  if (!prefetch::IsSomePreloadingEnabled(
          *Profile::FromBrowserContext(render_frame_host().GetBrowserContext())
               ->GetPrefs())) {
    attempt->SetEligibility(
        content::PreloadingEligibility::kPreloadingDisabled);
    return;
  }

  auto* loading_predictor = predictors::LoadingPredictorFactory::GetForProfile(
      Profile::FromBrowserContext(render_frame_host().GetBrowserContext()));
  if (!loading_predictor) {
    attempt->SetEligibility(ToPreloadingEligibility(
        ChromePreloadingEligibility::kUnableToGetLoadingPredictor));
    return;
  }

  attempt->SetEligibility(content::PreloadingEligibility::kEligible);
  RecordUmaPreloadedTriggered(AnchorElementPreloaderType::kPreconnect);

  if (base::GetFieldTrialParamByFeatureAsBool(
          blink::features::kAnchorElementInteraction, "preconnect_holdback",
          false)) {
    attempt->SetHoldbackStatus(content::PreloadingHoldbackStatus::kHoldback);
    return;
  }
  attempt->SetHoldbackStatus(content::PreloadingHoldbackStatus::kAllowed);

  if (preconnected_targets_.find(scheme_host_port) !=
      preconnected_targets_.end()) {
    // We've already preconnected to that origin.
    attempt->SetTriggeringOutcome(
        content::PreloadingTriggeringOutcome::kDuplicate);
    return;
  }
  int max_preloading_attempts = base::GetFieldTrialParamByFeatureAsInt(
      blink::features::kAnchorElementInteraction, "max_preloading_attempts",
      -1);
  if (max_preloading_attempts >= 0 &&
      preconnected_targets_.size() >=
          static_cast<size_t>(max_preloading_attempts)) {
    attempt->SetFailureReason(
        ToFailureReason(AnchorPreloadingFailureReason::kLimitExceeded));
    return;
  }
  preconnected_targets_.insert(scheme_host_port);
  attempt->SetTriggeringOutcome(
      content::PreloadingTriggeringOutcome::kTriggeredButOutcomeUnknown);

  net::SchemefulSite schemeful_site(target);
  net::NetworkIsolationKey network_isolation_key(schemeful_site,
                                                 schemeful_site);
  loading_predictor->PreconnectURLIfAllowed(target, /*allow_credentials=*/true,
                                            network_isolation_key);
}

void AnchorElementPreloader::RecordUmaPreloadedTriggered(
    AnchorElementPreloaderType preload) {
  base::UmaHistogramEnumeration(
      kPreloadingAnchorElementPreloaderPreloadingTriggered, preload);
}

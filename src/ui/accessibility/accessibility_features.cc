// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/accessibility_features.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_features.mojom-features.h"

#if BUILDFLAG(IS_CHROMEOS)
#include "ash/constants/ash_features.h"
#endif

namespace features {

BASE_FEATURE(kAccessibilityPdfOcrForSelectToSpeak,
             "kAccessibilityPdfOcrForSelectToSpeak",
#if BUILDFLAG(IS_CHROMEOS)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif  // BUILDFLAG(IS_CHROMEOS)
);
bool IsAccessibilityPdfOcrForSelectToSpeakEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityPdfOcrForSelectToSpeak);
}

BASE_FEATURE(kAccessibilityExposeSummaryAsHeading,
             "AccessibilityExposeSummaryAsHeading",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityExposeSummaryAsHeadingEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityExposeSummaryAsHeading);
}

BASE_FEATURE(kAccessibilityBlockFlowIterator,
             "AccessibilityBlockFlowIterator",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityBlockFlowIteratorEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityBlockFlowIterator);
}

BASE_FEATURE(kAccessibilityPruneRedundantInlineText,
             "AccessibilityPruneRedundantInlineText",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityPruneRedundantInlineTextEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityPruneRedundantInlineText);
}

BASE_FEATURE(kAccessibilityPruneRedundantInlineConnectivity,
             "AccessibilityPruneRedundantInlineConnectivity",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityPruneRedundantInlineConnectivityEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityPruneRedundantInlineConnectivity);
}

BASE_FEATURE(kImageDescriptionsAlternateRouting,
             "ImageDescriptionsAlternateRouting",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsImageDescriptionsAlternateRoutingEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kImageDescriptionsAlternateRouting);
}

BASE_FEATURE(kAutoDisableAccessibility,
             "AutoDisableAccessibility",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAutoDisableAccessibilityEnabled() {
  return base::FeatureList::IsEnabled(::features::kAutoDisableAccessibility);
}

BASE_FEATURE(kEnableAccessibilityAriaVirtualContent,
             "AccessibilityAriaVirtualContent",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityAriaVirtualContentEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kEnableAccessibilityAriaVirtualContent);
}

BASE_FEATURE(kEnableAccessibilityLanguageDetection,
             "AccessibilityLanguageDetection",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityLanguageDetectionEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kEnableAccessibilityLanguageDetection);
}

BASE_FEATURE(kEnableAccessibilityRestrictiveIA2AXModes,
             "AccessibilityRestrictiveIA2AXModes",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityRestrictiveIA2AXModesEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kEnableAccessibilityRestrictiveIA2AXModes);
}

BASE_FEATURE(kExtensionManifestV3NetworkSpeechSynthesis,
             "ExtensionManifestV3NetworkSpeechSynthesis",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsExtensionManifestV3NetworkSpeechSynthesisEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kExtensionManifestV3NetworkSpeechSynthesis);
}

BASE_FEATURE(kEnableAriaElementReflection,
             "EnableAriaElementReflection",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAriaElementReflectionEnabled() {
  return base::FeatureList::IsEnabled(::features::kEnableAriaElementReflection);
}

BASE_FEATURE(kTextBasedAudioDescription,
             "TextBasedAudioDescription",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsTextBasedAudioDescriptionEnabled() {
  return base::FeatureList::IsEnabled(::features::kTextBasedAudioDescription);
}

BASE_FEATURE(kUseAXPositionForDocumentMarkers,
             "UseAXPositionForDocumentMarkers",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsUseAXPositionForDocumentMarkersEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kUseAXPositionForDocumentMarkers);
}

#if BUILDFLAG(IS_WIN)
BASE_FEATURE(kIChromeAccessible,
             "IChromeAccessible",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsIChromeAccessibleEnabled() {
  return base::FeatureList::IsEnabled(::features::kIChromeAccessible);
}

BASE_FEATURE(kSelectiveUIAEnablement,
             "SelectiveUIAEnablement",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsSelectiveUIAEnablementEnabled() {
  return base::FeatureList::IsEnabled(::features::kSelectiveUIAEnablement);
}

BASE_FEATURE(kUiaProvider, "UiaProvider", base::FEATURE_DISABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_CHROMEOS)
bool IsDictationOfflineAvailable() {
  return base::FeatureList::IsEnabled(
      ash::features::kOnDeviceSpeechRecognition);
}

BASE_FEATURE(kAccessibilityAccelerator,
             "AccessibilityAccelerator",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityAcceleratorEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityAccelerator);
}

BASE_FEATURE(kAccessibilityReducedAnimations,
             "AccessibilityReducedAnimations",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityReducedAnimationsEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityReducedAnimations);
}

BASE_FEATURE(kAccessibilityFaceGaze,
             "AccessibilityFaceGaze",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityFaceGazeEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityFaceGaze);
}

BASE_FEATURE(kAccessibilityReducedAnimationsInKiosk,
             "AccessibilityReducedAnimationsInKiosk",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityReducedAnimationsInKioskEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityReducedAnimationsInKiosk);
}

BASE_FEATURE(kExperimentalAccessibilityGoogleTtsHighQualityVoices,
             "ExperimentalAccessibilityGoogleTtsHighQualityVoices",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsExperimentalAccessibilityGoogleTtsHighQualityVoicesEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kExperimentalAccessibilityGoogleTtsHighQualityVoices);
}

BASE_FEATURE(kExperimentalAccessibilityDictationContextChecking,
             "ExperimentalAccessibilityDictationContextChecking",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsExperimentalAccessibilityDictationContextCheckingEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kExperimentalAccessibilityDictationContextChecking);
}

BASE_FEATURE(kAccessibilityMagnifierFollowsChromeVox,
             "AccessibilityMagnifierFollowsChromeVox",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityMagnifierFollowsChromeVoxEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityMagnifierFollowsChromeVox);
}

BASE_FEATURE(kAccessibilityMouseKeys,
             "AccessibilityMouseKeys",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityMouseKeysEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityMouseKeys);
}

BASE_FEATURE(kAccessibilityDisableTouchpad,
             "AccessibilityDisableTouchpad",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityDisableTouchpadEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityDisableTouchpad);
}

BASE_FEATURE(kAccessibilityFlashScreenFeature,
             "AccessibilityFlashScreenFeature",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityFlashScreenFeatureEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityFlashScreenFeature);
}

BASE_FEATURE(kAccessibilityBounceKeys,
             "AccessibilityBounceKeys",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityBounceKeysEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityBounceKeys);
}

BASE_FEATURE(kAccessibilitySlowKeys,
             "AccessibilitySlowKeys",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilitySlowKeysEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilitySlowKeys);
}

BASE_FEATURE(kAccessibilityShakeToLocate,
             "AccessibilityShakeToLocate",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityShakeToLocateEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityShakeToLocate);
}

BASE_FEATURE(kAccessibilityManifestV3EnhancedNetworkTts,
             "AccessibilityManifestV3EnhancedNetworkTts",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityManifestV3EnabledForEnhancedNetworkTts() {
  return base::FeatureList::IsEnabled(
      ::features::kAccessibilityManifestV3EnhancedNetworkTts);
}

#endif  // BUILDFLAG(IS_CHROMEOS)

#if !BUILDFLAG(IS_ANDROID)
bool IsScreenAIMainContentExtractionEnabled() {
  return base::FeatureList::IsEnabled(
      ax::mojom::features::kScreenAIMainContentExtractionEnabled);
}

bool IsScreenAIOCREnabled() {
  return base::FeatureList::IsEnabled(ax::mojom::features::kScreenAIOCREnabled);
}

BASE_FEATURE(kAccessibilityService,
             "AccessibilityService",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsAccessibilityServiceEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityService);
}

// This feature is only used for generating training data for Screen2x and
// should never be used in any other circumstance, and should not be enabled by
// default.
BASE_FEATURE(kDataCollectionModeForScreen2x,
             "DataCollectionModeForScreen2x",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsDataCollectionModeForScreen2xEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kDataCollectionModeForScreen2x);
}

BASE_FEATURE(kMainNodeAnnotations,
             "MainNodeAnnotations",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsMainNodeAnnotationsEnabled() {
  return base::FeatureList::IsEnabled(::features::kMainNodeAnnotations);
}

BASE_FEATURE(kReadAnythingReadAloud,
             "ReadAnythingReadAloud",
#if BUILDFLAG(IS_CHROMEOS)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif  // BUILDFLAG(IS_CHROMEOS)
);

bool IsReadAnythingReadAloudEnabled() {
  return base::FeatureList::IsEnabled(::features::kReadAnythingReadAloud);
}

BASE_FEATURE(kReadAnythingReadAloudPhraseHighlighting,
             "ReadAnythingReadAloudPhraseHighlighting",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsReadAnythingReadAloudPhraseHighlightingEnabled() {
  return base::FeatureList::IsEnabled(::features::kReadAnythingReadAloud) &&
         base::FeatureList::IsEnabled(
             ::features::kReadAnythingReadAloudPhraseHighlighting);
}

BASE_FEATURE(kReadAnythingWithScreen2x,
             "ReadAnythingWithScreen2x",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsReadAnythingWithScreen2xEnabled() {
  return base::FeatureList::IsEnabled(::features::kReadAnythingWithScreen2x);
}

BASE_FEATURE(kReadAnythingWithAlgorithm,
             "ReadAnythingWithAlgorithm",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsReadAnythingWithAlgorithmEnabled() {
  return base::FeatureList::IsEnabled(::features::kReadAnythingWithAlgorithm);
}

BASE_FEATURE(kReadAnythingImagesViaAlgorithm,
             "ReadAnythingImagesViaAlgorithm",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsReadAnythingImagesViaAlgorithmEnabled() {
  return base::FeatureList::IsEnabled(
             ::features::kReadAnythingImagesViaAlgorithm) &&
         IsReadAnythingWithAlgorithmEnabled();
}

BASE_FEATURE(kReadAnythingDocsIntegration,
             "ReadAnythingDocsIntegration",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsReadAnythingDocsIntegrationEnabled() {
  return base::FeatureList::IsEnabled(
      ax::mojom::features::kReadAnythingDocsIntegration);
}

BASE_FEATURE(kReadAnythingDocsLoadMoreButton,
             "ReadAnythingDocsLoadMoreButton",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsReadAnythingDocsLoadMoreButtonEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kReadAnythingDocsLoadMoreButton);
}

// This feature is only for debug purposes and for security/privacy reasons,
// should be never enabled by default .
BASE_FEATURE(kScreenAIDebugMode,
             "ScreenAIDebugMode",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsScreenAIDebugModeEnabled() {
  return base::FeatureList::IsEnabled(::features::kScreenAIDebugMode);
}

// This feature is only used in tests and must not be enabled by default.
BASE_FEATURE(kScreenAITestMode,
             "ScreenAITestMode",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsScreenAITestModeEnabled() {
  return base::FeatureList::IsEnabled(::features::kScreenAITestMode);
}

#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_MAC)
// Enables the switchover to the newer NSAccessibility property-based API.
BASE_FEATURE(kMacAccessibilityAPIMigration,
             "MacAccessibilityAPIMigration",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsMacAccessibilityAPIMigrationEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kMacAccessibilityAPIMigration);
}

BASE_FEATURE(kMacAccessibilityOptimizeChildrenChanged,
             "MacAccessibilityOptimizeChildrenChanged",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool IsMacAccessibilityOptimizeChildrenChangedEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kMacAccessibilityOptimizeChildrenChanged);
}

BASE_FEATURE(kAccessibilityRemoteUIApp,
             "AccessibilityRemoteUIApp",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsAccessibilityRemoteUIAppEnabled() {
  return base::FeatureList::IsEnabled(::features::kAccessibilityRemoteUIApp);
}

BASE_FEATURE(kBlockRootWindowAccessibleNameChangeEvent,
             "BlockRootWindowAccessibleNameChangeEvent",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsBlockRootWindowAccessibleNameChangeEventEnabled() {
  return base::FeatureList::IsEnabled(
      ::features::kBlockRootWindowAccessibleNameChangeEvent);
}
#endif  // BUILDFLAG(IS_MAC)

}  // namespace features

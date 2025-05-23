// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Define all the base::Features used by ui/accessibility.
#ifndef UI_ACCESSIBILITY_ACCESSIBILITY_FEATURES_H_
#define UI_ACCESSIBILITY_ACCESSIBILITY_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_base_export.h"

// This file declares base::Features related to the ui/accessibility code.
//
// If your flag is for all platforms, include it in the first section. If
// the flag is build-specific, include it in the appropriate section or
// add a new section if needed.
//
// Keep all sections ordered alphabetically.
//
// Include the base declaration, and a bool "is...Enabled()" getter for
// convenience and consistency. The bool method does not need a comment. Place a
// comment above the feature flag describing what the flag does when enabled.
// For example, a new entry should look like:
//
//    // <<effect of the experiment>>
//    AX_BASE_EXPORT BASE_DECLARE_FEATURE(kNewFeature);
//    AX_BASE_EXPORT bool IsNewFeatureEnabled();
//
// In the .cc file, a corresponding new entry should look like:
//
//    BASE_FEATURE(kNewFeature, "NewFeature",
//    base::FEATURE_DISABLED_BY_DEFAULT); bool IsNewFeatureEnabled() {
//      return base::FeatureList::IsEnabled(::features::kNewFeature);
//    }
//
// Your feature name should start with "kAccessibility". There is no need to
// include the words "enabled" or "experimental", as these are implied. We
// include accessibility to differentiate these features from others in
// Chromium.

namespace features {

// Enable PDF OCR for Select-to-Speak. It will be disabled by default on
// platforms other than ChromeOS as STS is available only on ChromeOS.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityPdfOcrForSelectToSpeak);
AX_BASE_EXPORT bool IsAccessibilityPdfOcrForSelectToSpeakEnabled();

// A replacement algorithm for AbstractInlineTextBox + InlineCursor for
// AXInlineTextBox creation.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityBlockFlowIterator);
AX_BASE_EXPORT bool IsAccessibilityBlockFlowIteratorEnabled();

AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityPruneRedundantInlineText);
AX_BASE_EXPORT bool IsAccessibilityPruneRedundantInlineTextEnabled();

AX_BASE_EXPORT BASE_DECLARE_FEATURE(
    kAccessibilityPruneRedundantInlineConnectivity);
AX_BASE_EXPORT bool IsAccessibilityPruneRedundantInlineConnectivityEnabled();

// Use Alternative mechanism for acquiring image descriptions.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kImageDescriptionsAlternateRouting);
AX_BASE_EXPORT bool IsImageDescriptionsAlternateRoutingEnabled();

// Disable the accessibility engine after a certain
// number of user input events spanning a minimum amount of time with no
// accessibility API usage in that time.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAutoDisableAccessibility);
AX_BASE_EXPORT bool IsAutoDisableAccessibilityEnabled();

// Recognize "aria-virtualcontent" as a valid aria property.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kEnableAccessibilityAriaVirtualContent);
AX_BASE_EXPORT bool IsAccessibilityAriaVirtualContentEnabled();

// Expose <summary>" as a heading instead of a button.
// Two reasons to try this:
// 1. Unlike for a button, JAWS will not enforce leafiness for a heading, so
// that things like child links will still be presented to the user.
// 2. The user can use heading navigation for summaries.
// We may decide to scale this back for use cases such as a summary inside of
// a table or a list.
// Experiment until we validate the approach with ATs and ARIA WG.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityExposeSummaryAsHeading);
AX_BASE_EXPORT bool IsAccessibilityExposeSummaryAsHeadingEnabled();

// Use language detection to determine the language
// of text content in page and exposed to the browser process AXTree.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kEnableAccessibilityLanguageDetection);
AX_BASE_EXPORT bool IsAccessibilityLanguageDetectionEnabled();

// Restrict AXModes to web content related modes only when an IA2
// query is performed on a web content node.
// TODO(crbug.com/40266474): Remove flag once the change has been confirmed
// safe.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kEnableAccessibilityRestrictiveIA2AXModes);
AX_BASE_EXPORT bool IsAccessibilityRestrictiveIA2AXModesEnabled();

// Extension manifest v3 migration for network speech synthesis.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kExtensionManifestV3NetworkSpeechSynthesis);
AX_BASE_EXPORT bool IsExtensionManifestV3NetworkSpeechSynthesisEnabled();

// Support aria element reflection. For example:
//     element.ariaActiveDescendantElement = child;
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kEnableAriaElementReflection);
AX_BASE_EXPORT bool IsAriaElementReflectionEnabled();

// Turn on browser vocalization of 'descriptions' tracks.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kTextBasedAudioDescription);
AX_BASE_EXPORT bool IsTextBasedAudioDescriptionEnabled();

// Expose document markers on inline text boxes in addition to
// static nodes. (Note: This will make it possible for AXPosition in the browser
// process to handle document markers, which will be platform agnositc)
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kUseAXPositionForDocumentMarkers);
AX_BASE_EXPORT bool IsUseAXPositionForDocumentMarkersEnabled();

#if BUILDFLAG(IS_WIN)
// Use Chrome-specific accessibility COM API.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kIChromeAccessible);
AX_BASE_EXPORT bool IsIChromeAccessibleEnabled();

// Selectively enable accessibility depending on the
// UIA APIs that are called. (Note: This will make it possible for
// non-screenreader services to enable less of the accessibility system)
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kSelectiveUIAEnablement);
AX_BASE_EXPORT bool IsSelectiveUIAEnablementEnabled();

// Use the browser's UIA provider when requested by
// an accessibility client.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kUiaProvider);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_CHROMEOS)
// TODO(accessibility): Should this be moved to ash_features.cc?
AX_BASE_EXPORT bool IsDictationOfflineAvailable();

// Adds option to enable Accessibility accelerator.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityAccelerator);
AX_BASE_EXPORT bool IsAccessibilityAcceleratorEnabled();

// Adds option to limit the movement on the screen.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityReducedAnimations);
AX_BASE_EXPORT bool IsAccessibilityReducedAnimationsEnabled();

// Integrate with FaceGaze.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityFaceGaze);
AX_BASE_EXPORT bool IsAccessibilityFaceGazeEnabled();

// Adds reduced animations toggle to kiosk quick settings.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityReducedAnimationsInKiosk);
AX_BASE_EXPORT bool IsAccessibilityReducedAnimationsInKioskEnabled();

// Allow context checking with the accessibility Dictation
// feature.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(
    kExperimentalAccessibilityDictationContextChecking);
AX_BASE_EXPORT bool
IsExperimentalAccessibilityDictationContextCheckingEnabled();

// Download Google TTS High Quality voices.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(
    kExperimentalAccessibilityGoogleTtsHighQualityVoices);
AX_BASE_EXPORT bool
IsExperimentalAccessibilityGoogleTtsHighQualityVoicesEnabled();

// Whether the screen magnifier can follow the ChromeVox focus.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityMagnifierFollowsChromeVox);
AX_BASE_EXPORT bool IsAccessibilityMagnifierFollowsChromeVoxEnabled();

// Control mouse with keyboard.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityMouseKeys);
AX_BASE_EXPORT bool IsAccessibilityMouseKeysEnabled();

// Controls whether the shake cursor to locate feature is available.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityShakeToLocate);
AX_BASE_EXPORT bool IsAccessibilityShakeToLocateEnabled();

// Controls whether the disable touchpad feature is enabled.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityDisableTouchpad);
AX_BASE_EXPORT bool IsAccessibilityDisableTouchpadEnabled();

// Controls whether the flash screen for notifications feature is available.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityFlashScreenFeature);
AX_BASE_EXPORT bool IsAccessibilityFlashScreenFeatureEnabled();

// Controls whether the bounce keys feature is available.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityBounceKeys);
AX_BASE_EXPORT bool IsAccessibilityBounceKeysEnabled();

// Controls whether the slow keys feature is available.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilitySlowKeys);
AX_BASE_EXPORT bool IsAccessibilitySlowKeysEnabled();

AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityManifestV3EnhancedNetworkTts);
AX_BASE_EXPORT bool IsAccessibilityManifestV3EnabledForEnhancedNetworkTts();

#endif  // BUILDFLAG(IS_CHROMEOS)

#if !BUILDFLAG(IS_ANDROID)
// Use the experimental Accessibility Service.
// TODO(katydek): Provide a more descriptive name here.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityService);
AX_BASE_EXPORT bool IsAccessibilityServiceEnabled();

// Open Read Anything side panel when the browser is opened, and
// call distill after the navigation's load-complete event. (Note: The browser
// is only being opened to render one webpage, for the sake of generating
// training data for Screen2x data collection. The browser is intended to be
// closed by the user who launches Chrome once the first distill call finishes
// executing.)
//
// Note: This feature should be used along with 'ScreenAIDebugModeEnabled=true'
// and --no-sandbox.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kDataCollectionModeForScreen2x);
AX_BASE_EXPORT bool IsDataCollectionModeForScreen2xEnabled();

// Identify and annotate the main node of the AXTree where one was not already
// provided.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kMainNodeAnnotations);
AX_BASE_EXPORT bool IsMainNodeAnnotationsEnabled();

// Show the Read Aloud feature in Read Anything.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingReadAloud);
AX_BASE_EXPORT bool IsReadAnythingReadAloudEnabled();

// Enable phrase highlighting in Read Anything Read Aloud.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingReadAloudPhraseHighlighting);
AX_BASE_EXPORT bool IsReadAnythingReadAloudPhraseHighlightingEnabled();

// Use screen2x integration for Read Anything to distill web pages
// using an ML model.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingWithScreen2x);
AX_BASE_EXPORT bool IsReadAnythingWithScreen2xEnabled();

// Enable rules based algorithm for distilling content. Should be enabled by
// default.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingWithAlgorithm);
AX_BASE_EXPORT bool IsReadAnythingWithAlgorithmEnabled();

// Enable images to be distilled via algorithm. Should be disabled by
// default.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingImagesViaAlgorithm);
AX_BASE_EXPORT bool IsReadAnythingImagesViaAlgorithmEnabled();

// Enable Reading Mode to work on Google Docs. Should be disabled by default.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingDocsIntegration);
AX_BASE_EXPORT bool IsReadAnythingDocsIntegrationEnabled();

// Enable "load more" button to show at the end of Reading Mode panel.
// Should be disabled by default.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kReadAnythingDocsLoadMoreButton);
AX_BASE_EXPORT bool IsReadAnythingDocsLoadMoreButtonEnabled();

// Write some ScreenAI library debug data in /tmp.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kScreenAIDebugMode);
AX_BASE_EXPORT bool IsScreenAIDebugModeEnabled();

// ScreenAI library's Main Content Extraction service is enabled.
AX_BASE_EXPORT bool IsScreenAIMainContentExtractionEnabled();

// ScreenAI library's OCR service is enabled.
AX_BASE_EXPORT bool IsScreenAIOCREnabled();

// Enables to use the Screen AI component available for testing.
// If enabled, ScreenAI library will be loaded from //third_party/screen-ai.
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kScreenAITestMode);
AX_BASE_EXPORT bool IsScreenAITestModeEnabled();

#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_MAC)
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kMacAccessibilityAPIMigration);
AX_BASE_EXPORT bool IsMacAccessibilityAPIMigrationEnabled();

AX_BASE_EXPORT BASE_DECLARE_FEATURE(kMacAccessibilityOptimizeChildrenChanged);
AX_BASE_EXPORT bool IsMacAccessibilityOptimizeChildrenChangedEnabled();

// Set NSAccessibilityRemoteUIElement's RemoteUIApp to YES to fix
// some accessibility bugs in PWA Mac. (Note: When enabling
// NSAccessibilityRemoteUIElement's RemoteUIApp previously, chromium would hang.
// See: https://crbug.com/1491329).
AX_BASE_EXPORT BASE_DECLARE_FEATURE(kAccessibilityRemoteUIApp);
AX_BASE_EXPORT bool IsAccessibilityRemoteUIAppEnabled();

AX_BASE_EXPORT BASE_DECLARE_FEATURE(kBlockRootWindowAccessibleNameChangeEvent);
AX_BASE_EXPORT bool IsBlockRootWindowAccessibleNameChangeEventEnabled();
#endif  // BUILDFLAG(IS_MAC)

}  // namespace features

#endif  // UI_ACCESSIBILITY_ACCESSIBILITY_FEATURES_H_

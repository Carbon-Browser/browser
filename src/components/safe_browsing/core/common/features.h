// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_CORE_COMMON_FEATURES_H_
#define COMPONENTS_SAFE_BROWSING_CORE_COMMON_FEATURES_H_

#include <stddef.h>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/values.h"

namespace safe_browsing {
// Features list
//
// sticky_prefixes attach to the lines after them, and group_prefixes
// attach to the lines before them. These prefixes were chosen to sort
// by feature name. See https://github.com/google/keep-sorted for
// detailed semantics.
//
// keep-sorted start sticky_prefixes=["#if"] group_prefixes=["#else", "#endif", "extern const base::FeatureParam"] newline_separated=yes

// Controls various parameters related to occasionally collecting ad samples,
// for example to control how often collection should occur.
BASE_DECLARE_FEATURE(kAdSamplerTriggerFeature);

#if BUILDFLAG(IS_ANDROID)
// Enables adding an Android app referrer to Protego pings.
BASE_DECLARE_FEATURE(kAddReferringAppInfoToProtegoPings);
#endif

// Enables adding warning shown timestamp to client safe browsing report.
BASE_DECLARE_FEATURE(kAddWarningShownTSToClientSafeBrowsingReport);

// Expand allowlist usage beyond CSPP allowlist by using the high confidence
// allowlist as well.
BASE_DECLARE_FEATURE(kClientSideDetectionAcceptHCAllowlist);

// Create a response containing the brand and the intent of the page using the
// on-device model LLM.
BASE_DECLARE_FEATURE(kClientSideDetectionBrandAndIntentForScamDetection);

BASE_DECLARE_FEATURE(kClientSideDetectionDebuggingMetadataCache);

// Killswitch for client side phishing detection. Since client side models are
// run on a large fraction of navigations, crashes due to the model are very
// impactful, even if only a small fraction of users have a bad version of the
// model. This Finch flag allows us to remediate long-tail component versions
// while we fix the root cause. This will also halt the model distribution from
// OptimizationGuide.
BASE_DECLARE_FEATURE(kClientSideDetectionKillswitch);

// Expand CSPP beyond phishing and trigger when keyboard or pointer lock request
// occurs on the page.
BASE_DECLARE_FEATURE(kClientSideDetectionKeyboardPointerLockRequest);

// Expand CSD-Phishing beyond phishing and trigger when a notification prompt
// occurs on the page.
BASE_DECLARE_FEATURE(kClientSideDetectionNotificationPrompt);

// Send a sample CSPP ping when a URL matches the CSD allowlist and all other
// preclassification check conditions pass.
BASE_DECLARE_FEATURE(kClientSideDetectionSamplePing);

// Show a warning to the user that factors in the IntelligentScanVerdict from
// ClientPhishingResponse.
BASE_DECLARE_FEATURE(kClientSideDetectionShowScamVerdictWarning);

// Expand CSPP beyond phishing and trigger when vibration API is called on the
// web page.
BASE_DECLARE_FEATURE(kClientSideDetectionVibrationApi);

// Set a RESIZE_BEST preference for image resizing algorithm in Client Side
// Detection renderer processes for both image classification and image
// embedding. This experiment is done to see if the resizing algorithm
// preference will send clearer screenshots for server side evaluation.
BASE_DECLARE_FEATURE(kConditionalImageResize);

// Creates and sends CSBRRs when notification permissions are accepted for an
// abusive site whose interstitial has been bypassed.
BASE_DECLARE_FEATURE(kCreateNotificationsAcceptedClientSafeBrowsingReports);

// Creates and sends CSBRRs when warnings are first shown to users.
BASE_DECLARE_FEATURE(kCreateWarningShownClientSafeBrowsingReports);

// Enables the interstitial warning prompt on dangerous downloads. This replaces
// the current prompt which is a dialog/modal.
BASE_DECLARE_FEATURE(kDangerousDownloadInterstitial);

// Controls whether we use new broader criteria for deep scans.
BASE_DECLARE_FEATURE(kDeepScanningCriteria);

// Controls whether we prompt the user on unencrypted deep scans.
BASE_DECLARE_FEATURE(kDeepScanningPromptRemoval);

// Controls whether the delayed warning experiment is enabled.
BASE_DECLARE_FEATURE(kDelayedWarnings);
// True if mouse clicks should undelay the warnings immediately when delayed
// warnings feature is enabled.
extern const base::FeatureParam<bool> kDelayedWarningsEnableMouseClicks;

// Sends the WebProtect content scanning request to the corresponding regional
// DLP endpoint based on ChromeDataRegionSetting policy.
BASE_DECLARE_FEATURE(kDlpRegionalizedEndpoints);

// Show referrer URL on download item on chrome://downloads page. This will
// replace the downloads url.
BASE_DECLARE_FEATURE(kDownloadsPageReferrerUrl);

// The kill switch for download tailored warnings. The main control is on the
// server-side.
BASE_DECLARE_FEATURE(kDownloadTailoredWarnings);

// Enables HaTS surveys for users encountering desktop download warnings on the
// download bubble or the downloads page.
BASE_DECLARE_FEATURE(kDownloadWarningSurvey);

// Gives the type of the download warning HaTS survey that the user is eligible
// for. This should be set in the fieldtrial config along with the trigger ID
// for the corresponding survey (as en_site_id). The int value corresponds to
// the value of DownloadWarningHatsType enum (see
// //c/b/download/download_warning_desktop_hats_util.h).
extern const base::FeatureParam<int> kDownloadWarningSurveyType;

// The time interval after which to consider a download warning ignored, and
// potentially show the survey for ignoring a download bubble warning.
extern const base::FeatureParam<int> kDownloadWarningSurveyIgnoreDelaySeconds;

// Enables Enhanced Safe Browsing promos for iOS.
BASE_DECLARE_FEATURE(kEnhancedSafeBrowsingPromo);

// Enables showing an updated Password Reuse UI for enterprise users.
BASE_DECLARE_FEATURE(kEnterprisePasswordReuseUiRefresh);

// When on, enterprise policy EnterpriseRealTimeUrlCheckMode on Android is
// supported.
BASE_DECLARE_FEATURE(kEnterpriseRealTimeUrlCheckOnAndroid);

// Enables string update on the enhanced protection description on
// chrome://settings/security to mention the use of AI.
BASE_DECLARE_FEATURE(kEsbAiStringUpdate);

// Makes the Enhanced Protection a syncable setting.
// Check the design doc (go/esb-as-a-synced-setting-dd) for further details.
BASE_DECLARE_FEATURE(kEsbAsASyncedSetting);

// Controls whether Safe Browsing Extended Reporting (SBER) is deprecated.
// When this feature flag is enabled:
// - the Extended Reporting toggle will not be displayed on
//   chrome://settings/security
// - features will not depend on the SBER preference value,
//   safebrowsing.scout_reporting_enabled
BASE_DECLARE_FEATURE(kExtendedReportingRemovePrefDependency);

// Allows the Extension Telemetry Service to accept and use configurations
// sent by the server.
BASE_DECLARE_FEATURE(kExtensionTelemetryConfiguration);

// Enables collection of telemetry signal whenever an extension invokes the
// declarativeNetRequest actions.
BASE_DECLARE_FEATURE(kExtensionTelemetryDeclarativeNetRequestActionSignal);

// Allows the Extension Telemetry Service to include file data of extensions
// specified in the --load-extension commandline switch in telemetry reports.
BASE_DECLARE_FEATURE(kExtensionTelemetryFileDataForCommandLineExtensions);

// Enables the telemetry service to collect signals and generate reports to send
// for enterprise.
BASE_DECLARE_FEATURE(kExtensionTelemetryForEnterprise);

// Specifies the reporting interval for enterprise telemetry reports.
extern const base::FeatureParam<int>
    kExtensionTelemetryEnterpriseReportingIntervalSeconds;

// Enables collection of telemetry signal whenever an extension invokes the
// tabs.executeScript API call.
BASE_DECLARE_FEATURE(kExtensionTelemetryTabsExecuteScriptSignal);

// Enables intercepting remote hosts contacted by extensions in renderer
// throttles.
BASE_DECLARE_FEATURE(
    kExtensionTelemetryInterceptRemoteHostsContactedInRenderer);

// Enables reporting of external app redirects
BASE_DECLARE_FEATURE(kExternalAppRedirectTelemetry);

// Whether to provide Google Play Protect status in APK telemetry pings
BASE_DECLARE_FEATURE(kGooglePlayProtectInApkTelemetry);

// Whether Google Play Protect should supercede file-type warnings
BASE_DECLARE_FEATURE(kGooglePlayProtectReducesWarnings);

// Sends hash-prefix real-time lookup requests on navigations for Standard Safe
// Browsing users instead of hash-prefix database lookups.
BASE_DECLARE_FEATURE(kHashPrefixRealTimeLookups);

// This parameter controls the relay URL that will forward the lookup requests
// to the Safe Browsing server.
extern const base::FeatureParam<std::string> kHashPrefixRealTimeLookupsRelayUrl;

// Enable faster OHTTP key rotation for hash-prefix real-time lookups.
BASE_DECLARE_FEATURE(kHashPrefixRealTimeLookupsFasterOhttpKeyRotation);

// Send sample hash-prefix real-time lookups for real-time lookups to catch
// "false positives" where real-time lookup says safe but hash-prefix lookup
// says unsafe.
// Check the design doc (go/sample-esb-ping-send-hprt) for further
// details.
BASE_DECLARE_FEATURE(kHashPrefixRealTimeLookupsSamplePing);
// Determines the percentage of ESB lookups that we sample to send a background
// HPRT lookup. The value should be between 0 and 100.
extern const base::FeatureParam<int> kHashPrefixRealTimeLookupsSampleRate;

// If enabled, fetching lists from Safe Browsing and performing checks on those
// lists uses the v5 APIs instead of the v4 Update API. There is no change to
// how often the checks are triggered (they are still not in real time).
BASE_DECLARE_FEATURE(kLocalListsUseSBv5);

// Killswitch for fetching and executing the notification content detection
// model. This also gates logging metrics related to this model.
BASE_DECLARE_FEATURE(kOnDeviceNotificationContentDetectionModel);
// Determines the percentage of notifications from allowlisted sites that we
// will check the model for. The value should be between 0 and 100.
extern const base::FeatureParam<int>
    kOnDeviceNotificationContentDetectionModelAllowlistSamplingRate;

// Enable movement of password leak toggle out of standard protection and into
// its own section.
BASE_DECLARE_FEATURE(kPasswordLeakToggleMove);

// Enables HaTS surveys for users encountering red warnings.
BASE_DECLARE_FEATURE(kRedWarningSurvey);

// Specifies whether we want to show HaTS surveys based on if the user bypassed
// the warning or not. Note: specifying any combination of TRUE and FALSE
// corresponds to "don't care."
extern const base::FeatureParam<std::string> kRedWarningSurveyDidProceedFilter;

// Specifies which CSBRR report types (and thus, red warning types) we want to
// show HaTS surveys for.
extern const base::FeatureParam<std::string> kRedWarningSurveyReportTypeFilter;

// Specifies the HaTS survey's identifier.
extern const base::FeatureParam<std::string> kRedWarningSurveyTriggerId;

// Controls whether asynchronous real-time check is enabled. When enabled, the
// navigation can be committed before real-time Safe Browsing check is
// completed.
BASE_DECLARE_FEATURE(kSafeBrowsingAsyncRealTimeCheck);

// Enables client side phishing daily reports limit to be configured via Finch
// for ESB and SBER users
BASE_DECLARE_FEATURE(kSafeBrowsingDailyPhishingReportsLimit);

// Specifies the CSD-Phishing daily reports limit for ESB users
extern const base::FeatureParam<int> kSafeBrowsingDailyPhishingReportsLimitESB;

// Enables new ESB specific threshold fields in Visual TF Lite model files
BASE_DECLARE_FEATURE(kSafeBrowsingPhishingClassificationESBThreshold);

// Controls whether cookies are removed when the access token is present.
BASE_DECLARE_FEATURE(kSafeBrowsingRemoveCookiesInAuthRequests);

#if BUILDFLAG(IS_ANDROID)
// Enables sync checker to check allowlist first on Chrome on Android. This is
// an optimization to improve the speed of Safe Browsing checks.
// See go/skip-sync-hpd-allowlist-android for details.
BASE_DECLARE_FEATURE(kSafeBrowsingSyncCheckerCheckAllowlist);
#endif

// Automatically revoke abusive notifications in Safety Hub.
BASE_DECLARE_FEATURE(kSafetyHubAbusiveNotificationRevocation);

// Enables saving gaia password hash from the Profile Picker sign-in flow.
BASE_DECLARE_FEATURE(kSavePasswordHashFromProfilePicker);

// Enables replacing notification contents with a Chrome warning when the
// on-device model returns a sufficiently suspicious verdict.
BASE_DECLARE_FEATURE(kShowWarningsForSuspiciousNotifications);
// Determines the minimum "suspicious" score returned from the notification
// content LiteRT model that warrants showing a warning. If the score is higher
// than this threshold, then the notification contents will be replaced with a
// warning. By default, no notifications will be replaced by a warning.
extern const base::FeatureParam<int>
    kShowWarningsForSuspiciousNotificationsScoreThreshold;

// Controls the daily quota for the suspicious site trigger.
BASE_DECLARE_FEATURE(kSuspiciousSiteTriggerQuotaFeature);

// Controls whether the integration of tailored security settings is enabled.
BASE_DECLARE_FEATURE(kTailoredSecurityIntegration);

// Specifies which non-resource HTML Elements to collect based on their tag and
// attributes. It's a single param containing a comma-separated list of pairs.
// For example: "tag1,id,tag1,height,tag2,foo" - this will collect elements with
// tag "tag1" that have attribute "id" or "height" set, and elements of tag
// "tag2" if they have attribute "foo" set. All tag names and attributes should
// be lower case.
BASE_DECLARE_FEATURE(kThreatDomDetailsTagAndAttributeFeature);

// Controls the behavior of visual features in CSD pings. This feature is
// checked for the final size of the visual features and the minimum size of
// the screen.
BASE_DECLARE_FEATURE(kVisualFeaturesSizes);

// keep-sorted end

base::Value::List GetFeatureStatusList();

}  // namespace safe_browsing
#endif  // COMPONENTS_SAFE_BROWSING_CORE_COMMON_FEATURES_H_

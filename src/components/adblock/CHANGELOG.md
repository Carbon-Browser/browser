# Release Notes

## eyeo Chromium SDK 105.1
* Fixed faulty handling URL redirection by creating AdblockURLLoaderFactory as the last proxy

## eyeo Chromium SDK 105.0
* Updated to Chromium 105.0.5195.68
* Rebranded to eyeo Chromium SDK (DPD-1322)
* Moved snippet update to gclient sync (DPD-1283)
* Updated extensions API:
  - Added `onPopupAllowed` event which is fired when a popup is allowlisted
  - Added `onPopupBlocked` event which is fired when a popup is blocked
  - Added `onPageAllowed` event which is fired when a whole domain is allowlisted
  - Fixed typo in `OnSubscriptionUpdated` which is now `onSubscriptionUpdated`
* Updated Java AdBlockedObserver API:
  - Removed `onAdMatched` and instead added `onAdAllowed` and `onAdBlocked`
  - Removed `onPopupMatched` and instead added `onPopupAllowed` and `onPopupBlocked`
  - Added `onPageAllowed` callback which is fired when a whole domain is allowlisted

## Chromium 104.0 + ABP 2.1 (ABP Chromium 104 v1)
* Updated to Chromium 104.0.5112.55
* Added support for :not in element hiding (DPD-1106)
* Fixed bug which caused that language specific list was not enabled by default in 1st run scenario (DPD-1302)
* Moved waiting for SubscriptionService to be initialized from ResourceClassifier to mojo (DPD-1303)
* Classification logic split into chrome-specific and chrome-agnostic part (DPD-1304)
* Moved checking ABP enabled state to AdblockContentBrowserClient (DPD-1334)
* Disabled flaky RecordNotificationDisplayedAndInteraction test

## Chromium 103.0 + ABP 2.1 (ABP Chromium 103 v1)
* Updated to Chromium 103.0.5060.53
* Added rewrite filter type support (DPD-810)
* Exposed filter list version, status and installation time and updated documentation in adblock_private.idl (DPD-870)
* Fixed parsing regex filter as a filter option (DPD-1209)
* Fixed parsing wildcard filters
* Updated sequence diagrams in documentation (components/adblock/docs)
* Fixed issues in AdblockController Java code which could lead to crashes when Profile is not yet initialized (DPD-1288)
* Moved CSP injection and ad blocking into AdblockURLLoaderFactory and removed AdblockUrlLoaderThrottle (DPD-1238, DPD-1263)
* Splitted ABP core codebase in namespaces (DPD-1258)
* Several minor code cleanups and refactorings
* Removed hardcoding fieldtrial_testing_enabled=false flag in BUILD.gn

## Chromium 102.0 + ABP 2.1 (ABP Chromium 102 v1)
* Updated to Chromium 102.0.5005.50
* Added header filter support (DPD-1103)
* Added Java API to get subscription version
* Improved matching performance for long URLs (DPD-419)
* Added support for `! Redirect` metadata in subscription header (DPD-965)
* Fixed parsing of `! Expires` metadata in subscription header (DPD-965)

## Chromium 101.0 + ABP 2.0 (ABP Chromium 101 v1)
* Updated to Chromium 101.0.4951.41
* Element hiding CSS sanitized before injection (DPD-1010)
* Ping filter support (DPD-1102)
* Allow defining default and privileged subscriptions via configuration file (DPD-1161, DPD-1205)
* Restructure adblock component to fit Chromium Layered Components design (DPD-1165)
* Updated in-repo documentation (DPD-1176)
* Allow disabling adblocking via feature flag for testing (DPD-1172)
* Add java API to get subscription version

## Chromium 100.0 + ABP 2.0 (ABP Chromium 100 v1)
* Updated to Chromium 100.0.4896.46
* Replaced libadblockplus with a native, flatbuffer-based implementation of ad-filtering logic
* Improved memory consumption considerably
* Improved startup time considerably
* Removed V8 dependency from browser process
* Enabled preloaded filter lists for out-of-the-box ad filtering
* Counting active users by sending periodic Telemetry pings with no user-identifiable data
* Optimized injecting Snippets and Element Hiding Emulation JS
* Added support for CSP filters
* Updated in-repo documentation (components/adblock/docs)

Known issues:
* The following browser tests are failing and will be fixed in future releases:
  - ClientHintsBrowserTest.DelegateAndMerge_HttpEquiv
  - ClientHintsBrowserTest.DelegateAndMerge_MetaName
  - ClientHintsBrowserTest.DelegateToBar_HttpEquiv
  - ClientHintsBrowserTest.DelegateToBar_MetaName
  - ClientHintsBrowserTest.DelegateToFoo_HttpEquiv
  - ClientHintsBrowserTest.DelegateToFoo_MetaName
  - ExtensionWebRequestApiTest.WebRequestBlocking

## Chromium 99.0 + ABP 1.3 (ABP Chromium 99 v1)
* Updated to Chromium 99.0.4844.35
* Updated libadblockplus to version 13.1-c0.5.1
* Enabled command line preferences for desktop builds (DPD-1088)
* Added statistics API to expose runtime counters (DPC-610)
* Extended JS API to receive notification when a subscription gets updated (DPC-694)

Known issues:
* Due to the V8 dependency, the following browser tests are still failing, and will be fixed in future releases:
  - PageTextObserverSingleProcessBrowserTest.SameProcessAMPSubframe
  - PageTextObserverSingleProcessBrowserTest.SameProcessIframe
  - SingleProcessBrowserTest.Test

## Chromium 98.0 + ABP 1.2 (ABP Chromium 98 v1)
* Updated libadblockplus to version 12.1-c0.3.0

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/12.0-c0.3.0...12.1-c0.3.0)

## Chromium 97.0 + ABP 1.1 (ABP Chromium 97 v1)
* Updated to Chromium 97.0.4692.45
* More fixes to make non-ABP test pass (DPD-866)
* Moved ABP Java code to `components/adblock` to allow other components to declare dependencies from this code (DPD-837)
  Package `org.chromium.chrome.browser.adblock` is changed to `org.chromium.components.adblock`
* `.ci-scripts/v8.patch` introduced in 94v1 is no longer required (DPD-794)

## Chromium 95.0 + ABP 1.0 (ABP Chromium 95 v1)
* Updated to Chromium 95.0.4638.50
* Removed unused AdblockTraceCall class and calls
* Fixed majority of non-ABP unit tests and browser tests (DPC-568)

Known issues:
* The following browser tests are still failing, and will be fixed later:
  - PersistentBackground/PermissionsApiTestWithContextType.OptionalPermissionsAutoConfirm/0
  - PersistentBackground/PermissionsApiTestWithContextType.OptionalPermissionsGranted/0
  - PageTextObserverSingleProcessBrowserTest.SameProcessAMPSubframe
  - PageTextObserverSingleProcessBrowserTest.SameProcessIframe
  - SingleProcessBrowserTest.Test

## Chromium 94.0 + ABP 0.24 (ABP Chromium 94 v1)
* Updated to Chromium 94.0.4606.50
* Solved assertion issue detected in Chromium 92
* Solved DCHECK issue detected in Chromium 93

Known issues:
* Debug builds sometimes hit DCHECK: https://bugs.chromium.org/p/chromium/issues/detail?id=1206694
* Build is unsuccessful in certain environments due to warnings in V8: https://bugs.chromium.org/p/chromium/issues/detail?id=1251165

  This issue can be circumvented by:

  `cd v8 ; git reset --hard ; git apply ../.ci-scripts/v8.patch ; cd ..`

## Chromium 93.0 + ABP 0.23 (ABP Chromium 93 v1)
* Updated to Chromium 93.0.4577.62
* Moved ABP-related translations to chrome/android/adblock to separate them from Chromium strings and avoid merging conflicts (DPD-696)
* Fixed DCHECK for wrong ConversionMeasurementAPIAlternativeUsage feature configuration in upstream (DPD-749)
* Fixed issue that blocked download PDF popup (DPD-742)
* Added snippet filters support (DPD-648)
* Added licensing header on chromium modified files (DPD-50)

Known issues:
* Debug builds running in specific x86 emulator hit DCHECK: https://bugs.chromium.org/p/chromium/issues/detail?id=1245583

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/10.0-c0.3.0...12.0-c0.3.0)

## Chromium 92.0 + ABP 0.22 (ABP Chromium 92 v1)
* Updated to Chromium 92.0.4515.105
* Fixed crash when browser closed before FilterEngine has loaded (DPD-578)
* Fixed crash when tab changed right after startup, before FilterEngine has loaded (DPD-367)
* Fixed downloading the exceptions list on first run despite acceptable ads being off (DPD-53)
* Replaced AdblockBridge with finer-grained classes: AdblockRequestClassifier and AdblockSitekeyStorage (DPD-611)
* Added Extension API for testing and automation tasks on desktop (DPD-636)
* Decrease priority for some adblocking-related background tasks for better startup experience (DPD-579)
* Fixed redundant HEAD requests for disabled subscriptions (DPD-590)
* Added class-level comments describing purpose and basic functionality description (DPD-400)
* Added sequence diagrams for various ABP lifecycle scenarious to the docs_abp folder
* Allow to override ABP application name & version from gn

Known issues:
* Debug builds running in x86 emulator crash due to spurious assertion in V8: https://bugs.chromium.org/p/chromium/issues/detail?id=1220335#c5

  The assertion can be quenched by:

  `cd v8 ; git reset --hard ; git apply ../.ci-scripts/v8.patch ; cd ..`

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/9.0-c0.3.0...10.0-c0.3.0)

## Chromium 91.0 + ABP 0.21 (ABP Chromium 91 v1)
* Updated to Chromium 91.0.4472.77
* Moved ad-blocking logic from the network service to the browser and renderer(s) (DPD-368)
* Moved all filter engine operations from AdblockController to AdblockBridge (DPD-284)
* Added caching of JS and CSS generated for element hiding and element hiding emu purposes (DPD-7)
* Updated libadblockplus to version 9.0-c0.3.0

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/8.1-c0.3.0...9.0-c0.3.0)

## Chromium 90.0 + ABP 0.20 (ABP Chromium 90 v1)
* Updated to Chromium 90.0.4430.66
* Fixed a crash due to LegacyPrefsMigration sometimes starting without Profile (DPD-301)
* Moved blocking/allowing WebSocket connections to ContentBrowserClient::CreateWebSocket (DPD-86)
* Refactored AdblockBridgeImpl SendAdAllowed and SendAdBlocked into single method SendAdMatched (DPD-279)
* Updated adblockpluscore to version 0.3.0
* Updated libadblockplus to version 8.1-c0.3.0

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/8.0-c0.2.2...8.1-c0.3.0)

## Chromium 89.0 + ABP 0.19 (ABP Chromium 89 v1)
* Updated to Chromium 89.0.4389.72
* Replaced deprecated base::ListValue with std::vector<base::Value> (DPD-26)
* Created removeCustomFilter() method in AdblockController (DPD-58)
* Removed ABP Network delegate (DPD-83)
* Removed Android dependency from `components/adblock` (DPD-128)
* Fixed Android Tests failing for builds with preloaded subscriptions (DPD-161)

Known issues:
* Some Android Tests are still failing for builds with preloaded subscriptions

## Chromium 88.0 + ABP 0.18 (ABP Chromium 88 v1)
* Updated to Chromium 88.0.4324.93
* Fixed user counting when Acceptable Ads are disabled (DP-2118)
* Fixed allowlisting in detached iframes (DP-2128)
* Moved the bulk of the implementation to components/adblock (DP-1445)
* Added dark-theme icons in Settings (DP-2054)
* Reduced coupling between AdblockBridge and AdblockController (DP-2072)
* Fixed potential random crashes in V8 (DP-2074)
* Updated adblockpluscore to version 0.2.2
* Updated libadblockplus to version 8.0-c0.2.2

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/3.1-c0.2.1...8.0-c0.2.2)

## Chromium 87.0 + ABP 0.17 (ABP Chromium 87 v1)
* Updated to Chromium 87.0.4280.66 (DP-1677)

## Chromium 86.0 + ABP 0.17 (ABP Chromium 86 v2)
* Allowlisting improvements:
  - Generated allowlisted domain rule limited to the specific domain only (DP-1533)
  - Adjusted allowlisting logic with Web Extension (DP-449)
  - Fixed frame hierarchy not including browser-initiated loads (DP-1764)
* AdblockController improvements:
  - Added APIs allowing to add a custom filter and check if a filter matches (DP-1577)
  - Made subscriptions-related APIs return `Subscription` object and aligned style with Java coding style (DP-1593, DP-1663)
* Updated UI strings with translations (DP-1456, DP-1868, DP-1863)
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) updated to version 3.1-c0.2.1 (containing adblockplus core 0.2.1 equivalent to Web Extension version 3.10)
  - Added Filter Engine enabled/disabled state, for cleaner prevention of subscription updates on startup (DP-1723)
  - Aligning with Web Extension regarding blocking/allowing calls (DP-449)
  - Made `Filter` and `Subscription` mockable via refactoring (DP-1490)
  - Updated punycode.js to 2.1.0 (DP-1649)

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/35983c194d19afdf56847ee011c47df7b0fd7ff0...3b6d3934f1d85a09b9f33d374f56885e033950ca)

Known issues:
* Debug builds may crash due to failed assertions in vanilla Chromium source code; these crashes do not affect release builds (crbug.com/1129004, crbug.com/1041225)

## Chromium 86.0 + ABP 0.16 (ABP Chromium 86 v1)
* Updated to Chromium 86.0.4240.75 (DP-1669)

Known issues:
* Debug builds may crash due to failed assertions in vanilla Chromium source code; these crashes do not affect release builds (crbug.com/1129004, crbug.com/1041225)

## Chromium 85.0 + ABP 0.16 (ABP Chromium 85 v2)
* Introduced new architecture:
  - Removed dependency on libadblockplus-android
  - Removed the need for patches in v8/ and third_party/icu/
  - Removed dependency on Android Support Library
  - More responsive UI, especially on application startup
  - Automatic migration of user settings from previous versions
  - More how-to guides, including Migration FAQ
  - Code better aligned with Chromium conventions
  - Reduced the number of required changes in BUILD.gn files
* Support for user-defined element blocking rules via core API (DP-1412)
* Support for pre-loaded filter lists (DP-1430)
* Filter lists will no longer be downloaded if ad-blocking is disabled (DP-1725)
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/-/compare/92c258681107db81c414083db7f64710642c7fec...35983c194d19afdf56847ee011c47df7b0fd7ff0)

## Chromium 85.0 + ABP 0.15 (ABP Chromium 85 v1)
* Updated to Chromium 85.0.4183.81 (DP-1471)
* Reduced the verbosity of log messages containing debug information

## Chromium 84.0 + ABP 0.15 (ABP Chromium 84 v1)
* Updated to Chromium 84.0.4147.89 (DP-1334)
* Switched to special version of [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) migrated to AndroidX, dependency points to branch `release-3.24-androidx-migrated`

## Chromium 83.0 + ABP 0.15 (ABP Chromium 83 v2)
* Upgraded ABP Core to version 3.9.1 (DP-1250, DP-1371)
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated
* Fixed a bug causing some ads whitelisted by sitekey filters to be blocked on browser restart (DP-1268)
* Reduced the number of ANRs (freezes) significantly due to moving more usages of the filter engine out of the UI thread (DP-1029)
* Simplified code in order to make future Chromium updates easier (DP-871, DP-1266, DP-1189, DP-1323)
* Fixed an ABP Core regression: Extended anchor does not matching a repeating pattern (DP-1208)

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/ffb27eeb2c61e3f951264485e435d03c0be5cd82...9f5579efbc774325c0b71978b775f96bf9fe64b1)

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/9436c424909ef9df9393ac48f0bae45c2c1dfa28...92c258681107db81c414083db7f64710642c7fec)

## Chromium 83.0 + ABP 0.14 (ABP Chromium 83 v1)
* Updated to Chromium 83.0.4103.96 (DP-1193)
* Made ABP Settings UI more complying to other parts of chromium's settings UI which also mitigated chromium switching from the android support library to androidx (DP-1187)

## Chromium 81.0 + ABP 0.14 (ABP Chromium 81 v2)
* Upgraded ABP Core to version 3.6 (DP-1107)
* Made ABP integration ready for Network Service running out of process (DP-1017)
* Made the ABP popup blocker run first and disabled the builtin 'popups blocked' dialog (DP-1149)
* Moved waiting for the filter engine out of the UI thread (DP-1249)
* Fixed a bug causing the cache not being kept when changing subscriptions (DP-1061)
* Fixed a bug leaving ad blocking still enabled when ABP and Acceptable Ads are both off (DP-1090)
* Fixed a bug causing crashes due to the filter engine not being ready when entering Settings (DP-1173)
* Fixed a crash caused by not checking if the filter engine is ready before the sitekey verification (DP-1267)
* Adjusted to the threading changes introduced by libadblockplus-android 3.19 (DP-1281, DP-1263)
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/bf43c5313fbb4c39b91e84bc063cbeb448557461...ffb27eeb2c61e3f951264485e435d03c0be5cd82)

Known regression:
* Extended anchor does not match repeating pattern (DP-1208)

## Chromium 81.0 + ABP 0.13 (ABP Chromium 81 v1)
* Updated to Chromium 81.0.4044.96 (DP-939)
* Fixed the detection if ABP library should not be loaded for other processes than the browser one so it does not raise an exception for the case the process is started by the Zygote preload.

## Chromium 80.0 + ABP 0.13 (ABP Chromium 80 v2)
* Updated to Chromium 80.0.3987.132 (DP-895)
* Fixed incorrect JS escaping, convinience fix for strings manipulation (DP-843)
* New way to collect frame heirarchy, improves filter matching for some cases (DP-923)
* Various fixes aimed at improving stability and avoiding ANRs (DP-965, DP-887, DP-885, DP-831)

## Chromium 80.0 + ABP 0.12 (ABP Chromium 80 v1)
* Updated to Chromium 80.0.3987.99 (DP-751)

## Chromium 79.0 + ABP 0.12 (ABP Chromium 79 v2)
* Improved performance by distinguishing shared vs exclusive lock (rw locks) when setting/using g_adblock_provider (DP-736)

## Chromium 79.0 + ABP 0.11 (ABP Chromium 79 v1)
* Updated to Chromium 79.0.3945.93 (DP-724)

## Chromium 78.0 + ABP 0.11 (ABP Chromium 78 v3)
* Added API providing ad blocked and ad whitelisted notifications (DP-665, DP-553, DP-586)
* Updated to Chromium 78.0.3904.108 (DP-658)
* Fixed a regression bug introduced with ABPChromium 77, where subframes were not hidden after being blocked (DP-617)
* Improved stability of multithreaded code (DP-422)

## Chromium 78.0 + ABP 0.10 (ABP Chromium 78 v1)
* Updated to Chromium 78.0.3904.62 (DP-630)

## Chromium 77.0 + ABP 0.10
* Updated to Chromium 77.0.3865.73 (DP-559)
* Addressed the architectural changes of Chromium 77 by adapting our integration to NetworkService running in-process
* Ported settings to use types from Android support library to be compatible with Chromium (DP-584)
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/4cf02b4b5f2837d45852234d9a1d2448e85f140c...9fff93a94836c9d31323c7d4a748c75f8bbc56c8)

Known issues and notes:
* ABP does not work with NetworkService running out of process
* Subframes are not being hidden after being blocked (DP-601)

## Chromium 76.0 + ABP 0.10
* Updated to Chromium 76.0.3809.132 (DP-494)
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* Improved stability (DP-212, DP-482)
* Improved performance (DP-469)

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/7a2ee1131ada85f9ec76186a1fe8ae5fa82cade3...4cf02b4b5f2837d45852234d9a1d2448e85f140c)

## Chromium 76.0 + ABP 0.9
* Updated to Chromium 76.0.3809.89
* Disabled usage of NetworkService on top of upstream Chromium 76

## Chromium 75.0 + ABP 0.9
* Updated to Chromium 75.0.3770.101
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated
* Added support for Genericblock filter option (DP-164)
* Added support for Generichide filter option (DP-163)
* Fixed a URL parsing bug causing sitekey whitelisting to not work on testpages.adblockplus.org (DP-229)
* Fixed a bug which was blocking AAX sitekey ads when re-enabling ABP or restarting the browser (DP-242)
* Fixed a bug causing anonymous iframe document not being blocked (DP-245)
* Code cleanup: Reverted DP-222 changes - it was made redundant by DP-273 (DP-311)

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/468be7e41f58f035278d825a50f06aa04b4b9b02...9face67824818f9195d884dd4bc16e905937fcf8)

## Chromium 75.0 + ABP 0.8
* Updated to Chromium 75.0.3770.67
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/260136a5dffd457f3b964caa54bfcc7eeece4335...468be7e41f58f035278d825a50f06aa04b4b9b02)

## Chromium 74.0 + ABP 0.8
* Updated to Chromium 74.0.3729.136
* Completely disabled field trial in order to disable lite page notifications (DP-66,DP-273)
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/d5c01bb06dffe21f7dcd634f65f224596d2094b4...260136a5dffd457f3b964caa54bfcc7eeece4335)

## Chromium 73.0 + ABP 0.8
* Updated to Chromium 73.0.3683.90
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* Improved whitelisting (DP-190, DP-145)
* Fixed a bug with websocket support (DP-93)
* Fixed a bug where request blocking was disabled in Production Builds due to a field trial NetworkService feature (DP-222)
* Added support for compilation for arm64 architecture (DP-181)

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/10266f56a6cc8b18deb497adc9f566d15d38ea87...4794faabf5a743085983a7f0bb5dcaed7afe2249)

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/9ee5526e35cc651542082cbf61d70b9ae813457c...7a2ee1131ada85f9ec76186a1fe8ae5fa82cade3)

## Chromium 73.0 + ABP 0.7
* Updated to Chromium 73.0.3683.75
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/925ad81b5c81829b341b06fc98980912b1429b6d...9ee5526e35cc651542082cbf61d70b9ae813457c)

## Chromium 72.0 + ABP 0.7
* Updated a minor version of Chromium in order to include fix of CVE-2019-5786 (DP-90)

## Chromium 72.0 + ABP 0.7
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* Fixed a bug where domain whitelisting is not working (DP-7)
* Added support for element hiding emulation (DP-17)
* Added support for sitekey (DP-28)

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/ec692c663b60e76c4a02a30323fac73686a18c14...10266f56a6cc8b18deb497adc9f566d15d38ea87)

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/f2e1a80624bb5280121eb07a726e5417af227ba7...925ad81b5c81829b341b06fc98980912b1429b6d)

## Chromium 72.0 + ABP 0.6
* Updated to Chromium 72.0.3626.76

## Chromium 71.0 + ABP 0.6
* [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus) was updated
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/71) where chromium would crash in incognito mode
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/29) in blocking of popups
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/55) that caused chromium to crash on android 4.4.4

[Full list of `libadblockplus` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus/compare/9fb96255de4d62e8a29f6d7e889d54d1fad9feb4...d0a4727ac3c21c8551eb392d2571122d1f88dead)

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/d150f08d5d72de8938c7ebbdccd9b0c4e06b4070...c803f858ca2c7ab572acf333042e254f41de3b94)

## Chromium 71.0 + ABP 0.5
* Updated to Chromium 71.0.3578.83

## Chromium 70.0 + ABP 0.5
* [libadblockplus-android](https://gitlab.com/eyeo/adblockplus/libadblockplus-android) was updated
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/33) with websocket requests not being blocked
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/35) with !important css styles not being blocked
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/36) where element hiding was not applied to content added via document.write() to anonymous iFrames
* [Added support](https://gitlab.com/eyeo/adblockplus/chromium/issues/39) for allowing custom filter list subscriptions

[Full list of `libadblockplus-android` changes](https://gitlab.com/eyeo/adblockplus/libadblockplus-android/compare/da7f888d7ef0c4a9fb798a972e5132612730b740...d150f08d5d72de8938c7ebbdccd9b0c4e06b4070)

[Full list of `adblockpluschromium` changes](https://gitlab.com/eyeo/adblockplus/chromium/compare/cd317a965431966844f8d25f4e13dd352a6e1340...dev-70.0.3538.64_2)

## Chromium 70.0 + ABP 0.4
* Updated to Chromium 70.0.3538.64

## Chromium 69.0 + ABP 0.4
* Updated to Chromium 69.0.3497.100

## Chromium 69.0 + ABP 0.4
* Updated to Chromium 69.0.3497.91

## Chromium 68.0 + ABP 0.4
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/31) where type of some network requests was incorrectly labeled as FAVICON instead of IMAGE, causing some images to not be blocked.
* [Fixed a bug](https://gitlab.com/eyeo/adblockplus/chromium/issues/29) where pop-up blocking was not working.
* [Improved ad blocking](https://gitlab.com/eyeo/adblockplus/chromium/issues/23) experience by hiding empty spaces left from elements which were blocked on a network level.

## Chromium 68.0 + ABP 0.3.4
* Updated to Chromium `68.0.3440.70`
* [Use Chromium Android NDK](https://gitlab.com/eyeo/adblockplus/chromium/issues/26) when building, instead of downloading official Google Android NDK
* [Fixed a bug](https://issues.adblockplus.org/ticket/6799) where Hebrew subscription was not installed for Hebrew locale

## Chromium 67.0 + ABP 0.3.3
* Updated to Chromium `67.0.3396.87`
* [Update](https://gitlab.com/eyeo/adblockplus/chromium/issues/7) the way `libadblockplus` and `libadblockplus-android` are built
* [Added support](https://issues.adblockplus.org/ticket/6632) for:
   - `am-ET - Amharic (Ethiopia)`
   - `fr-CA - French (Canada)`
   - `fil-PH - Filipino (Philippines)`
   - `sw-KE - Kiswahili (Kenya)`
* [Initialize](https://gitlab.com/eyeo/adblockplus/chromium/issues/9) filter engine asynchronously, and [wait](https://gitlab.com/eyeo/adblockplus/chromium/issues/14) for it in Settings.
* Stop using [deprecated libadblockplus-android API](https://gitlab.com/eyeo/adblockplus/chromium/issues/13)
* [Disable](https://gitlab.com/eyeo/adblockplus/chromium/issues/20) Chromium's built-in ad blocking
* [Support](https://gitlab.com/eyeo/adblockplus/chromium/issues/22) configuring of android package name filter list network requests
* [Correctly detect](https://gitlab.com/eyeo/adblockplus/chromium/issues/24) the type of image resources
* Added a requirement to have 7zip installed on building machine

[Full list of `libadblockplus` changes](https://github.com/adblockplus/libadblockplus/compare/ea5309a0a6f3c5ab1e378b79c09d930ac3fbcfd0...40f5d4d2d00abe3f94ce69210267bcce908cd748).

[Full list of `libadblockplus-android` changes](https://github.com/adblockplus/libadblockplus-android/compare/7ef63f2b8458a5b23595bd22c180c0e6f2398801...5342ea7697a7a55a3f649aadb6a976a0799e7922)

[Full list of `adblockpluschromium` changes](https://github.com/adblockplus/chromium/compare/483c3b509b0f032a7e92bd4fce339e70c452d2aa...6b18f01e06da9bcb1f5ffdd3c9cfb1e1e485cbf2)

## Chromium 65.0 + ABP 0.3.2
* Base release of ad blocking integration

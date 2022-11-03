# Design Overview

ABP Chromium is a modification of Chromium that employs publicly available rules to filter ads on websites. This document gives an overview of the project.

## Composition

The implementation includes several services (`KeyedService`):

* `AdblockRequestClassifier` is responsible for deciding which web requests should be blocked.
* `AdblockElementHider` is responsible for generating special CSS styles and JavaScript code that Chromium injects into web pages to hide unwanted content.
* `AdblockController` allows controlling ad filtering settings
* `AdblockPlatformEmbedder` maintains `IFilterEngine`, an entry point into `libadblockplus` used by `AdblockRequestClassifier` and `AdblockElementHider`
* `AdblockTelemetryService` is responsible for reporting anonymous usage statistics.

You will find most of ABP Chromium-specific code in the following places:

* `components/adblock` - platform-agnostic ad filtering integration
* `chrome/browser/adblock` - OS-agnostic but Chrome-specific integration
* `chrome/browser/android/adblock` and `chrome/android` - Android-specific JNI code for UI
* `chrome/renderer/adblock` - hooks in the Renderer process to observe Renderer-issued resource loads
* `chrome/common/extensions` - implementation of the `adblockPrivate` Extension API
* `chrome/android/java/src/org/chromium/chrome/browser/adblock` - Android implementation of Settings UI
* `third_party/libadblockplus` - portable C++ ad filtering core

## Request Filtering

AdblockPlus integration plugins into Chromium's network stack by injecting an implementation of `blink::URLLoaderThrottle` (see: `chrome/common/adblock/adblock_url_loader_throttle.h`).
This implementation, `AdblockURLLoaderThrottle`, is notified in two situations:
* At the very beginning of network request initialization (`AdblockURLLoaderThrottle::WillStartRequest`). We may pause the request until we decide whether to block or allow it.
* When receiving headers of HTTP requests (`AdblockURLLoaderThrottle::WillProcessResponse`) in order to extract a SiteKey. SiteKeys can influence the filtering decision.

Instances of `AdblockURLLoaderThrottle` may live in the renderer processes or in the browser process.
`mojom::AdblockInterface` forwards the following pieces of data to the browser process:
* Request URL
* Request type (content/script/image, etc.)
* User-Agent header of the request
* `X-Adblock-Key` response header
* ID of the render frame that requested the resource

In scope of the browser process, `AdblockRequestClassifier` receives the data, makes a blocking decision (`AdblockRequestClassifier::CheckFilterMatch`) and sends the decision back to the network service via Mojo.

## Element Hiding

Intercepting network requests is not enough to filter some content.

On some web pages, we must hide content after it has loaded. Hiding elements is done by injecting CSS or JavaScript code into web pages.

This is implemented as follows:

1. We register WebContentsObserver to get a notification that the page is loaded.
2. CSS and JavaScript are generated for injection (`AdblockElementHider`).
3. This data is sent to the renderer process and added as part of the page content via a special method: `RenderFrameHost::InsertUserStylesheet`.

## Filtering Rules

The content filtering process depends on a large number of rules. When all the data is correctly initialized, it takes libadblockplus very little time to decide which content should be blocked. However, there are a few peculiarities.

First, while `IFilterEngine` is initializing, the network requests processing is suspended. Loading rules from disk takes time, and we postpone filtering decisions until the data is ready. According to our measurements, the initialization time is acceptable and does not affect the user experience. We plan to speed up loading even more in the future.

Secondly, by default, the implementation downloads rules from the network on the first run and updates them periodically. At first startup, in absence of filter rules, the implementation will allow any content. To avoid this situation, it is possible to bundle some filter rules within the APK.

The rules and most of the configuration are saved in the user data folder, file name is `patterns.ini`. To make the configuration available between Chromium processes, for example to know if the filtering is enabled in the network service, some of the options are available in the Chromium preferences.

## Java API

The Java interface was designed primarily to implement user preferences. The `org.chromium.chrome.browser.adblock.AdblockController` class lets you configure subscriptions to different rule lists and enable/disable content filtering, among some other features.

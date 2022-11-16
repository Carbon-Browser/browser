# Design overview

This section describes the general design of eyeo Chromium SDK, and how it fits into Chromium's design (classes, services, processes, etc).

## Services

The SDK is comprised of the following `KeyedService`s:

* `AdblockController`: Allows to control ad-filtering settings.
* `SubscriptionService`: Maintains a state of available `Subscription`s and synchronizes it with persistent storage.
* `ResourceClassificationRunner`: Decides whether to block or allow network requests.
* `ElementHider`: Applies element hiding scripts and stylesheets on web pages.
* `AdblockTelemetryService`: Reports anonymous usage statistics to eyeo.
* `SitekeyStorage`: Extracts SiteKeys from response headers, validates and stores them.


## Highlighted Chromium classes

The Chromium classes that are particularly important for our implementation are:

* `RenderFrameHost`: To find the frame hierarchy, and to execute CSS and JavaScript.
* `WebContentsObserver`: To receive page load events, as well as injecting element hiding. Parent of `AdblockWebContentsObserver`.
* `ChromeContentBrowserClient`: For setup of URL loader factories, MOJO bindings, etc. Parent of `AdblockChromeContentBrowserClient`.


## Mapping to Chromium processes

eyeo Chromium SDK code is executed either in the browser process or in the renderer process. Given that the vast majority falls into the former category, this section only describes the exceptions.

* `AdblockURLLoaderFactory`: Used in browser process.
* `AdblockMojoInterfaceImpl`: Allows to communicate between the browser and the renderer processes but here used to communicate just within a browser process. May be removed in the future as not needed.

# Integration how-to guides

This section provides guidance, including code snippets, on integrating ABP Chromium with your browser.


## 1. Where can I find code related to ad-blocking?

All Java classes related to ad-blocking use an [AdblockController](
chrome/android/java/src/org/chromium/chrome/browser/adblock/AdblockController.java).
This class is a singleton.

Classes located under
`chrome/android/java/src/org/chromium/chrome/browser/settings`
allow to configure functionality like adding/removing filter lists,
enabling/disabling ad-blocking or enabling/disabling Acceptable Ads.


## 2. How to change the application identifier for the filter lists download?

We recommend overriding the values returned from [version_info::GetProductName()](components/version_info/version_info.cc)
and [version_info::GetVersionNumber()](components/version_info/version_info.cc) by
setting the values of `PRODUCT_NAME` and `PRODUCT_VERSION` constants:
- `PRODUCT_NAME` is derived from [chrome/app/theme/chromium/BRANDING](chrome/app/theme/chromium/BRANDING) file.
- `PRODUCT_VERSION` is derived from [chrome/VERSION](chrome/VERSION) file.

You can either modify those files or have the functions return different strings.

If this is not convenient (changes of the above affect the whole application) this still
can be overridden with gn `abp_application_name` and `abp_application_version` variables i.e.
`gn gen --args='abp_application_name="My great browser" abp_application_version="99.99.0.1"...' ...`

## 3. How to add and remove subscription(s) to my filter lists?

Call [AdblockController's](
chrome/android/java/src/org/chromium/chrome/browser/adblock/AdblockController.java) `addCustomSubscription()` method.

The following example makes ABP Chromium install (download and parse) the filter list:

```java
import org.chromium.chrome.browser.adblock.AdblockController;

URL myFilterList = new URL("http://example.com/my_list.txt");
AdblockController.getInstance().addCustomSubscription(myFilterList);
```

To remove a custom filter list:

```java
import org.chromium.chrome.browser.adblock.AdblockController;

URL myFilterList = new URL("http://example.com/my_list.txt");
AdblockController.getInstance().removeCustomSubscription(myFilterList);
```


## 4. How to disable ad-blocking on a specific domain?

Call [AdblockController's](
chrome/android/java/src/org/chromium/chrome/browser/adblock/AdblockController.java) `addAllowedDomain()` method.

The following example makes ABP Chromium cease blocking ads on the specified domain:

```java
import org.chromium.chrome.browser.adblock.AdblockController;

String domain = "example.com";
AdblockController.getInstance().addAllowedDomain(domain);
```

To re-enable blocking ads on the specified domain:

```java
import org.chromium.chrome.browser.adblock.AdblockController;

String domain = "example.com";
AdblockController.getInstance().removeAllowedDomain(domain);
```

Note: Pass a *domain* (`example.com`) as an argument, not a URL (`http://www.example.com/page.html`).


## 5. How to migrate settings from the older versions?

Older releases (up to ABP Chromium 85v1) stored settings in Android's [Shared
Preferences](https://developer.android.com/training/data-storage/shared-preferences).
Newer releases rely on Chromium preferences.
In order to preserve user settings when upgrading from a release prior to 85v2, use the [LegacyPrefsMigration](
chrome/android/java/src/org/chromium/chrome/browser/adblock/legacy/LegacyPrefsMigration.java)
class.

Include the following at application startup (e.g. in
`ChromeBrowserInitializer.onFinishNativeInitialization()`) to automatically
handle the settings migration:

```java
import org.chromium.chrome.browser.adblock.legacy.LegacyPrefsMigration;

LegacyPrefsMigration.migrateLegacyPreferences();
```


## 6. How to receive notifications about blocked or allowed network requests

Implement the `AdblockController.AdBlockedObserver` interface.

Add your observer by calling
`AdblockController.getInstance().addOnAdBlockedObserver()` and remove it by calling
`AdblockController.getInstance().removeOnAdBlockedObserver()`. Do not add an
`AdBlockedObserver` if you are not going to consume these notifications, as it
has a small performance penalty.


## 7. Optional preloading of bundled subscriptions

Preloading bundled subscriptions allows providing an existing filter list instead of
downloading it from a server. This is especially useful when the network is unreliable or slow.

We provide the minified EasyList and exception rules which reside at:

components/resources/adblocking/easylist.txt
components/resources/adblocking/exceptionrules.txt

In order to bundle these subscriptions, specify `enable_bundled_subscriptions=true` argument
for `gn gen` command. E.g.: `gn gen --args='enable_bundled_subscriptions=true ...' ...`

In order to add more preloaded subscriptions, please modify the code in
`ReadResourceInBackground` at components/adblock/adblock_resource_reader_impl.cc
and add the records for the corresponding files in `components/resources/adblock_resources.grdp`
Consider removing `!Title` attribute in the bundled file in order to not overwrite the
subscription title.


## 8. How to test filters?

Eyeo provides testing pages for ABP Chromium's ad-blocking features on
https://testpages.adblockplus.org. You need to subscribe to the accompanying [test filter list](
https://testpages.adblockplus.org/en/abp-testcase-subscription.txt) before loading them.
To learn how to subscribe to a specific filter list, consult point 3 of this FAQ.

## 9. How to enable advanced ad-blocking using snippet filters?

Advanced Ad-blocking is achieved through the usage of snippet filters.
More information about this filter type can be found at https://help.eyeo.com/adblockplus/snippet-filters-tutorial.
To make use of eyeo's snippet filters, please add a default subscription to the "ABP Filters" filter list.

## 10. What does the "Update only on WiFi" setting in ABP Chromium's UI mean?

This setting specifies under which circumstances ABP Chromium may issue network
requests, like filter list downloads.

When WiFi is the only allowed connection type (the default setting) ABP Chromium won't be
issuing any network requests when the user is on a different connection type,
reducing the usage of their data allowance. But as a consequence, filter lists
won't be updated or even downloaded for the first time, which means this
setting has a huge impact on ad-blocking functionality.
You can provide preloaded subscriptions to ensure a filter list is available in this scenario. See
point 7 of this FAQ.

This setting corresponds to the `AdblockController.setAllowedConnectionType()`
method.

## 11. How to know what was changed between ABP Chromium major releases

You can use `tools/adblock/interdiff.py` to compare two git revision ranges. This is useful in case
you want to cherry-pick ABP changes to you repository. To get combined patch, execute interdiff for
ABP changes in old branch and ABP changes in new branch. You can get revison ranges from release
announcements. To get help how to use the script run `./interdiff.py --help`.

The script requires patchutils to be available. On Mac it is available via `brew install patchutils`,
on Linux you can use your package manager to get it like `sudo apt install patchutils`.

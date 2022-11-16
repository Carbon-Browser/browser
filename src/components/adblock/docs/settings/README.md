# Settings

eyeo Chromium SDK can be configured to provide the experience most adequate for the end-user.

At the moment, the following settings are supported:

| Setting                 | Type    | Description |
| ----------------------- | ------- | ----------- |
| Ad-blocking             | Boolean | Whether ad-blocking is enabled or disabled. |
| Acceptable Ads          | Boolean | Whether Acceptable Ads are displayed or hidden. |
| Subscriptions           | List    | Subscriptions to filter lists. |
| Allowed domains         | List    | Domains on which no ads will be blocked, even when ad-blocking is enabled. |
| Custom filters          | List    | Additional filters implemented via [filter language](https://help.eyeo.com/adblockplus/how-to-write-filters) |
| Allowed connection type | String  | Connection type under which filter lists can be downloaded. |


## "Allowed connection type" setting ("Update on WiFi only" in the UI)

The "allowed connection type" setting specifies under which circumstances eyeo Chromium SDK may issue network requests, like filter list downloads. The possible values are:

* *any*: On any network.
* *wifi*: Only on WiFi.

By default, downloads are allowed on any connection type to ensure that the most up-to-date filter lists are always available.

When only WiFi is allowed, eyeo Chromium SDK won't issue network requests when the user is on a different connection type, reducing the usage of their data allowance. But as a consequence, filter lists won't be updated or even downloaded, which means this setting has a huge impact on ad-blocking functionality. You can bundle filter lists to ensure that they are available in this scenario. See the [ad-filtering documentation](../ad-filtering/filter-lists.md) for more information about bundled filter lists.


## Implementation details

Settings can be configured via the C++ class `AdblockController` or its Java counterpart, `org.chromium.components.adblock.AdblockController`.

The Browser Extension API defined and documented in `chrome/common/extensions/api/adblock_private.idl` uses the C++ implementation. Android UI fragments consume the Java one.

The [user setting sequence diagram](user-setting-sequence.png) describes the flow of updating a setting, and how it varies depending on the API being used.

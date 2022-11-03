# Adblock Plus Chromium integration

Adblock Plus Chromium (ABP Chromium) is a fork of the [Chromium project](https://chromium.googlesource.com/chromium/src)
that integrates ad-blocking capabilities.

This fork serves as a Software Development Kit (SDK) for extending Android browsers based
on Chromium. We have forked and modified Chromium rather than bundling an
extension because extensions cannot be installed on mobile browsers.

Please send any questions or report any issues to distribution-partners@eyeo.com.


## Dependencies and prerequisites

ABP Chromium depends on several parts of Chromium:
  - [KeyedService](components/keyed_service/core/keyed_service.h)
  - [Network Service](services/network/)
  - [PrefService](components/prefs/pref_service.h)
  - [Profile](chrome/browser/profiles/profile.h)
  - [Resources](components/resources/)
  - [Version Info](components/version_info/)

If you cannot include those or any other parts of Chromium in your browser, you
will have to re-implement them or work around them.

ABP Chromium depends on [libadblockplus](https://gitlab.com/eyeo/adblockplus/libadblockplus/)
which is regularly updated along with ABP Chromium releases. It is already
present in the ABP Chromium source tree in `third_party/libadblockplus`.


## Integration how-to guides

For more information and snippets, please see our [how-to guides](integration-how-to.md).

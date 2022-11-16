## SF-Browser-Chromium

Fetch release branch with no history to save time ( add --depth 1 after clone)

    git clone --branch [VERSION TAG] https://chromium.googlesource.com/chromium/src.git

Sync with branch heads and tags

    gclient sync --with_branch_heads --with_tags

Rebrand strings by running lib/replace_strings.py script
Search and replace "Google Super Fast Browser"/Google <ph name="APP_NAME">%1$s<ex> text, found in managing storage.

Update string ids by running src/tools/grit/grit/extern/rebrand.py

Change icon in src/chrome/android/java/res_chromium

Replace toolbar icons and location bar background -> copy paste lib/res/chrome in chrome/android/java/res and merge/replace
Same for lib/res/content in content/public/android/java/res
and lib/res/res_chromium in chrome/android/java/res_chromium

Change package in src/chrome/android/BUILD.gn

Change version code (android_default_version_code) and name (android_default_version_name) and keystore path in build/config/android/config.gni

#### Common useful stuff

Add this flag to args.gn when adding a new dependency (run "gn gen out/Default" and remove the flag)

    update_android_aar_prebuilts = true

Clone ABPChromium

    git clone -b [Branch ex: eyeo-95-rc] https://git.distpartners.eyeo.com/abpchromium.git/

How to refresh resources (doing gn gen out/whatever doesn't work anymore) 'chrome_java_resources.gni'   -   src/chrome/android/

    sed -i '/^[^#][^[]*$/d' chrome_java_resources.gni

    (for f in $(find java/res/*/ -type f); do echo '  "'$f'",'; done; echo ']') >> chrome_java_resources.gni


[Chromium versions](https://www.chromium.org/developers/calendar)


Android strings file: chrome/browser/ui/android/strings/android_chrome_strings.grd



args.gn
##### START
#update_android_aar_prebuilts = true
treat_warnings_as_errors=false
target_os="android"
target_cpu="arm64"
is_debug=false
is_java_debug=false

android_channel="stable"
is_official_build=true
is_component_build=false
is_chrome_branded=false
is_clang=true
symbol_level=1
# make this 0 for release

enable_iterator_debugging=false

proprietary_codecs=true
ffmpeg_branding="Chrome"
enable_nacl=false

use_official_google_api_keys=false

ffmpeg_branding = "Chrome"
proprietary_codecs = true
enable_nacl = false
#enable_widevine = false
enable_mse_mpeg2ts_stream_parser = true
enable_remoting = false

v8_use_external_startup_data = true

use_thin_lto = true
##### END

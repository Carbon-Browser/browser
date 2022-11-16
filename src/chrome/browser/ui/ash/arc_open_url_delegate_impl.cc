// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/arc_open_url_delegate_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/components/arc/mojom/intent_helper.mojom.h"
#include "ash/constants/ash_features.h"
#include "ash/public/cpp/new_window_delegate.h"
#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/files/safe_base_name.h"
#include "base/notreached.h"
#include "chrome/browser/apps/app_service/app_service_proxy.h"
#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/apps/app_service/intent_util.h"
#include "chrome/browser/apps/app_service/launch_utils.h"
#include "chrome/browser/ash/apps/apk_web_app_service.h"
#include "chrome/browser/ash/arc/arc_util.h"
#include "chrome/browser/ash/arc/fileapi/arc_content_file_system_url_util.h"
#include "chrome/browser/ash/arc/intent_helper/custom_tab_session_impl.h"
#include "chrome/browser/ash/file_manager/fileapi_util.h"
#include "chrome/browser/ash/file_manager/path_util.h"
#include "chrome/browser/ash/fusebox/fusebox_server.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/browser/chromeos/fileapi/external_file_url_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/shelf/app_service/app_service_app_window_arc_tracker.h"
#include "chrome/browser/ui/ash/shelf/app_service/app_service_app_window_shelf_controller.h"
#include "chrome/browser/ui/ash/shelf/chrome_shelf_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "chrome/browser/ui/webui/settings/chromeos/constants/routes.mojom.h"
#include "chrome/browser/web_applications/web_app_helpers.h"
#include "chrome/browser/web_applications/web_app_id.h"
#include "chrome/browser/web_applications/web_app_utils.h"
#include "chrome/browser/webshare/prepare_directory_task.h"
#include "chrome/common/webui_url_constants.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "components/arc/intent_helper/custom_tab.h"
#include "components/services/app_service/public/cpp/app_update.h"
#include "components/services/app_service/public/cpp/intent_filter_util.h"
#include "components/services/app_service/public/cpp/intent_util.h"
#include "components/services/app_service/public/cpp/types_util.h"
#include "components/services/app_service/public/mojom/types.mojom.h"
#include "components/user_manager/user_manager.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/base/filename_util.h"
#include "net/base/url_util.h"
#include "storage/browser/file_system/file_system_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using arc::mojom::ChromePage;

namespace {

ArcOpenUrlDelegateImpl* g_instance = nullptr;

constexpr auto kOSSettingsMap = base::MakeFixedFlatMap<ChromePage,
                                                       const char*>({
    {ChromePage::ACCOUNTS,
     chromeos::settings::mojom::kManageOtherPeopleSubpagePathV2},
    {ChromePage::AUDIO, chromeos::settings::mojom::kAudioSubpagePath},
    {ChromePage::BLUETOOTH,
     chromeos::settings::mojom::kBluetoothDevicesSubpagePath},
    {ChromePage::BLUETOOTHDEVICES,
     chromeos::settings::mojom::kBluetoothDevicesSubpagePath},
    {ChromePage::CHANGEPICTURE,
     chromeos::settings::mojom::kChangePictureSubpagePath},
    {ChromePage::CUPSPRINTERS,
     chromeos::settings::mojom::kPrintingDetailsSubpagePath},
    {ChromePage::DATETIME, chromeos::settings::mojom::kDateAndTimeSectionPath},
    {ChromePage::DISPLAY, chromeos::settings::mojom::kDisplaySubpagePath},
    {ChromePage::HELP, chromeos::settings::mojom::kAboutChromeOsSectionPath},
    {ChromePage::KEYBOARDOVERLAY,
     chromeos::settings::mojom::kKeyboardSubpagePath},
    {ChromePage::LOCKSCREEN,
     chromeos::settings::mojom::kSecurityAndSignInSubpagePathV2},
    {ChromePage::MAIN, ""},
    {ChromePage::MANAGEACCESSIBILITY,
     chromeos::settings::mojom::kManageAccessibilitySubpagePath},
    {ChromePage::MANAGEACCESSIBILITYTTS,
     chromeos::settings::mojom::kTextToSpeechSubpagePath},
    {ChromePage::MULTIDEVICE,
     chromeos::settings::mojom::kMultiDeviceSectionPath},
    {ChromePage::NETWORKSTYPEVPN,
     chromeos::settings::mojom::kVpnDetailsSubpagePath},
    {ChromePage::OSLANGUAGESINPUT,
     chromeos::settings::mojom::kInputSubpagePath},
    {ChromePage::OSLANGUAGESLANGUAGES,
     chromeos::settings::mojom::kLanguagesSubpagePath},
    {ChromePage::POINTEROVERLAY,
     chromeos::settings::mojom::kPointersSubpagePath},
    {ChromePage::POWER, chromeos::settings::mojom::kPowerSubpagePath},
    {ChromePage::PRIVACYHUB, chromeos::settings::mojom::kPrivacyHubSubpagePath},
    {ChromePage::SMARTPRIVACY,
     chromeos::settings::mojom::kSmartPrivacySubpagePath},
    {ChromePage::STORAGE, chromeos::settings::mojom::kStorageSubpagePath},
    {ChromePage::WIFI, chromeos::settings::mojom::kWifiNetworksSubpagePath},
});

constexpr auto kBrowserSettingsMap =
    base::MakeFixedFlatMap<ChromePage, const char*>({
        {ChromePage::APPEARANCE, chrome::kAppearanceSubPage},
        {ChromePage::AUTOFILL, chrome::kAutofillSubPage},
        {ChromePage::CLEARBROWSERDATA, chrome::kClearBrowserDataSubPage},
        {ChromePage::DOWNLOADS, chrome::kDownloadsSubPage},
        {ChromePage::LANGUAGES, chrome::kLanguagesSubPage},
        {ChromePage::ONSTARTUP, chrome::kOnStartupSubPage},
        {ChromePage::PASSWORDS, chrome::kPasswordManagerSubPage},
        {ChromePage::PRIVACY, chrome::kPrivacySubPage},
        {ChromePage::RESET, chrome::kResetSubPage},
        {ChromePage::SEARCH, chrome::kSearchSubPage},
        {ChromePage::SYNCSETUP, chrome::kSyncSetupSubPage},
    });

constexpr auto kAboutPagesMap =
    base::MakeFixedFlatMap<ChromePage, const char*>({
        {ChromePage::ABOUTBLANK, url::kAboutBlankURL},
        {ChromePage::ABOUTDOWNLOADS, "chrome://downloads/"},
        {ChromePage::ABOUTHISTORY, "chrome://history/"},
    });

// Converts the given ARC URL to an external file URL to read it via ARC content
// file system when necessary. Otherwise, returns the given URL unchanged.
GURL ConvertArcUrlToExternalFileUrlIfNeeded(const GURL& url) {
  if (url.SchemeIs(url::kFileScheme) || url.SchemeIs(url::kContentScheme)) {
    // Chrome cannot open this URL. Read the contents via ARC content file
    // system with an external file URL.
    return arc::ArcUrlToExternalFileUrl(url);
  }
  return url;
}

// Converts an externalfile:// ARC URL to a file:// URL managed by the FuseBox
// Moniker system. This Moniker file is readable on the Linux filesystem like
// any other file. Returns the input URL if a Moniker could not be created.
GURL ConvertToMonikerFileUrl(Profile* profile, GURL external_file_url) {
  const base::FilePath virtual_path =
      chromeos::ExternalFileURLToVirtualPath(external_file_url);

  const storage::FileSystemURL fs_url =
      file_manager::util::GetFileManagerFileSystemContext(profile)
          ->CreateCrackedFileSystemURL(
              blink::StorageKey(file_manager::util::GetFilesAppOrigin()),
              storage::kFileSystemTypeExternal, virtual_path);
  if (!fs_url.is_valid()) {
    return external_file_url;
  }

  fusebox::Server* fusebox_server = fusebox::Server::GetInstance();
  if (!fusebox_server) {
    return external_file_url;
  }

  fusebox::Moniker moniker = fusebox_server->CreateMoniker(fs_url);

  // Keep the Moniker alive for the same time as a file shared through the Web
  // Share API. We could be cleverer about scheduling the clean up, but "destroy
  // after a fixed amount of time" is simple and works well enough in
  // practice.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](fusebox::Moniker moniker) {
            fusebox::Server* fusebox_server = fusebox::Server::GetInstance();
            if (fusebox_server) {
              fusebox_server->DestroyMoniker(moniker);
            }
          },
          moniker),
      webshare::PrepareDirectoryTask::kSharedFileLifetime);

  return net::FilePathToFileURL(
      base::FilePath(fusebox::MonikerMap::GetFilename(moniker)));
}

apps::mojom::IntentPtr ConvertLaunchIntent(
    Profile* profile,
    const arc::mojom::LaunchIntentPtr& launch_intent) {
  apps::mojom::IntentPtr intent = apps::mojom::Intent::New();

  const char* action =
      apps_util::ConvertArcToAppServiceIntentAction(launch_intent->action);
  if (action)
    intent->action = action;

  intent->url = launch_intent->data;
  intent->mime_type = launch_intent->type;
  intent->share_title = launch_intent->extra_subject;
  intent->share_text = launch_intent->extra_text;

  if (launch_intent->files.has_value() && launch_intent->files->size() > 0) {
    std::vector<std::string> mime_types;
    intent->files = std::vector<apps::mojom::IntentFilePtr>();
    for (const auto& file_info : *launch_intent->files) {
      auto file = apps::mojom::IntentFile::New();

      file->url = arc::ArcUrlToExternalFileUrl(file_info->content_uri);
      if (ash::features::IsArcFuseBoxFileSharingEnabled()) {
        file->url = ConvertToMonikerFileUrl(profile, file->url);
      }

      file->mime_type = file_info->type;
      file->file_name = file_info->name;
      file->file_size = file_info->size;
      intent->files->push_back(std::move(file));
      mime_types.push_back(file_info->type);
    }

    // Override the given MIME type based on the files that we're sharing.
    intent->mime_type = apps_util::CalculateCommonMimeType(mime_types);
  }

  return intent;
}

// Finds the best matching web app that can handle the |url|.
absl::optional<std::string> FindWebAppForURL(Profile* profile,
                                             const GURL& url) {
  apps::AppServiceProxy* proxy =
      apps::AppServiceProxyFactory::GetForProfile(profile);
  if (!proxy) {
    return absl::nullopt;
  }

  std::vector<std::string> app_ids = proxy->GetAppIdsForUrl(
      url, /*exclude_browsers=*/true, /*exclude_browser_tab_apps=*/true);

  std::string best_match;
  size_t best_match_length = 0;
  for (const std::string& app_id : app_ids) {
    // Among all the matched apps, select a web app with the longest matching
    // scope.
    size_t match_length = 0;
    proxy->AppRegistryCache().ForOneApp(
        app_id, [&url, &match_length](const apps::AppUpdate& update) {
          if (update.AppType() != apps::AppType::kWeb) {
            return;
          }
          for (const auto& filter : update.IntentFilters()) {
            match_length =
                std::max(match_length,
                         apps_util::IntentFilterUrlMatchLength(filter, url));
          }
        });
    if (match_length > best_match_length) {
      best_match_length = match_length;
      best_match = app_id;
    }
  }
  if (best_match.empty()) {
    return absl::nullopt;
  }
  return best_match;
}

}  // namespace

ArcOpenUrlDelegateImpl::ArcOpenUrlDelegateImpl() {
  arc::ArcIntentHelperBridge::SetOpenUrlDelegate(this);
  DCHECK(!g_instance);
  g_instance = this;
}

ArcOpenUrlDelegateImpl::~ArcOpenUrlDelegateImpl() {
  DCHECK_EQ(g_instance, this);
  g_instance = nullptr;
  arc::ArcIntentHelperBridge::SetOpenUrlDelegate(nullptr);
}

ArcOpenUrlDelegateImpl* ArcOpenUrlDelegateImpl::GetForTesting() {
  return g_instance;
}

void ArcOpenUrlDelegateImpl::OpenUrlFromArc(const GURL& url) {
  if (!url.is_valid())
    return;

  GURL url_to_open = ConvertArcUrlToExternalFileUrlIfNeeded(url);
  ash::NewWindowDelegate::GetPrimary()->OpenUrl(
      url_to_open, ash::NewWindowDelegate::OpenUrlFrom::kArc);
}

void ArcOpenUrlDelegateImpl::OpenWebAppFromArc(const GURL& url) {
  DCHECK(url.is_valid() && url.SchemeIs(url::kHttpsScheme));

  // Fetch the profile associated with ARC. This method should only be called
  // for a |url| which was installed via ARC, and so we want the web app that is
  // opened through here to be installed in the profile associated with ARC.
  // |user| may be null if sign-in hasn't happened yet
  const auto* user = user_manager::UserManager::Get()->GetPrimaryUser();
  if (!user)
    return;

  // |profile| may be null if sign-in has happened but the profile isn't loaded
  // yet.
  Profile* profile = ash::ProfileHelper::Get()->GetProfileByUser(user);
  if (!profile)
    return;

  absl::optional<web_app::AppId> app_id =
      web_app::IsWebAppsCrosapiEnabled()
          ? FindWebAppForURL(profile, url)
          : web_app::FindInstalledAppWithUrlInScope(profile, url,
                                                    /*window_only=*/true);

  if (!app_id) {
    OpenUrlFromArc(url);
    return;
  }

  int event_flags = apps::GetEventFlags(WindowOpenDisposition::NEW_WINDOW,
                                        /*prefer_container=*/false);
  apps::AppServiceProxy* proxy =
      apps::AppServiceProxyFactory::GetForProfile(profile);

  proxy->AppRegistryCache().ForOneApp(
      *app_id, [&event_flags](const apps::AppUpdate& update) {
        if (update.WindowMode() == apps::WindowMode::kBrowser) {
          event_flags =
              apps::GetEventFlags(WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  /*prefer_container=*/false);
        }
      });

  proxy->LaunchAppWithUrl(*app_id, event_flags, url,
                          apps::mojom::LaunchSource::kFromArc);

  ash::ApkWebAppService* apk_web_app_service =
      ash::ApkWebAppService::Get(profile);
  if (!apk_web_app_service ||
      !apk_web_app_service->IsWebAppInstalledFromArc(app_id.value())) {
    return;
  }

  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(profile);
  if (!prefs)
    return;

  absl::optional<std::string> package_name =
      apk_web_app_service->GetPackageNameForWebApp(app_id.value());
  if (!package_name.has_value())
    return;

  ChromeShelfController* chrome_shelf_controller =
      ChromeShelfController::instance();
  if (!chrome_shelf_controller)
    return;

  auto* arc_tracker =
      chrome_shelf_controller->app_service_app_window_controller()
          ->app_service_arc_tracker();
  if (!arc_tracker)
    return;

  for (const auto& app_id : prefs->GetAppsForPackage(package_name.value()))
    arc_tracker->CloseWindows(app_id);
}

void ArcOpenUrlDelegateImpl::OpenArcCustomTab(
    const GURL& url,
    int32_t task_id,
    arc::mojom::IntentHelperHost::OnOpenCustomTabCallback callback) {
  GURL url_to_open = ConvertArcUrlToExternalFileUrlIfNeeded(url);
  Profile* profile = ProfileManager::GetActiveUserProfile();

  aura::Window* arc_window = arc::GetArcWindow(task_id);
  if (!arc_window) {
    std::move(callback).Run(mojo::NullRemote());
    return;
  }

  auto custom_tab = std::make_unique<arc::CustomTab>(arc_window);
  auto web_contents = arc::CreateArcCustomTabWebContents(profile, url);

  // |custom_tab_browser| will be destroyed when its tab strip becomes empty,
  // either due to the user opening the custom tab page in a tabbed browser or
  // because of the CustomTabSessionImpl object getting destroyed.
  Browser::CreateParams params(Browser::TYPE_CUSTOM_TAB, profile,
                               /*user_gesture=*/true);
  params.omit_from_session_restore = true;
  auto* custom_tab_browser = Browser::Create(params);

  custom_tab_browser->tab_strip_model()->AppendWebContents(
      std::move(web_contents), /* foreground= */ true);

  // TODO(crbug.com/955171): Remove this temporary conversion to InterfacePtr
  // once OnOpenCustomTab from //ash/components/arc/mojom/intent_helper.mojom
  // could take pending_remote directly. Refer to crrev.com/c/1868870.
  auto custom_tab_remote(
      CustomTabSessionImpl::Create(std::move(custom_tab), custom_tab_browser));
  std::move(callback).Run(std::move(custom_tab_remote));
}

void ArcOpenUrlDelegateImpl::OpenChromePageFromArc(ChromePage page) {
  if (auto* it = kOSSettingsMap.find(page); it != kOSSettingsMap.end()) {
    Profile* profile = ProfileManager::GetActiveUserProfile();
    chrome::SettingsWindowManager::GetInstance()->ShowOSSettings(profile,
                                                                 it->second);
    return;
  }

  if (auto* it = kBrowserSettingsMap.find(page);
      it != kBrowserSettingsMap.end()) {
    OpenUrlFromArc(GURL(chrome::kChromeUISettingsURL).Resolve(it->second));
    return;
  }

  if (auto* it = kAboutPagesMap.find(page); it != kAboutPagesMap.end()) {
    OpenUrlFromArc(GURL(it->second));
    return;
  }

  NOTREACHED();
}

void ArcOpenUrlDelegateImpl::OpenAppWithIntent(
    const GURL& start_url,
    arc::mojom::LaunchIntentPtr arc_intent) {
  DCHECK(start_url.is_valid());
  DCHECK(start_url.SchemeIs(url::kHttpsScheme) || net::IsLocalhost(start_url));

  // Fetch the profile associated with ARC. This method should only be called
  // for a |url| which was installed via ARC, and so we want the web app that is
  // opened through here to be installed in the profile associated with ARC.
  const auto* user = user_manager::UserManager::Get()->GetPrimaryUser();
  DCHECK(user);

  // |profile| may be null if sign-in has happened but the profile isn't loaded
  // yet.
  Profile* profile = ash::ProfileHelper::Get()->GetProfileByUser(user);
  if (!profile)
    return;

  web_app::AppId app_id =
      web_app::GenerateAppId(/*manifest_id=*/absl::nullopt, start_url);

  bool app_installed = false;
  auto* proxy = apps::AppServiceProxyFactory::GetForProfile(profile);
  proxy->AppRegistryCache().ForOneApp(
      app_id, [&app_installed](const apps::AppUpdate& update) {
        app_installed = apps_util::IsInstalled(update.Readiness());
      });

  if (!app_installed) {
    if (arc_intent->data)
      OpenUrlFromArc(*arc_intent->data);
    return;
  }

  apps::mojom::IntentPtr intent = ConvertLaunchIntent(profile, arc_intent);

  auto disposition = WindowOpenDisposition::NEW_WINDOW;
  proxy->AppRegistryCache().ForOneApp(
      app_id, [&disposition](const apps::AppUpdate& update) {
        if (update.WindowMode() == apps::WindowMode::kBrowser) {
          disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
        }
      });

  int event_flags = apps::GetEventFlags(disposition,
                                        /*prefer_container=*/false);

  proxy->LaunchAppWithIntent(app_id, event_flags, std::move(intent),
                             apps::mojom::LaunchSource::kFromArc);
}

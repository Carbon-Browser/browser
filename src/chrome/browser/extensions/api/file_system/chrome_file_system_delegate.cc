// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/file_system/chrome_file_system_delegate.h"

#include <string>
#include <utility>
#include <vector>

#include "apps/saved_files_service.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"
#include "chrome/browser/download/download_core_service.h"
#include "chrome/browser/download/download_core_service_factory.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/extensions/api/file_system/file_entry_picker.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/apps/directory_access_confirmation_dialog.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/file_handlers/app_file_handler_util.h"
#include "extensions/browser/api/file_system/saved_files_service_interface.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/api/file_system.h"
#include "extensions/common/extension.h"
#include "storage/browser/file_system/external_mount_points.h"
#include "storage/browser/file_system/isolated_context.h"
#include "storage/common/file_system/file_system_types.h"
#include "storage/common/file_system/file_system_util.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "ui/shell_dialogs/select_file_dialog.h"

#if BUILDFLAG(IS_MAC)
#include <CoreFoundation/CoreFoundation.h>
#include "base/mac/foundation_util.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/file_manager/volume_manager.h"
#include "chrome/browser/extensions/api/file_system/consent_provider.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"
#endif

namespace extensions {

namespace file_system = api::file_system;

#if BUILDFLAG(IS_CHROMEOS_ASH)
using file_system_api::ConsentProvider;
using file_system_api::ConsentProviderDelegate;

namespace {

const char kConsentImpossible[] =
    "Impossible to ask for user consent as there is no app window visible.";
const char kNotSupportedOnNonKioskSessionError[] =
    "Operation only supported for kiosk apps running in a kiosk session.";
const char kRequiresFileSystemWriteError[] =
    "Operation requires fileSystem.write permission";
const char kSecurityError[] = "Security error.";
const char kVolumeNotFoundError[] = "Volume not found.";

// Fills a list of volumes mounted in the system.
void FillVolumeList(content::BrowserContext* browser_context,
                    std::vector<file_system::Volume>* result) {
  file_manager::VolumeManager* const volume_manager =
      file_manager::VolumeManager::Get(browser_context);
  DCHECK(volume_manager);

  const auto& volume_list = volume_manager->GetVolumeList();
  // Convert volume_list to result_volume_list.
  for (const auto& volume : volume_list) {
    file_system::Volume result_volume;
    result_volume.volume_id = volume->volume_id();
    result_volume.writable = !volume->is_read_only();
    result->push_back(std::move(result_volume));
  }
}

// Callback called when consent is granted or denied.
void OnConsentReceived(content::BrowserContext* browser_context,
                       scoped_refptr<ExtensionFunction> requester,
                       FileSystemDelegate::FileSystemCallback success_callback,
                       FileSystemDelegate::ErrorCallback error_callback,
                       const url::Origin& origin,
                       const base::WeakPtr<file_manager::Volume>& volume,
                       bool writable,
                       ConsentProvider::Consent result) {
  using file_manager::Volume;
  using file_manager::VolumeManager;

  // Render frame host can be gone before this callback method is executed.
  if (!requester->render_frame_host()) {
    std::move(error_callback).Run(std::string());
    return;
  }

  switch (result) {
    case ConsentProvider::CONSENT_REJECTED:
      std::move(error_callback).Run(kSecurityError);
      return;

    case ConsentProvider::CONSENT_IMPOSSIBLE:
      std::move(error_callback).Run(kConsentImpossible);
      return;

    case ConsentProvider::CONSENT_GRANTED:
      break;
  }

  if (!volume.get()) {
    std::move(error_callback).Run(kVolumeNotFoundError);
    return;
  }

  DCHECK_EQ(origin.scheme(), kExtensionScheme);
  scoped_refptr<storage::FileSystemContext> file_system_context =
      util::GetStoragePartitionForExtensionId(origin.host(), browser_context)
          ->GetFileSystemContext();
  storage::ExternalFileSystemBackend* const backend =
      file_system_context->external_backend();
  DCHECK(backend);

  base::FilePath virtual_path;
  if (!backend->GetVirtualPath(volume->mount_path(), &virtual_path)) {
    std::move(error_callback).Run(kSecurityError);
    return;
  }

  storage::IsolatedContext* const isolated_context =
      storage::IsolatedContext::GetInstance();
  DCHECK(isolated_context);

  const storage::FileSystemURL original_url =
      file_system_context->CreateCrackedFileSystemURL(
          blink::StorageKey(origin), storage::kFileSystemTypeExternal,
          virtual_path);

  // Set a fixed register name, as the automatic one would leak the mount point
  // directory.
  std::string register_name = "fs";
  const storage::IsolatedContext::ScopedFSHandle file_system =
      isolated_context->RegisterFileSystemForPath(
          storage::kFileSystemTypeLocalForPlatformApp,
          std::string() /* file_system_id */, original_url.path(),
          &register_name);
  if (!file_system.is_valid()) {
    std::move(error_callback).Run(kSecurityError);
    return;
  }

  backend->GrantFileAccessToOrigin(origin, virtual_path);

  // Grant file permissions to the renderer hosting component.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  DCHECK(policy);

  const auto process_id = requester->source_process_id();
  // Read-only permisisons.
  policy->GrantReadFile(process_id, volume->mount_path());
  policy->GrantReadFileSystem(process_id, file_system.id());

  // Additional write permissions.
  if (writable) {
    policy->GrantCreateReadWriteFile(process_id, volume->mount_path());
    policy->GrantCopyInto(process_id, volume->mount_path());
    policy->GrantWriteFileSystem(process_id, file_system.id());
    policy->GrantDeleteFromFileSystem(process_id, file_system.id());
    policy->GrantCreateFileForFileSystem(process_id, file_system.id());
  }

  std::move(success_callback).Run(file_system.id(), register_name);
}

}  // namespace

namespace file_system_api {

void DispatchVolumeListChangeEvent(content::BrowserContext* browser_context) {
  DCHECK(browser_context);
  EventRouter* const event_router = EventRouter::Get(browser_context);
  if (!event_router)  // Possible on shutdown.
    return;

  ExtensionRegistry* const registry = ExtensionRegistry::Get(browser_context);
  if (!registry)  // Possible on shutdown.
    return;

  ConsentProviderDelegate consent_provider_delegate(
      Profile::FromBrowserContext(browser_context));
  ConsentProvider consent_provider(&consent_provider_delegate);

  file_system::VolumeListChangedEvent event_args;
  FillVolumeList(browser_context, &event_args.volumes);
  for (const auto& extension : registry->enabled_extensions()) {
    if (!consent_provider.IsGrantable(*extension.get()))
      continue;

    event_router->DispatchEventToExtension(
        extension->id(),
        std::make_unique<Event>(
            events::FILE_SYSTEM_ON_VOLUME_LIST_CHANGED,
            file_system::OnVolumeListChanged::kEventName,
            file_system::OnVolumeListChanged::Create(event_args)));
  }
}

}  // namespace file_system_api
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

/******** ChromeFileSystemDelegate ********/

ChromeFileSystemDelegate::ChromeFileSystemDelegate() = default;

ChromeFileSystemDelegate::~ChromeFileSystemDelegate() = default;

base::FilePath ChromeFileSystemDelegate::GetDefaultDirectory() {
  base::FilePath documents_dir;
  base::PathService::Get(chrome::DIR_USER_DOCUMENTS, &documents_dir);
  return documents_dir;
}

base::FilePath ChromeFileSystemDelegate::GetManagedSaveAsDirectory(
    content::BrowserContext* browser_context,
    const Extension& extension) {
  if (extension.id() != extension_misc::kPdfExtensionId)
    return base::FilePath();

  ChromeDownloadManagerDelegate* download_manager =
      DownloadCoreServiceFactory::GetForBrowserContext(browser_context)
          ->GetDownloadManagerDelegate();
  DownloadPrefs* download_prefs = download_manager->download_prefs();
  if (!download_prefs->IsDownloadPathManaged())
    return base::FilePath();
  return download_prefs->DownloadPath();
}

bool ChromeFileSystemDelegate::ShowSelectFileDialog(
    scoped_refptr<ExtensionFunction> extension_function,
    ui::SelectFileDialog::Type type,
    const base::FilePath& default_path,
    const ui::SelectFileDialog::FileTypeInfo* file_types,
    FileSystemDelegate::FilesSelectedCallback files_selected_callback,
    base::OnceClosure file_selection_canceled_callback) {
  const Extension* extension = extension_function->extension();
  content::WebContents* web_contents =
      extension_function->GetSenderWebContents();

  if (!web_contents)
    return false;

  // TODO(asargent/benwells) - As a short term remediation for
  // crbug.com/179010 we're adding the ability for a allowlisted extension to
  // use this API since chrome.fileBrowserHandler.selectFile is ChromeOS-only.
  // Eventually we'd like a better solution and likely this code will go back
  // to being platform-app only.

  // Make sure there is an app window associated with the web contents, so that
  // platform apps cannot open the file picker from a background page.
  // TODO(michaelpg): As a workaround for https://crbug.com/736930, allow this
  // to work from a background page for non-platform apps (which, in practice,
  // is restricted to allowlisted extensions).
  if (extension->is_platform_app() &&
      !AppWindowRegistry::Get(extension_function->browser_context())
           ->GetAppWindowForWebContents(web_contents)) {
    return false;
  }

  // The file picker will hold a reference to the ExtensionFunction
  // instance, preventing its destruction (and subsequent sending of the
  // function response) until the user has selected a file or cancelled the
  // picker. At that point, the picker will delete itself, which will also free
  // the function instance.
  new FileEntryPicker(web_contents, default_path, *file_types, type,
                      std::move(files_selected_callback),
                      std::move(file_selection_canceled_callback));
  return true;
}

void ChromeFileSystemDelegate::ConfirmSensitiveDirectoryAccess(
    bool has_write_permission,
    const std::u16string& app_name,
    content::WebContents* web_contents,
    base::OnceClosure on_accept,
    base::OnceClosure on_cancel) {
  CreateDirectoryAccessConfirmationDialog(has_write_permission, app_name,
                                          web_contents, std::move(on_accept),
                                          std::move(on_cancel));
}

int ChromeFileSystemDelegate::GetDescriptionIdForAcceptType(
    const std::string& accept_type) {
  if (accept_type == "image/*")
    return IDS_IMAGE_FILES;
  if (accept_type == "audio/*")
    return IDS_AUDIO_FILES;
  if (accept_type == "video/*")
    return IDS_VIDEO_FILES;
  return 0;
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
bool ChromeFileSystemDelegate::IsGrantable(
    content::BrowserContext* browser_context,
    const Extension& extension) {
  // Only kiosk apps in kiosk sessions can use this API.
  // Additionally it is enabled for allowlisted component extensions and apps.
  ConsentProviderDelegate consent_provider_delegate(
      Profile::FromBrowserContext(browser_context));
  return ConsentProvider(&consent_provider_delegate).IsGrantable(extension);
}

void ChromeFileSystemDelegate::RequestFileSystem(
    content::BrowserContext* browser_context,
    scoped_refptr<ExtensionFunction> requester,
    const Extension& extension,
    std::string volume_id,
    bool writable,
    FileSystemCallback success_callback,
    ErrorCallback error_callback) {
  ConsentProviderDelegate consent_provider_delegate(
      Profile::FromBrowserContext(browser_context));
  ConsentProvider consent_provider(&consent_provider_delegate);

  using file_manager::Volume;
  using file_manager::VolumeManager;
  VolumeManager* const volume_manager = VolumeManager::Get(browser_context);
  DCHECK(volume_manager);

  if (writable &&
      !app_file_handler_util::HasFileSystemWritePermission(&extension)) {
    std::move(error_callback).Run(kRequiresFileSystemWriteError);
    return;
  }

  if (!consent_provider.IsGrantable(extension)) {
    std::move(error_callback).Run(kNotSupportedOnNonKioskSessionError);
    return;
  }

  base::WeakPtr<file_manager::Volume> volume =
      volume_manager->FindVolumeById(volume_id);
  if (!volume.get()) {
    std::move(error_callback).Run(kVolumeNotFoundError);
    return;
  }

  scoped_refptr<storage::FileSystemContext> file_system_context =
      util::GetStoragePartitionForExtensionId(extension.id(), browser_context)
          ->GetFileSystemContext();
  storage::ExternalFileSystemBackend* const backend =
      file_system_context->external_backend();
  DCHECK(backend);

  base::FilePath virtual_path;
  if (!backend->GetVirtualPath(volume->mount_path(), &virtual_path)) {
    std::move(error_callback).Run(kSecurityError);
    return;
  }

  if (writable && (volume->is_read_only())) {
    std::move(error_callback).Run(kSecurityError);
    return;
  }

  ConsentProvider::ConsentCallback callback =
      base::BindOnce(&OnConsentReceived, browser_context, requester,
                     std::move(success_callback), std::move(error_callback),
                     extension.origin(), volume, writable);

  consent_provider.RequestConsent(requester->render_frame_host(), extension,
                                  volume->volume_id(), volume->volume_label(),
                                  writable, std::move(callback));
}

void ChromeFileSystemDelegate::GetVolumeList(
    content::BrowserContext* browser_context,
    VolumeListCallback success_callback,
    ErrorCallback error_callback) {
  std::vector<file_system::Volume> result_volume_list;
  FillVolumeList(browser_context, &result_volume_list);

  std::move(success_callback).Run(result_volume_list);
}

#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

SavedFilesServiceInterface* ChromeFileSystemDelegate::GetSavedFilesService(
    content::BrowserContext* browser_context) {
  return apps::SavedFilesService::Get(browser_context);
}

}  // namespace extensions

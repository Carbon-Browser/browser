// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_
#define CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_

#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/dlp/dlp_service.pb.h"
#include "storage/browser/file_system/file_system_url.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom-forward.h"
#include "url/gurl.h"

namespace storage {
class FileSystemURL;
}  // namespace storage

class Profile;

namespace policy {

// DlpFilesController is responsible for deciding whether file transfers are
// allowed according to the files sources saved in the DLP daemon and the rules
// of the Data leak prevention policy set by the admin.
class DlpFilesController {
 public:
  // DlpFileMetadata keeps metadata about a file, such as whether it's managed
  // or not and the source URL, if it exists.
  struct DlpFileMetadata {
    DlpFileMetadata() = delete;
    DlpFileMetadata(const std::string& source_url, bool is_dlp_restricted);

    friend bool operator==(const DlpFileMetadata& a, const DlpFileMetadata& b) {
      return a.is_dlp_restricted == b.is_dlp_restricted &&
             a.source_url == b.source_url;
    }
    friend bool operator!=(const DlpFileMetadata& a, const DlpFileMetadata& b) {
      return !(a == b);
    }

    // Source URL from which the file was downloaded.
    std::string source_url;
    // Whether the file is under any DLP rule or not.
    bool is_dlp_restricted;
  };

  using GetDisallowedTransfersCallback =
      base::OnceCallback<void(std::vector<storage::FileSystemURL>)>;
  using GetFilesRestrictedByAnyRuleCallback = GetDisallowedTransfersCallback;
  using FilterDisallowedUploadsCallback = base::OnceCallback<void(
      std::vector<blink::mojom::FileChooserFileInfoPtr>)>;
  using GetDlpMetadataCallback =
      base::OnceCallback<void(std::vector<DlpFileMetadata>)>;

  DlpFilesController();
  DlpFilesController(const DlpFilesController& other) = delete;
  DlpFilesController& operator=(const DlpFilesController& other) = delete;

  ~DlpFilesController();

  // Returns a list of files disallowed to be transferred in |result_callback|.
  void GetDisallowedTransfers(
      std::vector<storage::FileSystemURL> transferred_files,
      storage::FileSystemURL destination,
      GetDisallowedTransfersCallback result_callback);

  // Retrieves metadata for each entry in |files| and returns it as a list in
  // |result_callback|.
  void GetDlpMetadata(std::vector<storage::FileSystemURL> files,
                      GetDlpMetadataCallback result_callback);

  // Filters files disallowed to be uploaded to `destination`.
  void FilterDisallowedUploads(
      std::vector<blink::mojom::FileChooserFileInfoPtr> uploaded_files,
      const GURL& destination,
      FilterDisallowedUploadsCallback result_callback);

  // Returns a list of `files_sources` from from which files aren't allowed to
  // be transferred to `destination`.
  static std::vector<GURL> IsFilesTransferRestricted(
      Profile* profile,
      std::vector<GURL> files_sources,
      std::string destination);

 private:
  void ReturnDisallowedTransfers(
      base::flat_map<std::string, storage::FileSystemURL> files_map,
      GetDisallowedTransfersCallback result_callback,
      dlp::CheckFilesTransferResponse response);

  void ReturnAllowedUploads(
      std::vector<blink::mojom::FileChooserFileInfoPtr> uploaded_files,
      FilterDisallowedUploadsCallback result_callback,
      dlp::CheckFilesTransferResponse response);
  void ReturnDlpMetadata(std::vector<absl::optional<ino_t>> inodes,
                         GetDlpMetadataCallback result_callback,
                         const dlp::GetFilesSourcesResponse response);

  base::WeakPtrFactory<DlpFilesController> weak_ptr_factory_{this};
};

}  // namespace policy

#endif  // CHROME_BROWSER_ASH_POLICY_DLP_DLP_FILES_CONTROLLER_H_

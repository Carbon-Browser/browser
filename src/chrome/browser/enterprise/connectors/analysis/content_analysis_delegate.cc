// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enterprise/connectors/analysis/content_analysis_delegate.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/enterprise/connectors/analysis/analysis_settings.h"
#include "chrome/browser/enterprise/connectors/analysis/content_analysis_dialog.h"
#include "chrome/browser/enterprise/connectors/analysis/files_request_handler.h"
#include "chrome/browser/enterprise/connectors/analysis/page_print_analysis_request.h"
#include "chrome/browser/enterprise/connectors/common.h"
#include "chrome/browser/enterprise/connectors/connectors_service.h"
#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router.h"
#include "chrome/browser/file_util_service.h"
#include "chrome/browser/policy/dm_token_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/cloud_content_scanning/binary_upload_service.h"
#include "chrome/browser/safe_browsing/cloud_content_scanning/deep_scanning_utils.h"
#include "chrome/browser/safe_browsing/cloud_content_scanning/file_analysis_request.h"
#include "chrome/browser/safe_browsing/download_protection/check_client_download_request.h"
#include "chrome/grit/generated_resources.h"
#include "components/enterprise/common/proto/connectors.pb.h"
#include "components/policy/core/common/chrome_schema.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/features.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/url_matcher/url_matcher.h"
#include "content/public/browser/web_contents.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "net/base/mime_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"

using safe_browsing::BinaryUploadService;

namespace enterprise_connectors {

namespace {

// Global pointer of factory function (RepeatingCallback) used to create
// instances of ContentAnalysisDelegate in tests.  !is_null() only in tests.
ContentAnalysisDelegate::Factory* GetFactoryStorage() {
  static base::NoDestructor<ContentAnalysisDelegate::Factory> factory;
  return factory.get();
}

// A BinaryUploadService::Request implementation that gets the data to scan
// from a string.
class StringAnalysisRequest : public BinaryUploadService::Request {
 public:
  StringAnalysisRequest(CloudOrLocalAnalysisSettings settings,
                        std::string text,
                        BinaryUploadService::ContentAnalysisCallback callback);
  ~StringAnalysisRequest() override;

  StringAnalysisRequest(const StringAnalysisRequest&) = delete;
  StringAnalysisRequest& operator=(const StringAnalysisRequest&) = delete;

  // BinaryUploadService::Request implementation.
  void GetRequestData(DataCallback callback) override;

 private:
  Data data_;
  BinaryUploadService::Result result_ =
      BinaryUploadService::Result::FILE_TOO_LARGE;
};

StringAnalysisRequest::StringAnalysisRequest(
    CloudOrLocalAnalysisSettings settings,
    std::string text,
    BinaryUploadService::ContentAnalysisCallback callback)
    : Request(std::move(callback), std::move(settings)) {
  data_.size = text.size();

  // Only remember strings less than the maximum allowed.
  if (text.size() < BinaryUploadService::kMaxUploadSizeBytes) {
    data_.contents = std::move(text);
    result_ = BinaryUploadService::Result::SUCCESS;
  }
  safe_browsing::IncrementCrashKey(
      safe_browsing::ScanningCrashKey::PENDING_TEXT_UPLOADS);
  safe_browsing::IncrementCrashKey(
      safe_browsing::ScanningCrashKey::TOTAL_TEXT_UPLOADS);
}

StringAnalysisRequest::~StringAnalysisRequest() {
  safe_browsing::DecrementCrashKey(
      safe_browsing::ScanningCrashKey::PENDING_TEXT_UPLOADS);
}

void StringAnalysisRequest::GetRequestData(DataCallback callback) {
  std::move(callback).Run(result_, std::move(data_));
}

bool* UIEnabledStorage() {
  static bool enabled = true;
  return &enabled;
}

}  // namespace

ContentAnalysisDelegate::Data::Data() = default;
ContentAnalysisDelegate::Data::Data(Data&& other) = default;
ContentAnalysisDelegate::Data::~Data() = default;

ContentAnalysisDelegate::Result::Result() = default;
ContentAnalysisDelegate::Result::Result(Result&& other) = default;
ContentAnalysisDelegate::Result::~Result() = default;

ContentAnalysisDelegate::~ContentAnalysisDelegate() = default;

void ContentAnalysisDelegate::BypassWarnings(
    absl::optional<std::u16string> user_justification) {
  if (callback_.is_null())
    return;

  // Mark the full text as complying and report a warning bypass.
  if (text_warning_) {
    std::fill(result_.text_results.begin(), result_.text_results.end(), true);

    int64_t content_size = 0;
    for (const std::string& entry : data_.text)
      content_size += entry.size();

    ReportAnalysisConnectorWarningBypass(
        profile_, url_, "Text data", std::string(), "text/plain",
        extensions::SafeBrowsingPrivateEventRouter::kTriggerWebContentUpload,
        access_point_, content_size, text_response_, user_justification);
  }

  if (!warned_file_indices_.empty()) {
    files_request_handler_->ReportWarningBypass(user_justification);
    // Mark every warned file as complying.
    for (size_t index : warned_file_indices_) {
      result_.paths_results[index] = true;
    }
  }

  // Mark the printed page as complying and report a warning bypass.
  if (page_warning_) {
    result_.page_result = true;

    ReportAnalysisConnectorWarningBypass(
        profile_, url_, "Printed page", /*sha256*/ std::string(),
        /*mime_type*/ std::string(),
        extensions::SafeBrowsingPrivateEventRouter::kTriggerPagePrint,
        access_point_, /*content_size*/ -1, page_response_, user_justification);
  }

  RunCallback();
}

void ContentAnalysisDelegate::Cancel(bool warning) {
  if (callback_.is_null())
    return;

  // Don't report this upload as cancelled if the user didn't bypass the
  // warning.
  if (!warning) {
    RecordDeepScanMetrics(access_point_,
                          base::TimeTicks::Now() - upload_start_time_, 0,
                          "CancelledByUser", false);
  }

  // Make sure to reject everything.
  FillAllResultsWith(false);
  RunCallback();
}

absl::optional<std::u16string> ContentAnalysisDelegate::GetCustomMessage()
    const {
  auto element = data_.settings.tags.find(final_result_tag_);
  if (element != data_.settings.tags.end() &&
      !element->second.custom_message.message.empty()) {
    return l10n_util::GetStringFUTF16(IDS_DEEP_SCANNING_DIALOG_CUSTOM_MESSAGE,
                                      element->second.custom_message.message);
  }

  return absl::nullopt;
}

absl::optional<GURL> ContentAnalysisDelegate::GetCustomLearnMoreUrl() const {
  auto element = data_.settings.tags.find(final_result_tag_);
  if (element != data_.settings.tags.end() &&
      element->second.custom_message.learn_more_url.is_valid() &&
      !element->second.custom_message.learn_more_url.is_empty()) {
    return element->second.custom_message.learn_more_url;
  }

  return absl::nullopt;
}

bool ContentAnalysisDelegate::BypassRequiresJustification() const {
  if (!base::FeatureList::IsEnabled(kBypassJustificationEnabled))
    return false;

  return data_.settings.tags.count(final_result_tag_) &&
         data_.settings.tags.at(final_result_tag_).requires_justification;
}

std::u16string ContentAnalysisDelegate::GetBypassJustificationLabel() const {
  return l10n_util::GetStringUTF16(
      IDS_DEEP_SCANNING_DIALOG_UPLOAD_BYPASS_JUSTIFICATION_LABEL);
}

absl::optional<std::u16string>
ContentAnalysisDelegate::OverrideCancelButtonText() const {
  return absl::nullopt;
}

// static
bool ContentAnalysisDelegate::IsEnabled(
    Profile* profile,
    GURL url,
    Data* data,
    enterprise_connectors::AnalysisConnector connector) {
  auto* service =
      enterprise_connectors::ConnectorsServiceFactory::GetForBrowserContext(
          profile);
  // If the corresponding Connector policy isn't set, don't perform scans.
  if (!service || !service->IsConnectorEnabled(connector))
    return false;

  // Check that |url| matches the appropriate URL patterns by getting settings.
  // No settings means no matches were found.
  auto settings = service->GetAnalysisSettings(url, connector);
  if (!settings.has_value()) {
    return false;
  }

  data->settings = std::move(settings.value());
  if (url.is_valid())
    data->url = url;

  return true;
}

// static
void ContentAnalysisDelegate::CreateForWebContents(
    content::WebContents* web_contents,
    Data data,
    CompletionCallback callback,
    safe_browsing::DeepScanAccessPoint access_point) {
  Factory* testing_factory = GetFactoryStorage();
  bool wait_for_verdict = data.settings.block_until_verdict ==
                          enterprise_connectors::BlockUntilVerdict::kBlock;
  // Using new instead of std::make_unique<> to access non public constructor.
  auto delegate = testing_factory->is_null()
                      ? base::WrapUnique(new ContentAnalysisDelegate(
                            web_contents, std::move(data), std::move(callback),
                            access_point))
                      : testing_factory->Run(web_contents, std::move(data),
                                             std::move(callback));

  bool work_being_done = delegate->UploadData();

  // Only show UI if work is being done in the background, the user must
  // wait for a verdict.
  bool show_ui = work_being_done && wait_for_verdict && (*UIEnabledStorage());

  // If the UI is enabled, create the modal dialog.
  if (show_ui) {
    ContentAnalysisDelegate* delegate_ptr = delegate.get();
    int files_count = delegate_ptr->data_.paths.size();

    // This dialog is owned by the constrained_window code.
    delegate_ptr->dialog_ = new ContentAnalysisDialog(
        std::move(delegate), web_contents, access_point, files_count);
    return;
  }

  if (!wait_for_verdict || !work_being_done) {
    // The UI will not be shown but the policy is set to not wait for the
    // verdict, or no scans need to be performed.  Inform the caller that they
    // may proceed.
    //
    // Supporting "wait for verdict" while not showing a UI makes writing tests
    // for callers of this code easier.
    delegate->FillAllResultsWith(true);
    delegate->RunCallback();
  }

  // Upload service callback will delete the delegate.
  if (work_being_done)
    delegate.release();
}

// static
void ContentAnalysisDelegate::SetFactoryForTesting(Factory factory) {
  *GetFactoryStorage() = factory;
}

// static
void ContentAnalysisDelegate::ResetFactoryForTesting() {
  if (GetFactoryStorage())
    GetFactoryStorage()->Reset();
}

// static
void ContentAnalysisDelegate::DisableUIForTesting() {
  *UIEnabledStorage() = false;
}

ContentAnalysisDelegate::ContentAnalysisDelegate(
    content::WebContents* web_contents,
    Data data,
    CompletionCallback callback,
    safe_browsing::DeepScanAccessPoint access_point)
    : data_(std::move(data)),
      callback_(std::move(callback)),
      access_point_(access_point) {
  DCHECK(web_contents);
  profile_ = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  url_ = web_contents->GetLastCommittedURL();
  result_.text_results.resize(data_.text.size(), false);
  result_.paths_results.resize(data_.paths.size(), false);
  result_.page_result = false;
}

void ContentAnalysisDelegate::StringRequestCallback(
    BinaryUploadService::Result result,
    enterprise_connectors::ContentAnalysisResponse response) {
  int64_t content_size = 0;
  for (const std::string& entry : data_.text)
    content_size += entry.size();
  RecordDeepScanMetrics(access_point_,
                        base::TimeTicks::Now() - upload_start_time_,
                        content_size, result, response);

  text_request_complete_ = true;

  RequestHandlerResult request_handler_result =
      CalculateRequestHandlerResult(data_.settings, result, response);

  bool text_complies = request_handler_result.complies;
  bool should_warn = request_handler_result.final_result ==
                     FinalContentAnalysisResult::WARNING;

  std::fill(result_.text_results.begin(), result_.text_results.end(),
            text_complies);

  MaybeReportDeepScanningVerdict(
      profile_, url_, "Text data", std::string(), "text/plain",
      extensions::SafeBrowsingPrivateEventRouter::kTriggerWebContentUpload,
      access_point_, content_size, result, response,
      CalculateEventResult(data_.settings, text_complies, should_warn));

  UpdateFinalResult(request_handler_result.final_result,
                    request_handler_result.tag);

  if (should_warn) {
    text_warning_ = true;
    text_response_ = std::move(response);
  }

  MaybeCompleteScanRequest();
}

void ContentAnalysisDelegate::FilesRequestCallback(
    std::vector<RequestHandlerResult> results) {
  // No reporting here, because the MultiFileRequestHandler does that.
  DCHECK_EQ(results.size(), result_.paths_results.size());
  for (size_t index = 0; index < results.size(); ++index) {
    FinalContentAnalysisResult result = results[index].final_result;
    result_.paths_results[index] = results[index].complies;
    if (result == FinalContentAnalysisResult::WARNING) {
      warned_file_indices_.push_back(index);
    }
    UpdateFinalResult(result, results[index].tag);
  }
  files_request_complete_ = true;

  MaybeCompleteScanRequest();
}

FilesRequestHandler*
ContentAnalysisDelegate::GetFilesRequestHandlerForTesting() {
  return files_request_handler_.get();
}

bool ContentAnalysisDelegate::ShowFinalResultInDialog() {
  if (!dialog_)
    return false;

  dialog_->ShowResult(final_result_);
  return true;
}

bool ContentAnalysisDelegate::CancelDialog() {
  if (!dialog_)
    return false;

  dialog_->CancelDialog();
  return true;
}

void ContentAnalysisDelegate::PageRequestCallback(
    BinaryUploadService::Result result,
    enterprise_connectors::ContentAnalysisResponse response) {
  RecordDeepScanMetrics(access_point_,
                        base::TimeTicks::Now() - upload_start_time_,
                        page_size_bytes_, result, response);

  page_request_complete_ = true;

  RequestHandlerResult request_handler_result =
      CalculateRequestHandlerResult(data_.settings, result, response);

  result_.page_result = request_handler_result.complies;
  bool should_warn = request_handler_result.final_result ==
                     FinalContentAnalysisResult::WARNING;

  MaybeReportDeepScanningVerdict(
      profile_, url_, "Printed page", /*sha256*/ std::string(),
      /*mime_type*/ std::string(),
      extensions::SafeBrowsingPrivateEventRouter::kTriggerPagePrint,
      access_point_, /*content_size*/ -1, result, response,
      CalculateEventResult(data_.settings, result_.page_result, should_warn));

  UpdateFinalResult(request_handler_result.final_result,
                    request_handler_result.tag);

  if (should_warn) {
    page_warning_ = true;
    page_response_ = std::move(response);
  }

  MaybeCompleteScanRequest();
}

bool ContentAnalysisDelegate::UploadData() {
  upload_start_time_ = base::TimeTicks::Now();

  // Create a text request, a page request and a file request for each file.
  PrepareTextRequest();
  PreparePageRequest();

  if (!data_.paths.empty()) {
    // Passing the settings using a reference is safe here, because
    // MultiFileRequestHandler is owned by this class.
    files_request_handler_ = FilesRequestHandler::Create(
        GetBinaryUploadService(), profile_, data_.settings, url_, access_point_,
        data_.paths,
        base::BindOnce(&ContentAnalysisDelegate::FilesRequestCallback,
                       GetWeakPtr()));
    files_request_complete_ = !files_request_handler_->UploadData();
  } else {
    // If no files should be uploaded, the file request is complete.
    files_request_complete_ = true;
  }
  data_uploaded_ = true;
  // Do not add code under this comment. The above line should be the last thing
  // this function does before the return statement.

  return !text_request_complete_ || !files_request_complete_ ||
         !page_request_complete_;
}

void ContentAnalysisDelegate::PrepareTextRequest() {
  std::string full_text;
  for (const std::string& text : data_.text)
    full_text.append(text);

  // The request is considered complete if there is no text or if the text is
  // too small compared to the minimum size. This means a minimum_data_size of
  // 0 is equivalent to no minimum, as the second part of the "or" will always
  // be false.
  text_request_complete_ =
      full_text.empty() || full_text.size() < data_.settings.minimum_data_size;

  if (!full_text.empty()) {
    base::UmaHistogramCustomCounts("Enterprise.OnBulkDataEntry.DataSize",
                                   full_text.size(),
                                   /*min=*/1,
                                   /*max=*/51 * 1024 * 1024,
                                   /*buckets=*/50);
  }

  if (!text_request_complete_) {
    auto request = std::make_unique<StringAnalysisRequest>(
        data_.settings.cloud_or_local_settings, std::move(full_text),
        base::BindOnce(&ContentAnalysisDelegate::StringRequestCallback,
                       weak_ptr_factory_.GetWeakPtr()));

    PrepareRequest(enterprise_connectors::BULK_DATA_ENTRY, request.get());
    UploadTextForDeepScanning(std::move(request));
  }
}

void ContentAnalysisDelegate::PreparePageRequest() {
  // The request is considered complete if the mapped region is invalid since it
  // prevents scanning.
  page_request_complete_ = !data_.page.IsValid();

  if (!page_request_complete_) {
    page_size_bytes_ = data_.page.GetSize();
    auto request = std::make_unique<PagePrintAnalysisRequest>(
        data_.settings, std::move(data_.page),
        base::BindOnce(&ContentAnalysisDelegate::PageRequestCallback,
                       weak_ptr_factory_.GetWeakPtr()));

    PrepareRequest(enterprise_connectors::PRINT, request.get());
    UploadPageForDeepScanning(std::move(request));
  }
}

void ContentAnalysisDelegate::PrepareRequest(
    enterprise_connectors::AnalysisConnector connector,
    BinaryUploadService::Request* request) {
  if (data_.settings.cloud_or_local_settings.is_cloud_analysis()) {
    request->set_device_token(
        data_.settings.cloud_or_local_settings.dm_token());
  }
  request->set_analysis_connector(connector);
  request->set_email(safe_browsing::GetProfileEmail(profile_));
  request->set_url(data_.url.spec());
  request->set_tab_url(data_.url);
  request->set_per_profile_request(data_.settings.per_profile);
  for (const auto& tag : data_.settings.tags)
    request->add_tag(tag.first);
  if (data_.settings.client_metadata)
    request->set_client_metadata(*data_.settings.client_metadata);
}

void ContentAnalysisDelegate::FillAllResultsWith(bool status) {
  std::fill(result_.text_results.begin(), result_.text_results.end(), status);
  std::fill(result_.paths_results.begin(), result_.paths_results.end(), status);
  result_.page_result = status;
}

BinaryUploadService* ContentAnalysisDelegate::GetBinaryUploadService() {
  return safe_browsing::BinaryUploadService::GetForProfile(profile_,
                                                           data_.settings);
}

void ContentAnalysisDelegate::UploadTextForDeepScanning(
    std::unique_ptr<BinaryUploadService::Request> request) {
  BinaryUploadService* upload_service = GetBinaryUploadService();
  if (upload_service)
    upload_service->MaybeUploadForDeepScanning(std::move(request));
}

void ContentAnalysisDelegate::UploadPageForDeepScanning(
    std::unique_ptr<BinaryUploadService::Request> request) {
  BinaryUploadService* upload_service = GetBinaryUploadService();
  if (upload_service)
    upload_service->MaybeUploadForDeepScanning(std::move(request));
}

bool ContentAnalysisDelegate::UpdateDialog() {
  // Only show final result UI in the case of a cloud analysis.
  // In the local case, the local agent does that.
  return data_.settings.cloud_or_local_settings.is_cloud_analysis()
             ? ShowFinalResultInDialog()
             : CancelDialog();
}

void ContentAnalysisDelegate::MaybeCompleteScanRequest() {
  if (!text_request_complete_ || !files_request_complete_ ||
      !page_request_complete_) {
    return;
  }

  // If showing the warning message, wait before running the callback. The
  // callback will be called either in BypassWarnings or Cancel.
  if (final_result_ != FinalContentAnalysisResult::WARNING)
    RunCallback();

  if (!UpdateDialog() && data_uploaded_) {
    // No UI was shown.  Delete |this| to cleanup, unless UploadData isn't done
    // yet.
    delete this;
  }
}

void ContentAnalysisDelegate::RunCallback() {
  if (!callback_.is_null())
    std::move(callback_).Run(data_, result_);
}

void ContentAnalysisDelegate::UpdateFinalResult(
    FinalContentAnalysisResult result,
    const std::string& tag) {
  if (result < final_result_) {
    final_result_ = result;
    final_result_tag_ = tag;
  }
}

}  // namespace enterprise_connectors

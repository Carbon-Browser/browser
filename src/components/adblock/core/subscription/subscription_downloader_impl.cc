/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/core/subscription/subscription_downloader_impl.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/safe_sprintf.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"

namespace adblock {
namespace {

// To retain user anonymity, we clamp the download count sent to subscription
// servers - any number higher than 4 is reported as just "4+".
std::string GetClampedDownloadCount(int download_count) {
  DCHECK_GE(download_count, 0);
  if (download_count > 4) {
    return "4+";
  }
  return base::NumberToString(download_count);
}

std::string BuildSubscriptionQueryParams(
    const GURL& subscription_url,
    const SubscriptionPersistentMetadata* persistent_metadata,
    const bool is_disabled) {
  return base::StrCat(
      {"lastVersion=",
       base::EscapeQueryParamValue(
           persistent_metadata->GetVersion(subscription_url), true),
       "&disabled=", is_disabled ? "true" : "false", "&downloadCount=",
       GetClampedDownloadCount(
           persistent_metadata->GetDownloadSuccessCount(subscription_url)),
       "&safe=true"});
}

int GenerateTraceId(const GURL& subscription_url) {
  return std::hash<std::string>{}(subscription_url.spec());
}

// converts |date| into abp version format ex: 202107210821
// in UTC format as necessary for server
std::string ConvertBaseTimeToABPFilterVersionFormat(const base::Time& date) {
  base::Time::Exploded exploded;
  // we receive in GMT and convert to UTC ( which has the same time )
  date.UTCExplode(&exploded);
  char buff[16];
  base::strings::SafeSPrintf(buff, "%04d%02d%02d%02d%02d", exploded.year,
                             exploded.month, exploded.day_of_month,
                             exploded.hour, exploded.minute);
  return std::string(buff);
}

}  // namespace

SubscriptionDownloaderImpl::SubscriptionDownloaderImpl(
    SubscriptionRequestMaker request_maker,
    ConversionExecutors* conversion_executor,
    SubscriptionPersistentMetadata* persistent_metadata)
    : request_maker_(std::move(request_maker)),
      conversion_executor_(conversion_executor),
      persistent_metadata_(persistent_metadata) {}

SubscriptionDownloaderImpl::~SubscriptionDownloaderImpl() = default;

void SubscriptionDownloaderImpl::StartDownload(
    const GURL& subscription_url,
    AdblockResourceRequest::RetryPolicy retry_policy,
    DownloadCompletedCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsUrlAllowed(subscription_url)) {
    LOG(WARNING) << "[eyeo] Download from URL not allowed, will not download "
                 << subscription_url;
    std::move(on_finished).Run(nullptr);
    return;
  }

  ongoing_downloads_[subscription_url] =
      OngoingDownload{request_maker_.Run(), std::move(on_finished)};
  std::get<ResourceRequestPtr>(ongoing_downloads_[subscription_url])
      ->Start(
          subscription_url, AdblockResourceRequest::Method::GET,
          base::BindRepeating(&SubscriptionDownloaderImpl::OnDownloadFinished,
                              weak_ptr_factory_.GetWeakPtr()),
          retry_policy,
          BuildSubscriptionQueryParams(subscription_url, persistent_metadata_,
                                       false));
}

void SubscriptionDownloaderImpl::CancelDownload(const GURL& subscription_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ongoing_downloads_.erase(subscription_url);
}

void SubscriptionDownloaderImpl::DoHeadRequest(
    const GURL& subscription_url,
    HeadRequestCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ongoing_ping_) {
    std::move(std::get<HeadRequestCallback>(*ongoing_ping_)).Run("");
  }

  ongoing_ping_ = HeadRequest{request_maker_.Run(), std::move(on_finished)};
  std::get<ResourceRequestPtr>(*ongoing_ping_)
      ->Start(subscription_url, AdblockResourceRequest::Method::HEAD,
              base::BindRepeating(
                  &SubscriptionDownloaderImpl::OnHeadersOnlyDownloaded,
                  weak_ptr_factory_.GetWeakPtr()),
              // A ping may fail. We don't retry, SubscriptionUpdater will send
              // a new ping in an hour anyway.
              AdblockResourceRequest::RetryPolicy::DoNotRetry,
              BuildSubscriptionQueryParams(subscription_url,
                                           persistent_metadata_, true));
}

bool SubscriptionDownloaderImpl::IsUrlAllowed(
    const GURL& subscription_url) const {
  if (net::IsLocalhost(subscription_url)) {
    // We trust all localhost urls, regardless of scheme.
    return true;
  }
  if (!subscription_url.SchemeIs("https") &&
      !subscription_url.SchemeIs("data")) {
    return false;
  }
  return true;
}

void SubscriptionDownloaderImpl::OnHeadersOnlyDownloaded(
    const GURL&,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(ongoing_ping_.has_value());

  std::string version("");

  // Parse date or Date from response headers.
  if (headers) {
    if (auto date = headers->GetDateValue()) {
      version = ConvertBaseTimeToABPFilterVersionFormat(date.value());
    }
  }

  std::move(std::get<HeadRequestCallback>(*ongoing_ping_))
      .Run(std::move(version));
  ongoing_ping_.reset();
}

void SubscriptionDownloaderImpl::OnDownloadFinished(
    const GURL& subscription_url,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto download_it = ongoing_downloads_.find(subscription_url);
  DCHECK(download_it != ongoing_downloads_.end());

  if (downloaded_file.empty()) {
    persistent_metadata_->IncrementDownloadErrorCount(subscription_url);
    DLOG(WARNING) << "[eyeo] Failed to retrieve content for "
                  << subscription_url << ", will abort";
    std::move(std::get<DownloadCompletedCallback>(download_it->second))
        .Run(nullptr);
    ongoing_downloads_.erase(download_it);
    return;
  }

  VLOG(1) << "[eyeo] Finished downloading " << subscription_url
          << ", starting conversion";

  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      "eyeo", "Converting subscription",
      TRACE_ID_LOCAL(GenerateTraceId(subscription_url)), "url",
      subscription_url.spec());

  conversion_executor_->ConvertFilterListFile(
      subscription_url, downloaded_file,
      base::BindOnce(&SubscriptionDownloaderImpl::OnConversionFinished,
                     weak_ptr_factory_.GetWeakPtr(), subscription_url));
}

void SubscriptionDownloaderImpl::OnConversionFinished(
    const GURL& subscription_url,
    ConversionResult converter_result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      "eyeo", "Converting subscription",
      TRACE_ID_LOCAL(GenerateTraceId(subscription_url)));
  const auto download_it = ongoing_downloads_.find(subscription_url);
  if (download_it == ongoing_downloads_.end()) {
    VLOG(1) << "[eyeo] Conversion result discarded, subscription download "
               "was cancelled.";
    return;
  }

  if (absl::holds_alternative<std::unique_ptr<FlatbufferData>>(
          converter_result)) {
    VLOG(1) << "[eyeo] Finished converting " << subscription_url
            << " successfully";
    std::move(std::get<DownloadCompletedCallback>(download_it->second))
        .Run(std::move(
            absl::get<std::unique_ptr<FlatbufferData>>(converter_result)));
    ongoing_downloads_.erase(download_it);
  } else if (absl::holds_alternative<GURL>(converter_result)) {
    const GURL& redirect_url = absl::get<GURL>(converter_result);
    if (!IsUrlAllowed(redirect_url)) {
      AbortWithWarning(download_it, "Redirect URL not allowed.");
      return;
    }
    if (redirect_url == subscription_url) {
      AbortWithWarning(download_it,
                       "Redirect to the same URL is not permitted.");
      return;
    }
    if (std::get<ResourceRequestPtr>(download_it->second)
            ->GetNumberOfRedirects() >= kMaxNumberOfRedirects) {
      AbortWithWarning(download_it, "Maximum number of redirects exceeded.");
    } else {
      auto ongoing_download = ongoing_downloads_.extract(download_it);
      ongoing_download.key() = redirect_url;
      auto redirected_download_it =
          ongoing_downloads_.insert(std::move(ongoing_download)).position;
      std::get<ResourceRequestPtr>(redirected_download_it->second)
          ->Redirect(redirect_url,
                     BuildSubscriptionQueryParams(redirect_url,
                                                  persistent_metadata_, false));
    }
  } else {
    persistent_metadata_->IncrementDownloadErrorCount(subscription_url);
    AbortWithWarning(download_it,
                     *absl::get<ConversionError>(converter_result));
    return;
  }
}

void SubscriptionDownloaderImpl::AbortWithWarning(
    const OngoingDownloadsIt ongoing_download_it,
    const std::string& warning) {
  if (ongoing_download_it == ongoing_downloads_.end()) {
    return;
  }
  DLOG(WARNING) << "[eyeo] " << warning << " Aborting download of "
                << ongoing_download_it->first;
  std::move(std::get<DownloadCompletedCallback>(ongoing_download_it->second))
      .Run(nullptr);
  ongoing_downloads_.erase(ongoing_download_it);
}

}  // namespace adblock

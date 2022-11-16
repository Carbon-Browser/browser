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

#include "components/adblock/core/subscription/ongoing_subscription_request_impl.h"

#include <memory>

#include "base/strings/string_piece_forward.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/allowed_connection_type.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {
namespace {

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("adblock_subscription_download", R"(
        semantics {
          sender: "SubscriptionDownloaderImpl"
          description:
            "A request to download ad-blocking filter lists, as the user "
            "selects a new filter list source or as a filter list update "
            "is fetched."
          trigger:
            "Interval or when user selects a new filter list source"
          data:
            "Version (timestamp) of the filter list, if present. "
            "Application name (ex. Chromium) "
            "Application version (93.0.4572.0) "
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You enable or disable this feature via 'Adblock Enable' pref."
          policy_exception_justification: "Not implemented."
          }
        })");

}

OngoingSubscriptionRequestImpl::OngoingSubscriptionRequestImpl(
    PrefService* prefs,
    const net::BackoffEntry::Policy* backoff_policy,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : backoff_entry_(std::make_unique<net::BackoffEntry>(backoff_policy)),
      url_loader_factory_(url_loader_factory),
      retry_timer_(std::make_unique<base::OneShotTimer>()),
      number_of_redirects_(0) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  allowed_connection_type_.Init(
      prefs::kAdblockAllowedConnectionType, prefs,
      base::BindRepeating(
          &OngoingSubscriptionRequestImpl::CheckAllowedConnectionType,
          // Unretained is safe because destruction of |this| will
          // remove |allowed_connection_type_| and abort the callback.
          base::Unretained(this)));
  adblocking_enabled_.Init(
      prefs::kEnableAdblock, prefs,
      base::BindRepeating(
          &OngoingSubscriptionRequestImpl::CheckAllowedConnectionType,
          base::Unretained(this)));
}

OngoingSubscriptionRequestImpl::~OngoingSubscriptionRequestImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void OngoingSubscriptionRequestImpl::Start(GURL url,
                                           Method method,
                                           ResponseCallback response_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(url_.is_empty()) << "Start() called twice";
  url_ = std::move(url);
  method_ = method;
  response_callback_ = std::move(response_callback);
  if (IsConnectionAllowed()) {
    StartInternal();
  } else {
    DLOG(INFO) << "[eyeo] Deferring download of " << url_
               << " due to current download policy.";
  }
}

void OngoingSubscriptionRequestImpl::Retry() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!url_.is_empty()) << "Retry() called before Start()";
  if (!url_loader_factory_) {
    // This happens in unit tests that have no network.
    return;
  }
  backoff_entry_->InformOfRequest(false);
  DLOG(INFO) << "[eyeo] Will retry downloading " << url_ << " in "
             << backoff_entry_->GetTimeUntilRelease();
  retry_timer_->Start(
      FROM_HERE, backoff_entry_->GetTimeUntilRelease(),
      base::BindOnce(&OngoingSubscriptionRequestImpl::StartInternal,
                     // Unretained is safe because destruction of |this| will
                     // remove |retry_timer_| and abort the callback.
                     base::Unretained(this)));
}

void OngoingSubscriptionRequestImpl::Redirect(GURL redirect_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!url_.is_empty()) << "Redirect() called before Start()";
  DCHECK(url_ != redirect_url) << "Invalid redirect. Same URL";
  DLOG(INFO) << "[eyeo] Will redirect " << url_ << " to " << redirect_url;
  ++number_of_redirects_;
  url_ = std::move(redirect_url);
  StartInternal();
}

size_t OngoingSubscriptionRequestImpl::GetNumberOfRedirects() const {
  return number_of_redirects_;
}

void OngoingSubscriptionRequestImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CheckAllowedConnectionType();
}

bool OngoingSubscriptionRequestImpl::IsStarted() const {
  return loader_ != nullptr || retry_timer_->IsRunning();
}

bool OngoingSubscriptionRequestImpl::IsConnectionAllowed() const {
  if (!adblocking_enabled_.GetValue())
    return false;
  if (method_ == Method::HEAD)
    return true;
  const auto allowed_connection_type =
      AllowedConnectionTypeFromString(allowed_connection_type_.GetValue());
  if (!allowed_connection_type)
    return true;
  if (*allowed_connection_type == AllowedConnectionType::kWiFi) {
    return net::NetworkChangeNotifier::GetConnectionType() ==
           net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI;
  }
  DCHECK_EQ(*allowed_connection_type, AllowedConnectionType::kAny);
  return true;
}

void OngoingSubscriptionRequestImpl::CheckAllowedConnectionType() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (IsConnectionAllowed() && !IsStarted()) {
    StartInternal();
    DCHECK(IsStarted());
  } else if (!IsConnectionAllowed() && IsStarted()) {
    StopInternal();
    DCHECK(!IsStarted());
  }
}

void OngoingSubscriptionRequestImpl::StartInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN2("eyeo", "Downloading subscription", this,
                                    "url", url_.spec(), "method",
                                    MethodToString());
  if (!url_loader_factory_) {
    // This happens in unit tests that have no network. The request will hang
    // indefinitely.
    return;
  }
  DLOG(INFO) << "[eyeo] Downloading " << url_;
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url_;
  request->method = MethodToString();
  loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  if (method_ == Method::GET) {
    loader_->DownloadToTempFile(
        url_loader_factory_.get(),
        base::BindOnce(&OngoingSubscriptionRequestImpl::OnDownloadFinished,
                       // Unretained is safe because destruction of |this| will
                       // remove |loader_| and will abort the callback.
                       base::Unretained(this)));
  } else {
    loader_->DownloadHeadersOnly(
        url_loader_factory_.get(),
        base::BindOnce(&OngoingSubscriptionRequestImpl::OnHeadersReceived,
                       base::Unretained(this)));
  }
}

void OngoingSubscriptionRequestImpl::StopInternal() {
  DLOG(INFO) << "[eyeo] Stopped downloading " << url_
             << " due to current download policy.";
  loader_.reset();
  retry_timer_->Stop();
}

void OngoingSubscriptionRequestImpl::OnDownloadFinished(
    base::FilePath downloaded_file) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0("eyeo", "Downloading subscription", this);
  GURL::Replacements strip_query;
  strip_query.ClearQuery();
  GURL url = url_.ReplaceComponents(strip_query);
  response_callback_.Run(
      url, std::move(downloaded_file),
      loader_->ResponseInfo() ? loader_->ResponseInfo()->headers : nullptr);
  // response_callback_ may delete this, do not call any member variables now.
}

void OngoingSubscriptionRequestImpl::OnHeadersReceived(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT_NESTABLE_ASYNC_END0("eyeo", "Downloading subscription", this);
  response_callback_.Run(GURL(), base::FilePath(), headers);
  // response_callback_ may delete this, do not call any member variables now.
}

const char* OngoingSubscriptionRequestImpl::MethodToString() const {
  return method_ == Method::GET ? net::HttpRequestHeaders::kGetMethod
                                : net::HttpRequestHeaders::kHeadMethod;
}

}  // namespace adblock

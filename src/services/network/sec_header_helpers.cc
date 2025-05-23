// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/sec_header_helpers.h"

#include <algorithm>
#include <string>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/cookie_util.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/cors/origin_access_list.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/initiator_lock_compatibility.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"
#include "services/network/public/cpp/request_destination.h"
#include "services/network/public/cpp/request_mode.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace network {

namespace {

constexpr std::string_view kSecFetchMode = "Sec-Fetch-Mode";
constexpr std::string_view kSecFetchSite = "Sec-Fetch-Site";
constexpr std::string_view kSecFetchUser = "Sec-Fetch-User";
constexpr std::string_view kSecFetchDest = "Sec-Fetch-Dest";
constexpr std::string_view kSecFetchStorageAccess = "Sec-Fetch-Storage-Access";

constexpr char kSecFetchStorageAccessOutcomeHistogram[] =
    "API.StorageAccessHeader.SecFetchStorageAccessOutcome";

// Sec-Fetch-Site infrastructure:
//
// Note that the order of enum values below is significant - it is important for
// std::max invocations that kSameOrigin < kSameSite < kCrossSite.
enum class SecFetchSiteValue {
  kNoOrigin,
  kSameOrigin,
  kSameSite,
  kCrossSite,
};

const char* GetSecFetchSiteHeaderString(const SecFetchSiteValue& value) {
  switch (value) {
    case SecFetchSiteValue::kNoOrigin:
      return "none";
    case SecFetchSiteValue::kSameOrigin:
      return "same-origin";
    case SecFetchSiteValue::kSameSite:
      return "same-site";
    case SecFetchSiteValue::kCrossSite:
      return "cross-site";
  }
}

SecFetchSiteValue GetHeaderValueForTargetAndInitiator(
    const GURL& target_url,
    const url::Origin& initiator) {
  url::Origin target_origin = url::Origin::Create(target_url);

  if (target_origin == initiator)
    return SecFetchSiteValue::kSameOrigin;

  // Cross-scheme initiator should be considered cross-site (even if it's host
  // is same-site with the target).  See also https://crbug.com/979257.
  if (initiator.scheme() == target_origin.scheme() &&
      net::registry_controlled_domains::SameDomainOrHost(
          initiator, target_origin,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return SecFetchSiteValue::kSameSite;
  }

  return SecFetchSiteValue::kCrossSite;
}

SecFetchSiteValue GetHeaderValueForRequest(
    net::URLRequest* request,
    const GURL* pending_redirect_url,
    const mojom::URLLoaderFactoryParams& factory_params,
    const cors::OriginAccessList& origin_access_list) {
  // Browser-initiated requests with no initiator origin will send
  // `Sec-Fetch-Site: None`.
  if (!request->initiator().has_value()) {
    // CorsURLLoaderFactory::IsValidRequest verifies that only the browser
    // process may initiate requests with no request initiator.
    DCHECK_EQ(factory_params.process_id, mojom::kBrowserProcessId);

    return SecFetchSiteValue::kNoOrigin;
  }
  const url::Origin& initiator = request->initiator().value();

  // Privileged requests initiated from a "non-webby" context will send
  // `Sec-Fetch-Site: None` while unprivileged ones will send
  // `Sec-Fetch-Site: cross-site`.
  if (factory_params.unsafe_non_webby_initiator) {
    cors::OriginAccessList::AccessState access_state =
        origin_access_list.CheckAccessState(initiator, request->url());
    bool is_privileged =
        (access_state == cors::OriginAccessList::AccessState::kAllowed);
    return is_privileged ? SecFetchSiteValue::kNoOrigin
                         : SecFetchSiteValue::kCrossSite;
  }

  // Other requests default to `kSameOrigin`, and walk through the request's URL
  // chain to calculate the correct value.
  auto header_value = SecFetchSiteValue::kSameOrigin;
  for (const GURL& target_url : request->url_chain()) {
    header_value = std::max(header_value, GetHeaderValueForTargetAndInitiator(
                                              target_url, initiator));
  }
  if (pending_redirect_url) {
    header_value = std::max(
        header_value,
        GetHeaderValueForTargetAndInitiator(*pending_redirect_url, initiator));
  }
  return header_value;
}

char const* GetSecFetchStorageAccessHeaderValue(
    net::cookie_util::StorageAccessStatus storage_access_status) {
  switch (storage_access_status) {
    case net::cookie_util::StorageAccessStatus::kInactive:
      return "inactive";
    case net::cookie_util::StorageAccessStatus::kActive:
      return "active";
    case net::cookie_util::StorageAccessStatus::kNone:
      return "none";
  }
  NOTREACHED();
}

net::cookie_util::SecFetchStorageAccessOutcome
ComputeSecFetchStorageAccessOutcome(const net::URLRequest& request,
                                    mojom::CredentialsMode credentials_mode) {
  if (!request.storage_access_status()) {
    return net::cookie_util::SecFetchStorageAccessOutcome::
        kOmittedStatusMissing;
  }
  if (credentials_mode != mojom::CredentialsMode::kInclude) {
    return net::cookie_util::SecFetchStorageAccessOutcome::
        kOmittedRequestOmitsCredentials;
  }
  switch (request.storage_access_status().value()) {
    case net::cookie_util::StorageAccessStatus::kInactive:
      return net::cookie_util::SecFetchStorageAccessOutcome::kValueInactive;
    case net::cookie_util::StorageAccessStatus::kActive:
      return net::cookie_util::SecFetchStorageAccessOutcome::kValueActive;
    case net::cookie_util::StorageAccessStatus::kNone:
      return net::cookie_util::SecFetchStorageAccessOutcome::kValueNone;
  }
  NOTREACHED();
}

// Sec-Fetch-Site
void SetSecFetchSiteHeader(net::URLRequest* request,
                           const GURL* pending_redirect_url,
                           const mojom::URLLoaderFactoryParams& factory_params,
                           const cors::OriginAccessList& origin_access_list) {
  SecFetchSiteValue header_value = GetHeaderValueForRequest(
      request, pending_redirect_url, factory_params, origin_access_list);

  request->SetExtraRequestHeaderByName(
      kSecFetchSite, GetSecFetchSiteHeaderString(header_value),
      /* overwrite = */ true);
}

// Sec-Fetch-Mode
void SetSecFetchModeHeader(net::URLRequest* request,
                           network::mojom::RequestMode mode) {
  std::string header_value = RequestModeToString(mode);

  request->SetExtraRequestHeaderByName(kSecFetchMode, header_value, false);
}

// Sec-Fetch-User
void SetSecFetchUserHeader(net::URLRequest* request, bool has_user_activation) {
  if (has_user_activation)
    request->SetExtraRequestHeaderByName(kSecFetchUser, "?1", true);
  else
    request->RemoveRequestHeaderByName(kSecFetchUser);
}

// Sec-Fetch-Dest
void SetSecFetchDestHeader(net::URLRequest* request,
                           network::mojom::RequestDestination dest) {
  // https://w3c.github.io/webappsec-fetch-metadata/#abstract-opdef-set-dest
  // If r's destination is the empty string, set header's value to the string
  // "empty". Otherwise, set header's value to r's destination.
  std::string header_value = RequestDestinationToString(
      dest, EmptyRequestDestinationOption::kUseFiveCharEmptyString);
  request->SetExtraRequestHeaderByName(kSecFetchDest, header_value, true);
}

// Sec-Fetch-Storage-Access
void SetSecFetchStorageAccessHeader(net::URLRequest& request,
                                    mojom::CredentialsMode credentials_mode) {
  base::UmaHistogramEnumeration(
      kSecFetchStorageAccessOutcomeHistogram,
      ComputeSecFetchStorageAccessOutcome(request, credentials_mode));

  if (credentials_mode != mojom::CredentialsMode::kInclude ||
      !request.storage_access_status()) {
    // A credentials mode of "same-origin" or "omit" prevents including cookies
    // on the request in the first place, so we don't bother to include the
    // `Sec-Fetch-Storage-Access` header in that case.
    //
    // To ensure that an erroneous value isn't sent by mistake (and that
    // consumers aren't allowed to override the correct "omitted" value), we
    // clear any existing value.
    request.RemoveRequestHeaderByName(kSecFetchStorageAccess);
    return;
  }
  request.SetExtraRequestHeaderByName(
      kSecFetchStorageAccess,
      GetSecFetchStorageAccessHeaderValue(
          request.storage_access_status().value()),
      /*overwrite=*/true);
}

}  // namespace

void SetFetchMetadataHeaders(
    net::URLRequest* request,
    network::mojom::RequestMode mode,
    bool has_user_activation,
    network::mojom::RequestDestination dest,
    const GURL* pending_redirect_url,
    const mojom::URLLoaderFactoryParams& factory_params,
    const cors::OriginAccessList& origin_access_list,
    const mojom::CredentialsMode credentials_mode) {
  DCHECK(request);
  DCHECK_NE(0u, request->url_chain().size());

  // Only append the header to potentially trustworthy URLs.
  const GURL& target_url =
      pending_redirect_url ? *pending_redirect_url : request->url();
  if (!IsUrlPotentiallyTrustworthy(target_url))
    return;

  SetSecFetchSiteHeader(request, pending_redirect_url, factory_params,
                        origin_access_list);
  SetSecFetchModeHeader(request, mode);
  SetSecFetchUserHeader(request, has_user_activation);
  SetSecFetchDestHeader(request, dest);
  SetSecFetchStorageAccessHeader(*request, credentials_mode);
}

void MaybeRemoveSecHeaders(net::URLRequest* request,
                           const GURL& pending_redirect_url) {
  DCHECK(request);

  // If our redirect destination is not trusted it would not have had sec-ch-
  // or sec-fetch- prefixed headers added to it. Our previous hops may have
  // added these headers if the current url is trustworthy though so we should
  // try to remove these now.
  if (IsUrlPotentiallyTrustworthy(request->url()) &&
      !IsUrlPotentiallyTrustworthy(pending_redirect_url)) {
    // Check each of our request headers and if it is a "sec-ch-" or
    // "sec-fetch-" prefixed header we'll remove it.
    const net::HttpRequestHeaders::HeaderVector request_headers =
        request->extra_request_headers().GetHeaderVector();
    for (const auto& header : request_headers) {
      if (StartsWith(header.key, "sec-ch-",
                     base::CompareCase::INSENSITIVE_ASCII) ||
          StartsWith(header.key, "sec-fetch-",
                     base::CompareCase::INSENSITIVE_ASCII)) {
        request->RemoveRequestHeaderByName(header.key);
      }
    }
  }
}

}  // namespace network

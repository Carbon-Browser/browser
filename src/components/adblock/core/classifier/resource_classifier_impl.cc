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

#include "components/adblock/core/classifier/resource_classifier_impl.h"

#include "base/strings/string_split.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/subscription/installed_subscription.h"

namespace adblock {
namespace {

absl::optional<HeaderFilterData> IsHeaderFilterOverruled(
    base::StringPiece blocking_header_filter,
    std::set<HeaderFilterData>& allowing_filters) {
  for (auto filter : allowing_filters) {
    if (filter.header_filter.empty()) {
      // Allowing header filters may not contain payload, allow all headers in
      // that case.
      return filter;
    }
    if (utils::RegexMatches(filter.header_filter, blocking_header_filter,
                            true)) {
      return filter;
    }
  }
  return absl::nullopt;
}

}  // namespace

ResourceClassifierImpl::~ResourceClassifierImpl() = default;

ResourceClassifier::ClassificationResult
ResourceClassifierImpl::ClassifyRequest(
    const SubscriptionCollection& subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) const {
  // Search all subscriptions for any blocking filters (generic or
  // domain-specific).
  const auto subscription_with_blocking_filter_it =
      subscription_collection.FindBySubresourceFilter(
          request_url, frame_hierarchy, content_type, sitekey,
          FilterCategory::Blocking);
  if (!subscription_with_blocking_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{ClassificationResult::Decision::Ignored, {}};
  }
  // Found a blocking filter but perhaps one of the subscriptions has an
  // allowing filter to override it?
  const auto subscription_with_allowing_filter_it =
      subscription_collection.FindByAllowFilter(request_url, frame_hierarchy,
                                                content_type, sitekey);
  if (subscription_with_allowing_filter_it) {
    // Found an overriding allowing filter:
    return ClassificationResult{ClassificationResult::Decision::Allowed,
                                *subscription_with_allowing_filter_it};
  }
  // Last chance to avoid blocking: maybe there is a GENERICBLOCK filter and we
  // should re-search for domain-specific filters only?
  if (subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Genericblock, request_url, frame_hierarchy,
          sitekey)) {
    // This is a relatively rare case - we should have searched for
    // domain-specific filters only.
    const auto subscription_with_domain_specific_blocking_filter_it =
        subscription_collection.FindBySubresourceFilter(
            request_url, frame_hierarchy, content_type, sitekey,
            FilterCategory::DomainSpecificBlocking);
    if (subscription_with_domain_specific_blocking_filter_it) {
      // There was a domain-specific blocking filter, the resource is blocked by
      // it.
      return ClassificationResult{
          ClassificationResult::Decision::Blocked,
          *subscription_with_domain_specific_blocking_filter_it};
    } else {
      // There were no domain-specific blocking filters, our first match must
      // have been a generic filter.
      return ClassificationResult{ClassificationResult::Decision::Ignored, {}};
    }
  }
  // There was no GENERICBLOCK filter available, so the original blocking result
  // is valid.
  return ClassificationResult{ClassificationResult::Decision::Blocked,
                              *subscription_with_blocking_filter_it};
}

ResourceClassifier::ClassificationResult ResourceClassifierImpl::ClassifyPopup(
    const SubscriptionCollection& subscription_collection,
    const GURL& popup_url,
    const GURL& opener_url,
    const SiteKey& sitekey) const {
  // Search all subscriptions for popup blocking filters (generic or
  // domain-specific).
  const auto subscription_with_blocking_filter_it =
      subscription_collection.FindByPopupFilter(popup_url, opener_url, sitekey,
                                                FilterCategory::Blocking);
  if (!subscription_with_blocking_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{ClassificationResult::Decision::Ignored, {}};
  }
  // Found a blocking filter but perhaps one of the subscriptions has an
  // allowing filter to override it?
  const auto subscription_with_allowing_filter_it =
      subscription_collection.FindByPopupFilter(popup_url, opener_url, sitekey,
                                                FilterCategory::Allowing);
  if (subscription_with_allowing_filter_it) {
    // Found an overriding allowing filter:
    return ClassificationResult{ClassificationResult::Decision::Allowed,
                                *subscription_with_allowing_filter_it};
  }
  // There is no overriding allowing filter, the popup should be blocked.
  return ClassificationResult{ClassificationResult::Decision::Blocked,
                              *subscription_with_blocking_filter_it};
}

ResourceClassifier::ClassificationResult
ResourceClassifierImpl::ClassifyResponse(
    const SubscriptionCollection& subscription_collection,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const scoped_refptr<net::HttpResponseHeaders>& response_headers) const {
  auto blocking_filters = subscription_collection.GetHeaderFilters(
      request_url, frame_hierarchy, content_type, FilterCategory::Blocking);
  if (blocking_filters.empty()) {
    return ClassificationResult{ClassificationResult::Decision::Ignored, {}};
  }

  // If blocking filters found, check first if filters are not overuled
  const auto subscription_with_allowing_document_filter_it =
      subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Document, request_url, frame_hierarchy, SiteKey());
  if (subscription_with_allowing_document_filter_it) {
    // Found no blocking filters in any of the subscriptions.
    return ClassificationResult{ClassificationResult::Decision::Allowed,
                                *subscription_with_allowing_document_filter_it};
  }

  if (subscription_collection.FindBySpecialFilter(
          SpecialFilterType::Genericblock, request_url, frame_hierarchy,
          SiteKey())) {
    // If genericblock filter found, searched for blocking domain-specific
    // filters.
    blocking_filters = subscription_collection.GetHeaderFilters(
        request_url, frame_hierarchy, content_type,
        FilterCategory::DomainSpecificBlocking);

    return CheckHeaderFiltersMatchResponseHeaders(
        request_url, frame_hierarchy, response_headers,
        std::move(blocking_filters), {});
  }
  // If no special filters found, get allowing filters and check which filters
  // applies.
  auto allowing_filters = subscription_collection.GetHeaderFilters(
      request_url, frame_hierarchy, content_type, FilterCategory::Allowing);
  return CheckHeaderFiltersMatchResponseHeaders(
      request_url, frame_hierarchy, response_headers,
      std::move(blocking_filters), std::move(allowing_filters));
}

ResourceClassifier::ClassificationResult
ResourceClassifierImpl::CheckHeaderFiltersMatchResponseHeaders(
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    std::set<HeaderFilterData> blocking_filters,
    std::set<HeaderFilterData> allowing_filters) const {
  ClassificationResult result{ClassificationResult::Decision::Ignored, {}};

  for (const auto& filter : blocking_filters) {
    auto key_value =
        base::SplitString(filter.header_filter, "=", base::KEEP_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);
    // If no '=' occurs, filter blocks response contains header, regardless
    // header value
    if (key_value.size() == 1u) {
      if (headers->HasHeader(filter.header_filter)) {
        if (auto allow_rule = IsHeaderFilterOverruled(filter.header_filter,
                                                      allowing_filters)) {
          result = {ClassificationResult::Decision::Allowed,
                    allow_rule->subscription_url};
        } else {
          return ClassificationResult{ClassificationResult::Decision::Blocked,
                                      filter.subscription_url};
        }
      }
    } else {
      DCHECK(key_value.size() == 2u);
      if (headers->HasHeaderValue(key_value[0], key_value[1])) {
        if (auto allow_rule = IsHeaderFilterOverruled(filter.header_filter,
                                                      allowing_filters)) {
          result = {ClassificationResult::Decision::Allowed,
                    allow_rule->subscription_url};
        } else {
          return ClassificationResult{ClassificationResult::Decision::Blocked,
                                      filter.subscription_url};
        }
      }
    }
  }
  return result;
}

}  // namespace adblock

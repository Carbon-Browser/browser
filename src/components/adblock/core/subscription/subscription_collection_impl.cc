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

#include "components/adblock/core/subscription/subscription_collection_impl.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include "base/json/json_string_value_serializer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "components/adblock/core/common/adblock_utils.h"

namespace adblock {
namespace {

std::string DocumentDomain(const GURL& request_url,
                           const std::vector<GURL>& frame_hierarchy) {
  return frame_hierarchy.empty() ? request_url.host()
                                 : frame_hierarchy[0].host();
}

std::vector<base::StringPiece> ReduceSelectors(
    InstalledSubscription::Selectors& combined_selectors) {
  // Populate result with blocking selectors.
  std::vector<base::StringPiece> final_selectors =
      std::move(combined_selectors.elemhide_selectors);
  // Remove exceptions.
  final_selectors.erase(
      std::remove_if(
          final_selectors.begin(), final_selectors.end(),
          [&](const auto& selector) {
            return std::find(combined_selectors.elemhide_exceptions.begin(),
                             combined_selectors.elemhide_exceptions.end(),
                             selector) !=
                   combined_selectors.elemhide_exceptions.end();
          }),
      final_selectors.end());
  return final_selectors;
}

using GenericGetter = absl::optional<base::StringPiece> (
    InstalledSubscription::*)(const GURL&,
                              const std::string&,
                              FilterCategory) const;

bool IsFilterOverruled(base::StringPiece blocking,
                       const InstalledSubscription& subscription,
                       const GURL& request_url,
                       const std::string& request_domain,
                       GenericGetter getter) {
  auto allowing = (subscription.*getter)(request_url, request_domain,
                                         FilterCategory::Allowing);
  if (!allowing)
    return false;
  return allowing->empty() || *allowing == blocking;
}

bool GenericHasAllowingFilter(const InstalledSubscription& subscription,
                              const base::StringPiece blocking,
                              const GURL& request_url,
                              const std::vector<GURL>& frame_hierarchy,
                              GenericGetter getter) {
  const auto request_domain = DocumentDomain(request_url, frame_hierarchy);
  // There may exist an allowing rule for this request and its immediate
  // parent frame. We also check for document-wide allowing filters.
  if (IsFilterOverruled(blocking, subscription, request_url, request_domain,
                        getter) ||
      subscription.HasSpecialFilter(SpecialFilterType::Document, request_url,
                                    request_domain, SiteKey())) {
    return true;
  }
  // For parent frames, we only match document-wide allowing filters. Parent
  // frames' allowing rules don't propagate to child frames.
  for (auto it = frame_hierarchy.begin(); it < frame_hierarchy.end(); ++it) {
    const GURL& current_url = *it;
    const std::string& current_domain = std::next(it) != frame_hierarchy.end()
                                            ? std::next(it)->host()
                                            : current_url.host();
    if (subscription.HasSpecialFilter(SpecialFilterType::Document, current_url,
                                      current_domain, SiteKey())) {
      return true;
    }
  }
  return false;
}

bool SubscriptionContainsSpecialFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    SpecialFilterType filter_type,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) {
  for (auto it = frame_hierarchy.begin(); it < frame_hierarchy.end(); ++it) {
    const GURL& current_url = *it;
    const std::string& current_domain = std::next(it) != frame_hierarchy.end()
                                            ? std::next(it)->host()
                                            : current_url.host();
    if (subscription->HasSpecialFilter(filter_type, current_url, current_domain,
                                       sitekey)) {
      return true;
    }
  }
  return false;
}

bool HasAllowFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) {
  if (subscription->HasUrlFilter(
          request_url, DocumentDomain(request_url, frame_hierarchy),
          content_type, sitekey, FilterCategory::Allowing)) {
    return true;
  }
  if (SubscriptionContainsSpecialFilter(subscription,
                                        SpecialFilterType::Document,
                                        frame_hierarchy, sitekey)) {
    return true;
  }
  return false;
}

bool HasSpecialFilter(
    const scoped_refptr<adblock::InstalledSubscription> subscription,
    SpecialFilterType filter_type,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) {
  if (subscription->HasSpecialFilter(
          filter_type, request_url,
          DocumentDomain(request_url, frame_hierarchy), sitekey)) {
    return true;
  }
  if (SubscriptionContainsSpecialFilter(subscription, filter_type,
                                        frame_hierarchy, sitekey)) {
    return true;
  }
  return false;
}

absl::optional<base::StringPiece> GenericFindFilter(
    const std::vector<scoped_refptr<InstalledSubscription>>& subscriptions,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    GenericGetter getter) {
  for (const auto& subscription : subscriptions) {
    // Is there a blocking filter in this subscription?
    const auto blocking = (*subscription.*getter)(
        request_url, DocumentDomain(request_url, frame_hierarchy),
        FilterCategory::Blocking);
    if (!blocking) {
      // There are no blocking filters in this subscription, check next one.
      continue;
    }
    // There may be an allowing rule for the entire document or an allowing //
    // filter in one of the subscriptions:
    if (base::ranges::any_of(subscriptions, [&](const auto& sub) {
          return GenericHasAllowingFilter(*sub, *blocking, request_url,
                                          frame_hierarchy, getter);
        })) {
      continue;
    }
    // Last chance to avoid blocking: maybe there is a Genericblock filter and
    // we should re-search for domain-specific filters only?
    if (base::ranges::any_of(subscriptions, [&](const auto& sub) {
          return HasSpecialFilter(sub, SpecialFilterType::Genericblock,
                                  request_url, frame_hierarchy, SiteKey());
        })) {
      // This is a relatively rare case - we should have searched for
      // domain-specific filters only.
      const auto domain_specific_blocking = (*subscription.*getter)(
          request_url, DocumentDomain(request_url, frame_hierarchy),
          FilterCategory::DomainSpecificBlocking);
      if (domain_specific_blocking) {
        // There is a domain-specific blocking filter. No point in
        // searching for a domain-specific allowing filter, since the
        // previous search for non-specific allowing filters would have found
        // it.
        return domain_specific_blocking;
      } else {
        // There are no domain-specific blocking filters in this
        // subscription.
        continue;
      }
    }
    return blocking;
  }
  return absl::nullopt;
}

}  // namespace

SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    std::vector<scoped_refptr<InstalledSubscription>> current_state)
    : subscriptions_(std::move(current_state)) {}
SubscriptionCollectionImpl::~SubscriptionCollectionImpl() = default;
SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    const SubscriptionCollectionImpl&) = default;
SubscriptionCollectionImpl::SubscriptionCollectionImpl(
    SubscriptionCollectionImpl&&) = default;
SubscriptionCollectionImpl& SubscriptionCollectionImpl::operator=(
    const SubscriptionCollectionImpl&) = default;
SubscriptionCollectionImpl& SubscriptionCollectionImpl::operator=(
    SubscriptionCollectionImpl&&) = default;

absl::optional<GURL> SubscriptionCollectionImpl::FindBySubresourceFilter(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey,
    FilterCategory category) const {
  const auto subscription = std::find_if(
      subscriptions_.begin(), subscriptions_.end(),
      [&](const auto& subscription) {
        return subscription->HasUrlFilter(
            request_url, DocumentDomain(request_url, frame_hierarchy),
            content_type, sitekey, category);
      });
  if (subscription != subscriptions_.end()) {
    return (*subscription)->GetSourceUrl();
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindByPopupFilter(
    const GURL& popup_url,
    const GURL& opener_url,
    const SiteKey& sitekey,
    FilterCategory category) const {
  const auto subscription =
      std::find_if(subscriptions_.begin(), subscriptions_.end(),
                   [&](const auto& subscription) {
                     return subscription->HasPopupFilter(popup_url, opener_url,
                                                         sitekey, category);
                   });
  if (subscription != subscriptions_.end()) {
    return (*subscription)->GetSourceUrl();
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindByAllowFilter(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    const SiteKey& sitekey) const {
  for (const auto& subscription : subscriptions_) {
    if (HasAllowFilter(subscription, request_url, frame_hierarchy, content_type,
                       sitekey)) {
      return (*subscription).GetSourceUrl();
    }
  }
  return absl::nullopt;
}

absl::optional<GURL> SubscriptionCollectionImpl::FindBySpecialFilter(
    SpecialFilterType filter_type,
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) const {
  for (const auto& subscription : subscriptions_) {
    if (HasSpecialFilter(subscription, filter_type, request_url,
                         frame_hierarchy, sitekey)) {
      return (*subscription).GetSourceUrl();
    }
  }
  return absl::nullopt;
}

std::vector<base::StringPiece>
SubscriptionCollectionImpl::GetElementHideSelectors(
    const GURL& frame_url,
    const std::vector<GURL>& frame_hierarchy,
    const SiteKey& sitekey) const {
  const bool domain_specific = !!FindBySpecialFilter(
      SpecialFilterType::Generichide, frame_url, frame_hierarchy, sitekey);

  InstalledSubscription::Selectors combined_selectors;
  for (const auto& subscription : subscriptions_) {
    auto selectors =
        subscription->GetElemhideSelectors(frame_url, domain_specific);
    std::move(selectors.elemhide_selectors.begin(),
              selectors.elemhide_selectors.end(),
              std::back_inserter(combined_selectors.elemhide_selectors));
    std::move(selectors.elemhide_exceptions.begin(),
              selectors.elemhide_exceptions.end(),
              std::back_inserter(combined_selectors.elemhide_exceptions));
  }
  return ReduceSelectors(combined_selectors);
}

std::vector<base::StringPiece>
SubscriptionCollectionImpl::GetElementHideEmulationSelectors(
    const GURL& frame_url) const {
  InstalledSubscription::Selectors combined_selectors;
  for (const auto& subscription : subscriptions_) {
    auto selectors = subscription->GetElemhideEmulationSelectors(frame_url);
    std::move(selectors.elemhide_selectors.begin(),
              selectors.elemhide_selectors.end(),
              std::back_inserter(combined_selectors.elemhide_selectors));
    std::move(selectors.elemhide_exceptions.begin(),
              selectors.elemhide_exceptions.end(),
              std::back_inserter(combined_selectors.elemhide_exceptions));
  }
  return ReduceSelectors(combined_selectors);
}

std::string SubscriptionCollectionImpl::GenerateSnippetsJson(
    const GURL& frame_url,
    const std::vector<GURL>& frame_hierarchy) const {
  std::vector<base::Value> snippets;
  auto document_domain = DocumentDomain(frame_url, frame_hierarchy);

  for (const auto& subscription : subscriptions_) {
    auto matched = subscription->MatchSnippets(document_domain);
    for (const auto& snippet : matched) {
      std::vector<base::Value> call;
      call.push_back(base::Value(snippet.command));
      for (const auto& arg : snippet.arguments)
        call.push_back(base::Value(arg));
      snippets.push_back(base::Value(std::move(call)));
    }
  }

  if (snippets.size() == 0)
    return "";

  std::string serialized;
  JSONStringValueSerializer serializer(&serialized);
  serializer.Serialize(base::Value(std::move(snippets)));

  return serialized;
}

base::StringPiece SubscriptionCollectionImpl::GetCspInjection(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) const {
  auto result = GenericFindFilter(subscriptions_, request_url, frame_hierarchy,
                                  &InstalledSubscription::FindCspFilter);
  return result ? *result : "";
}

absl::optional<GURL> SubscriptionCollectionImpl::GetRewriteUrl(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy) const {
  auto result = GenericFindFilter(subscriptions_, request_url, frame_hierarchy,
                                  &InstalledSubscription::FindRewriteFilter);
  return result ? absl::optional<GURL>(GURL(*result)) : absl::nullopt;
}

std::set<HeaderFilterData> SubscriptionCollectionImpl::GetHeaderFilters(
    const GURL& request_url,
    const std::vector<GURL>& frame_hierarchy,
    ContentType content_type,
    FilterCategory category) const {
  std::set<HeaderFilterData> filters{};
  for (const auto& subscription : subscriptions_) {
    subscription->FindHeaderFilters(
        request_url, content_type, DocumentDomain(request_url, frame_hierarchy),
        category, filters);
  }
  return filters;
}

}  // namespace adblock

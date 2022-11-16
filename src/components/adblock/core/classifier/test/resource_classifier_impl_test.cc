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
#include "absl/types/optional.h"
#include "base/strings/string_piece_forward.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using testing::_;
using testing::Return;

using Decision = ResourceClassifier::ClassificationResult::Decision;

class AdblockResourceClassifierImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    classifier_ = base::MakeRefCounted<ResourceClassifierImpl>();
  }

  void FindBySubresourceFilterReturns(const absl::optional<GURL>& return_value,
                                      FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                FindBySubresourceFilter(_, _, _, _, category))
        .WillOnce(Return(return_value));
  }

  void FindBySpecialFilterReturns(const absl::optional<GURL>& return_value,
                                  SpecialFilterType type) {
    EXPECT_CALL(mock_subscription_collection_,
                FindBySpecialFilter(type, _, _, _))
        .WillOnce(Return(return_value));
  }

  void FindByAllowFilterReturns(const absl::optional<GURL>& return_value) {
    EXPECT_CALL(mock_subscription_collection_, FindByAllowFilter(_, _, _, _))
        .WillOnce(Return(return_value));
  }

  void FindByPopupFilterReturns(const absl::optional<GURL>& return_value,
                                FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                FindByPopupFilter(_, _, _, category))
        .WillOnce(Return(return_value));
  }

  void GetHeaderFiltersReturns(const std::set<HeaderFilterData>& return_value,
                               FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                GetHeaderFilters(_, _, _, category))
        .WillOnce(Return(return_value));
  }

  void AssertFindBySubresourceFilterNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                FindBySubresourceFilter(_, _, _, _, category))
        .Times(0);
  }

  void AssertFindBySpecialFilterNotCalled(SpecialFilterType type) {
    EXPECT_CALL(mock_subscription_collection_,
                FindBySpecialFilter(type, _, _, _))
        .Times(0);
  }

  void AssertFindByAllowFilterNotCalled() {
    EXPECT_CALL(mock_subscription_collection_, FindByAllowFilter(_, _, _, _))
        .Times(0);
  }

  void AssertFindByPopupFilterNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                FindByPopupFilter(_, _, _, category))
        .Times(0);
  }

  void AssertGetHeaderFiltersNotCalled(FilterCategory category) {
    EXPECT_CALL(mock_subscription_collection_,
                GetHeaderFilters(_, _, _, category))
        .Times(0);
  }

  scoped_refptr<net::HttpResponseHeaders> CreateResponseHeaders(
      base::StringPiece header) {
    std::string full_headers = "HTTP/1.1 200 OK\n";
    full_headers.append(header.begin()).append(":");
    auto headers = net::HttpResponseHeaders::TryToCreate(full_headers);
    CHECK(headers);
    return headers;
  }
  const GURL kResourceAddress{"https://address.com/image.png"};
  const GURL kParentAddress{"https://parent.com/"};
  const ContentType kContentType = ContentType::Image;
  const SiteKey kSitekey{"abc"};
  const GURL kSourceUrl{"https://subscription.com/easylist.txt"};
  const std::set<HeaderFilterData> kAllowingHeaderFilters = {
      {"allowing_filter", kSourceUrl}};
  const std::set<HeaderFilterData> kBlockingHeaderFilters = {
      {"blocking_filter", kSourceUrl}};
  const std::set<HeaderFilterData> kDomainSpecificHeaderFilters = {
      {"domain_specific_filter", kSourceUrl}};
  MockSubscriptionCollection mock_subscription_collection_;
  scoped_refptr<ResourceClassifier> classifier_;
};

TEST_F(AdblockResourceClassifierImplTest, BlockingFilterNotFound) {
  // Subscriptions get queried for url filters, no blocking filter is found.
  FindBySubresourceFilterReturns(absl::nullopt, FilterCategory::Blocking);

  // Since there's no blocking filter, all search stops now.
  AssertFindByAllowFilterNotCalled();
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest, BlockingFilterFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Subscriptions have no allowing filter, so the last chance of not
  // blocking the request is a generic block filter (separate test).
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest, BlockingAndAllowingFilterFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::optional<GURL>(kSourceUrl));

  // Allowing filter has been found, make other searach stops now.
  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Allowed);
}

TEST_F(AdblockResourceClassifierImplTest, GenericBlockSearch) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found and genericblock is also not found.
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertFindBySubresourceFilterNotCalled(
      FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest, DomainSpecificNotFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found but gebricblock exists.
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);

  // However there is no domain specific filter.
  FindBySubresourceFilterReturns(absl::nullopt,
                                 FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest, DomainSpecificFound) {
  // Subscriptions get queried for url filters, one reports a blocking filter
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::Blocking);

  // Since there's a blocking filter, subscriptions are queried for allowing
  // filters
  FindByAllowFilterReturns(absl::nullopt);

  // Allowing filter has not been found but gebricblock exists.
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);

  // And there is a domain specific filter.
  FindBySubresourceFilterReturns(absl::optional<GURL>(kSourceUrl),
                                 FilterCategory::DomainSpecificBlocking);

  EXPECT_EQ(
      classifier_
          ->ClassifyRequest(mock_subscription_collection_, kResourceAddress,
                            {kParentAddress}, kContentType, kSitekey)
          .decision,
      Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest, PopupNoHasFilters) {
  FindByPopupFilterReturns(absl::nullopt, FilterCategory::Blocking);
  AssertFindByPopupFilterNotCalled(FilterCategory::Allowing);

  EXPECT_EQ(classifier_
                ->ClassifyPopup(mock_subscription_collection_, kResourceAddress,
                                {kParentAddress}, kSitekey)
                .decision,
            Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest, PopupHasBlockingFilter) {
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Blocking);
  FindByPopupFilterReturns(absl::nullopt, FilterCategory::Allowing);

  EXPECT_EQ(classifier_
                ->ClassifyPopup(mock_subscription_collection_, kResourceAddress,
                                {kParentAddress}, kSitekey)
                .decision,
            Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest, PopupHasAllowingFilter) {
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Blocking);
  FindByPopupFilterReturns(absl::optional<GURL>(kSourceUrl),
                           FilterCategory::Allowing);

  EXPECT_EQ(classifier_
                ->ClassifyPopup(mock_subscription_collection_, kResourceAddress,
                                {kParentAddress}, kSitekey)
                .decision,
            Decision::Allowed);
}

TEST_F(AdblockResourceClassifierImplTest, NoBlockingHeaderFilters) {
  GetHeaderFiltersReturns({}, FilterCategory::Blocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, {})
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithDocumentAllowingFilter) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Document);

  AssertFindBySpecialFilterNotCalled(SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, {})
          .decision,
      Decision::Allowed);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockButNoDomainSpecific) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns({}, FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, {})
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockAndDomainSpecificNoMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns(kDomainSpecificHeaderFilters,
                          FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto headers = CreateResponseHeaders("not_matching");
  CHECK(headers);
  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, headers)
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersWithGenericblockAndDomainSpecificMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::optional<GURL>(kSourceUrl),
                             SpecialFilterType::Genericblock);
  GetHeaderFiltersReturns(kDomainSpecificHeaderFilters,
                          FilterCategory::DomainSpecificBlocking);
  AssertGetHeaderFiltersNotCalled(FilterCategory::Allowing);

  auto headers = CreateResponseHeaders(
      kDomainSpecificHeaderFilters.begin()->header_filter);
  CHECK(headers);
  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, headers)
          .decision,
      Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest, BlockingHeaderFiltersNoMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers = CreateResponseHeaders("no_match");
  CHECK(headers);
  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, headers)
          .decision,
      Decision::Ignored);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersMatchNoAllowingFilterMatch) {
  GetHeaderFiltersReturns(kBlockingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers =
      CreateResponseHeaders(kBlockingHeaderFilters.begin()->header_filter);
  CHECK(headers);
  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, headers)
          .decision,
      Decision::Blocked);
}

TEST_F(AdblockResourceClassifierImplTest,
       BlockingHeaderFiltersMatchWithAllowingFilterMatch) {
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Blocking);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Document);
  FindBySpecialFilterReturns(absl::nullopt, SpecialFilterType::Genericblock);
  AssertGetHeaderFiltersNotCalled(FilterCategory::DomainSpecificBlocking);
  GetHeaderFiltersReturns(kAllowingHeaderFilters, FilterCategory::Allowing);

  auto headers =
      CreateResponseHeaders(kAllowingHeaderFilters.begin()->header_filter);
  CHECK(headers);
  EXPECT_EQ(
      classifier_
          ->ClassifyResponse(mock_subscription_collection_, kResourceAddress,
                             {kParentAddress}, kContentType, headers)
          .decision,
      Decision::Allowed);
}
}  // namespace adblock

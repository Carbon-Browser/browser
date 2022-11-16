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

#include <vector>

#include "absl/types/optional.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_piece_forward.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/test/mock_installed_subscription.h"
#include "gmock/gmock-actions.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using testing::_;
using testing::Return;

class AdblockSubscriptionCollectionImplTest : public ::testing::Test {
 public:
  const GURL kImageAddress{"https://address.com/image.png"};
  const GURL kParentAddress{"https://parent.com/"};
  const SiteKey kSitekey{"abc"};
  const GURL kSourceUrl{"https://subscription.com/easylist.txt"};
};

TEST_F(AdblockSubscriptionCollectionImplTest,
       AllowingFilterFoundForFrameHierarchy) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  const std::vector<GURL> frame_hierarchy{
      GURL("https://frame.com/frame1.html"),
      GURL("https://frame.com/frame2.html"),
      GURL("https://frame.com/"),
  };

  // Subscription has a blocking filter.
  EXPECT_CALL(*sub1, HasUrlFilter(kImageAddress, frame_hierarchy[0].host(),
                                  ContentType::Image, kSitekey,
                                  FilterCategory::Blocking))
      .WillOnce(Return(true));

  // The entire |frame_hierarchy| is queried to see if there's an allowing
  // filter present for any step in the chain. The final one reports a match.
  EXPECT_CALL(*sub1, HasUrlFilter(kImageAddress, frame_hierarchy[0].host(),
                                  ContentType::Image, kSitekey,
                                  FilterCategory::Allowing))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[0],
                               frame_hierarchy[1].host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[1],
                               frame_hierarchy[2].host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, frame_hierarchy[2],
                               frame_hierarchy[2].host(), kSitekey))
      .WillOnce(Return(true));

  // The resource is allowlisted
  EXPECT_CALL(*sub1, GetSourceUrl()).WillRepeatedly(Return(kSourceUrl));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_TRUE(!!collection.FindBySubresourceFilter(
      kImageAddress, frame_hierarchy, ContentType::Image, kSitekey,
      FilterCategory::Blocking));
  EXPECT_TRUE(!!collection.FindByAllowFilter(kImageAddress, frame_hierarchy,
                                             ContentType::Image, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       ElemhideSelectorsCombinedFromSubscriptions) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First, we establish whether to search for domain-specific selectors only by
  // querying for Generichide filters. One subscription does have a Generichide
  // filter.

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Generichide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Generichide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));
  const bool domain_specific = true;

  // Now all subscriptions are queried for selectors.
  InstalledSubscription::Selectors sub1_selectors;
  sub1_selectors.elemhide_selectors = {"div", "ad_frame", "billboard"};
  sub1_selectors.elemhide_exceptions = {"billboard"};
  EXPECT_CALL(*sub1, GetElemhideSelectors(kParentAddress, domain_specific))
      .WillOnce(Return(sub1_selectors));

  InstalledSubscription::Selectors sub2_selectors;
  sub2_selectors.elemhide_selectors = {"header", "ad_content", "billboard"};
  sub2_selectors.elemhide_exceptions = {"ad_frame"};
  EXPECT_CALL(*sub2, GetElemhideSelectors(kParentAddress, domain_specific))
      .WillOnce(Return(sub2_selectors));

  // sub1's "billboard" exception cancels out the "billboard" selectors from
  // both subscriptions. sub2's "ad_frame" exception cancels out the "ad_frame"
  // selector from sub1.
  std::vector<base::StringPiece> expected_selectors{"div", "header",
                                                    "ad_content"};

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  auto actual_selectors =
      collection.GetElementHideSelectors(kParentAddress, {}, kSitekey);
  std::sort(actual_selectors.begin(), actual_selectors.end());
  std::sort(expected_selectors.begin(), expected_selectors.end());
  EXPECT_EQ(actual_selectors, expected_selectors);
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       ElemhideEmulationSelectorsCombinedFromSubscriptions) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // This works very similarly to Elemhide selectors, only all Elemhide
  // Emulation selectors are by design domain-specific, so we don't search for
  // Generichide allow filters.

  // Now all subscriptions are queried for selectors.
  InstalledSubscription::Selectors sub1_selectors;
  sub1_selectors.elemhide_selectors = {"a", "b", "c"};
  sub1_selectors.elemhide_exceptions = {"c"};
  EXPECT_CALL(*sub1, GetElemhideEmulationSelectors(kParentAddress))
      .WillOnce(Return(sub1_selectors));

  InstalledSubscription::Selectors sub2_selectors;
  sub2_selectors.elemhide_selectors = {"d", "b", "e"};
  sub2_selectors.elemhide_exceptions = {"b"};
  EXPECT_CALL(*sub2, GetElemhideEmulationSelectors(kParentAddress))
      .WillOnce(Return(sub2_selectors));

  std::vector<base::StringPiece> expected_selectors{"a", "d", "e"};

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  auto actual_selectors =
      collection.GetElementHideEmulationSelectors(kParentAddress);
  std::sort(actual_selectors.begin(), actual_selectors.end());
  std::sort(expected_selectors.begin(), expected_selectors.end());
  EXPECT_EQ(actual_selectors, expected_selectors);
}

TEST_F(AdblockSubscriptionCollectionImplTest, GenerateSnippetsJson) {
  auto subscription = base::MakeRefCounted<MockInstalledSubscription>();

  InstalledSubscription::Snippet snippet;
  snippet.command = "say";
  snippet.arguments = {"Hello"};
  EXPECT_CALL(*subscription, MatchSnippets("parent.com"))
      .WillOnce(Return(std::vector<InstalledSubscription::Snippet>{snippet}));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{subscription});
  auto json = collection.GenerateSnippetsJson(kParentAddress, {});
  EXPECT_EQ("[[\"say\",\"Hello\"]]", json);
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasAllowingDocumentFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_TRUE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Document, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, NoneHasAllowingDocumentFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Document, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_FALSE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Document, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasAllowingElementHideFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_TRUE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Elemhide, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       NoneHasAllowingElementHideFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2, HasSpecialFilter(SpecialFilterType::Elemhide, kSourceUrl,
                                      kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Elemhide, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_FALSE(!!collection.FindBySpecialFilter(
      SpecialFilterType::Elemhide, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, OneHasGenericblockFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(true));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_TRUE(collection.FindBySpecialFilter(
      SpecialFilterType::Genericblock, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, NoneHasGenericblockFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kSourceUrl,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), kSitekey))
      .WillOnce(Return(false));

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_FALSE(collection.FindBySpecialFilter(
      SpecialFilterType::Genericblock, kSourceUrl, {kParentAddress}, kSitekey));
}

TEST_F(AdblockSubscriptionCollectionImplTest, CspBlockingFilterNotFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // There are no blocking filters found.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));

  // Since there are no blocking CSP filters, no need to check allow filters.
  EXPECT_CALL(*sub1, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub2, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub1, FindCspFilter(_, _, FilterCategory::Allowing)).Times(0);
  EXPECT_CALL(*sub2, FindCspFilter(_, _, FilterCategory::Allowing)).Times(0);

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});

  // Empty result means no CSP injection necessary.
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}), "");
}

TEST_F(AdblockSubscriptionCollectionImplTest, CspBlockingFilterFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter. Second is not queried
  // because we only return the first found blocking filter (may change in
  // DPD-1145).
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("block"));
  EXPECT_CALL(*sub2, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .Times(0);

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Neither subscription contains a document-wide allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Neither subscription contains an allowing CSP rule.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // Neither subscription contains a GenericBlock rule that would require
  // searching for domain-specific-only blocking CSP filters.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // In presence of a blocking CSP filters and absence of any allowing filters,
  // the string returned by first subscription becomes the CSP injection.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}),
            "block");
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaDocumentAllowingRule) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("script-src 'none'"));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // A document-wide allowing rule exists.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing Document filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}), "");
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaMatchingAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("script-src 'none'"));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Document-wide allowing filters may or may not be consulted, but if they
  // are, there are no matches.
  ON_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, _, _, SiteKey()))
      .WillByDefault(Return(false));

  // An allowing CSP rule, with specific payload, is found for parent.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return("script-src 'none'"));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}), "");
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterOverruledViaGenericAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("script-src 'none'"));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Document-wide allowing filters may or may not be consulted, but if they
  // are, there are no matches.
  ON_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Document, _, _, SiteKey()))
      .WillByDefault(Return(false));

  // An allowing CSP rule, with no payload, is found for request. A CSP allowing
  // filter with no payload overrules any blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(""));

  // No need to query GenericBlock rules since the blocking CSP filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing CSP filter overrules the blocking CSP filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}), "");
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       CspBlockingFilterNotOverruledViaMismatchedAllowingCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("script-src 'none'"));

  // No document-wide allowing filters present.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Allowing CSP rules with different payloads do NOT match the blocking
  // CSP filter.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return("script-src *"));

  // The blocking filter is not overruled yet, will now check generic block
  // rules.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // The blocking CSP filter was NOT overruled by an allowing CSP filter because
  // the payloads did not match.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}),
            "script-src 'none'");
}

TEST_F(AdblockSubscriptionCollectionImplTest, GenericBlockCspFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking CSP filter. Second does not.
  // Both are consulted, because the first subscription's blocking CSP filter
  // gets overruled as not domain-specific.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return("block"));
  EXPECT_CALL(*sub2, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));

  // Neither subscription contains a document-wide allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Neither subscription contains an allowing CSP rule.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // Second subscription contains a genericblock rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // Search is retried but now for domain-specific CSP filters only. No matches.
  EXPECT_CALL(*sub1, FindCspFilter(kImageAddress, kParentAddress.host(),
                                   FilterCategory::DomainSpecificBlocking))
      .WillOnce(Return(absl::nullopt));

  // Finally, no CSP filter found.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_EQ(collection.GetCspInjection(kImageAddress, {kParentAddress}), "");
}

TEST_F(AdblockSubscriptionCollectionImplTest, RewriteBlockingFilterNotFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // There are no blocking filters found.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));

  // Since there are no blocking filters, no need to check allow filters.
  EXPECT_CALL(*sub1, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub2, HasSpecialFilter(_, _, _, _)).Times(0);
  EXPECT_CALL(*sub1, FindRewriteFilter(_, _, FilterCategory::Allowing))
      .Times(0);
  EXPECT_CALL(*sub2, FindRewriteFilter(_, _, FilterCategory::Allowing))
      .Times(0);

  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});

  // Empty result means no redirect necessary.
  EXPECT_EQ(collection.GetRewriteUrl(kImageAddress, {kParentAddress}),
            absl::nullopt);
}

TEST_F(AdblockSubscriptionCollectionImplTest, RewriteBlockingFilterFound) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();
  constexpr const char* redirect = "about::blank";

  // First subscription contains a blocking filter. Second is not queried
  // because we only return the first found blocking filter.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return(redirect));
  EXPECT_CALL(*sub2, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .Times(0);

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.

  // Neither subscription contains a document-wide allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Neither subscription contains an allowing rule.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // Neither subscription contains a GenericBlock rule that would require
  // searching for domain-specific-only blocking filters.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // In presence of a blocking filters and absence of any allowing filters,
  // the string returned by first subscription becomes the redirect.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_EQ(collection.GetRewriteUrl(kImageAddress, {kParentAddress}),
            GURL(redirect));
}

TEST_F(AdblockSubscriptionCollectionImplTest,
       RewriteBlockingFilterOverruledViaDocumentAllowingRule) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();

  // Subscription contains a blocking filter.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return("block"));

  // Since a blocking filter is found, implementation will try to find allowing
  // rules.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // A document-wide allowing rule exists.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // No need to query GenericBlock rules since the blocking filter was
  // overruled already.
  EXPECT_CALL(*sub1, HasSpecialFilter(SpecialFilterType::Genericblock, _, _, _))
      .Times(0);

  // The allowing Document filter overrules the blocking filter.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1});
  EXPECT_EQ(collection.GetRewriteUrl(kImageAddress, {kParentAddress}),
            absl::nullopt);
}

TEST_F(AdblockSubscriptionCollectionImplTest, GenericBlockRewriteFilter) {
  auto sub1 = base::MakeRefCounted<MockInstalledSubscription>();
  auto sub2 = base::MakeRefCounted<MockInstalledSubscription>();

  // First subscription contains a blocking filter. Second does not.
  // Both are consulted, because the first subscription's blocking filter
  // gets overruled as not domain-specific.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return("block"));
  EXPECT_CALL(*sub2, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Blocking))
      .WillOnce(Return(absl::nullopt));

  // Neither subscription contains a document-wide allowing rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Document, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));

  // Neither subscription contains an allowing rule.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));
  EXPECT_CALL(*sub2, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::Allowing))
      .WillOnce(Return(absl::nullopt));

  // Second subscription contains a genericblock rule.
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub1,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kImageAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(false));
  EXPECT_CALL(*sub2,
              HasSpecialFilter(SpecialFilterType::Genericblock, kParentAddress,
                               kParentAddress.host(), SiteKey()))
      .WillOnce(Return(true));

  // Search is retried but now for domain-specific filters only. No matches.
  EXPECT_CALL(*sub1, FindRewriteFilter(kImageAddress, kParentAddress.host(),
                                       FilterCategory::DomainSpecificBlocking))
      .WillOnce(Return(absl::nullopt));

  // Finally, no filter found.
  SubscriptionCollectionImpl collection(
      std::vector<scoped_refptr<InstalledSubscription>>{sub1, sub2});
  EXPECT_EQ(collection.GetRewriteUrl(kImageAddress, {kParentAddress}),
            absl::nullopt);
}

}  // namespace adblock

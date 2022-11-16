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

#include "components/adblock/content/browser/element_hider_impl.h"

#include "base/callback_forward.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace adblock {

class AdblockElementHiderFlatbufferTest
    : public content::RenderViewHostTestHarness {
 protected:
  MockSubscriptionService sub_service_;

  const GURL kUrl{"https://domain.com"};
  const std::vector<GURL> kFrameHierarchy{kUrl};
  const SiteKey kSitekey{""};
};

TEST_F(AdblockElementHiderFlatbufferTest, BatchesSelectors) {
  std::vector<base::StringPiece> selectors(1u << 11u, "selector");
  std::vector<base::StringPiece> emu_selectors;

  // SubscriptionService starts initialized.
  EXPECT_CALL(sub_service_, IsInitialized()).WillOnce(testing::Return(true));

  ElementHiderImpl element_hide(&sub_service_);
  EXPECT_CALL(sub_service_, GetCurrentSnapshot())
      .WillOnce([this, selectors, emu_selectors]() {
        auto collection = std::make_unique<MockSubscriptionCollection>();
        EXPECT_CALL(*collection,
                    FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                        kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(absl::nullopt));
        EXPECT_CALL(*collection,
                    FindBySpecialFilter(SpecialFilterType::Elemhide, kUrl,
                                        kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(absl::nullopt));
        EXPECT_CALL(*collection,
                    GetElementHideSelectors(kUrl, kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(selectors));
        EXPECT_CALL(*collection, GetElementHideEmulationSelectors(kUrl))
            .WillOnce(testing::Return(emu_selectors));
        return collection;
      });

  element_hide.ApplyElementHidingEmulationOnPage(
      kUrl, kFrameHierarchy, main_rfh(), kSitekey,
      base::BindLambdaForTesting(
          [&](const ElementHider::ElemhideInjectionData& data) {
            const auto lines =
                base::SplitString(data.stylesheet, "\n", base::KEEP_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);
            EXPECT_EQ(lines.size(), 2u);
            for (auto& line : lines) {
              EXPECT_EQ(base::SplitString(line, ",", base::KEEP_WHITESPACE,
                                          base::SPLIT_WANT_NONEMPTY)
                            .size(),
                        (1u << 10u));  // must not be bigger than 2 ^ 10 = 1024

              EXPECT_TRUE(base::EndsWith(line, "{display: none !important;}"));
            }
          }));
}

TEST_F(AdblockElementHiderFlatbufferTest, UninitializedSubscriptionService) {
  std::vector<base::StringPiece> selectors{"selector"};
  std::vector<base::StringPiece> emu_selectors;

  // SubscriptionService starts uninitialized, ElementHider will post a task.
  EXPECT_CALL(sub_service_, IsInitialized()).WillOnce(testing::Return(false));
  // Remember the posted tasks.
  std::vector<base::OnceClosure> deferred_tasks;
  EXPECT_CALL(sub_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly([&](base::OnceClosure task) {
        deferred_tasks.push_back(std::move(task));
      });

  ElementHiderImpl element_hider(&sub_service_);

  base::MockOnceCallback<void(const ElementHider::ElemhideInjectionData&)>
      callback;
  // The final callback will be ran with the data generated based on selectors
  // from the snapshot.
  EXPECT_CALL(callback, Run(testing::_)).WillOnce([](const auto& data) {
    EXPECT_EQ(data.stylesheet, "selector {display: none !important;}\n");
    EXPECT_TRUE(data.elemhide_js.empty());
  });
  // SubscriptionService is not initialized, so no calls yet.
  EXPECT_CALL(sub_service_, GetCurrentSnapshot()).Times(0);

  element_hider.ApplyElementHidingEmulationOnPage(
      kUrl, kFrameHierarchy, main_rfh(), kSitekey, callback.Get());

  // Once initialized, SubscriptionService will return a snapshot that contains
  // element hiding selectors.
  EXPECT_CALL(sub_service_, GetCurrentSnapshot())
      .WillOnce([this, selectors, emu_selectors]() {
        auto collection = std::make_unique<MockSubscriptionCollection>();
        EXPECT_CALL(*collection,
                    FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                        kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(absl::nullopt));
        EXPECT_CALL(*collection,
                    FindBySpecialFilter(SpecialFilterType::Elemhide, kUrl,
                                        kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(absl::nullopt));
        EXPECT_CALL(*collection,
                    GetElementHideSelectors(kUrl, kFrameHierarchy, kSitekey))
            .WillOnce(testing::Return(selectors));
        EXPECT_CALL(*collection, GetElementHideEmulationSelectors(kUrl))
            .WillOnce(testing::Return(emu_selectors));
        return collection;
      });

  // From now on, SubscriptionService reports it is initialized.
  EXPECT_CALL(sub_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  // Run any tasks submitted to SubscriptionService by testee via
  // RunWhenInitialized().
  for (auto& task : deferred_tasks) {
    std::move(task).Run();
  }
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockElementHiderFlatbufferTest,
       GeneratesSnippetsWhenAllowListedPage) {
  EXPECT_CALL(sub_service_, IsInitialized()).WillOnce(testing::Return(true));

  EXPECT_CALL(sub_service_, GetCurrentSnapshot()).WillOnce([this]() {
    auto collection = std::make_unique<MockSubscriptionCollection>();
    EXPECT_CALL(*collection,
                FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                    kFrameHierarchy, kSitekey))
        .WillOnce(testing::Return(absl::nullopt));
    EXPECT_CALL(*collection,
                FindBySpecialFilter(SpecialFilterType::Elemhide, kUrl,
                                    kFrameHierarchy, kSitekey))
        .WillOnce(testing::Return(GURL("about:blank")));
    EXPECT_CALL(*collection, GenerateSnippetsJson(kUrl, kFrameHierarchy))
        .WillOnce(testing::Return("[]"));
    return collection;
  });

  ElementHiderImpl element_hide(&sub_service_);
  element_hide.ApplyElementHidingEmulationOnPage(
      kUrl, kFrameHierarchy, main_rfh(), kSitekey,
      base::BindLambdaForTesting(
          [&](const ElementHider::ElemhideInjectionData& data) {
            EXPECT_EQ(data.stylesheet, "");
            EXPECT_EQ(data.elemhide_js, "");
            EXPECT_TRUE(!data.snippet_js.empty());
          }));
  task_environment()->RunUntilIdle();
}

TEST_F(AdblockElementHiderFlatbufferTest, GeneratesNothingDocumentAllowListed) {
  EXPECT_CALL(sub_service_, IsInitialized()).WillOnce(testing::Return(true));

  EXPECT_CALL(sub_service_, GetCurrentSnapshot()).WillOnce([this]() {
    auto collection = std::make_unique<MockSubscriptionCollection>();
    EXPECT_CALL(*collection,
                FindBySpecialFilter(SpecialFilterType::Document, kUrl,
                                    kFrameHierarchy, kSitekey))
        .WillOnce(testing::Return(GURL("about:blank")));
    return collection;
  });

  ElementHiderImpl element_hide(&sub_service_);
  element_hide.ApplyElementHidingEmulationOnPage(
      kUrl, kFrameHierarchy, main_rfh(), kSitekey,
      base::BindLambdaForTesting(
          [&](const ElementHider::ElemhideInjectionData& data) {
            EXPECT_EQ(data.stylesheet, "");
            EXPECT_EQ(data.elemhide_js, "");
            EXPECT_EQ(data.snippet_js, "");
          }));
  task_environment()->RunUntilIdle();
}

}  // namespace adblock

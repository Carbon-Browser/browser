/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_controller_impl.h"

#include "base/json/json_string_value_serializer.h"
#include "base/test/mock_callback.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/mock_adblock_platform_embedder.h"
#include "components/adblock/mock_filter_engine.h"
#include "components/adblock/mock_filter_impl.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/Filter.h"

namespace adblock {

class AdblockControllerTest : public content::RenderViewHostTestHarness {
 public:
  AdblockControllerTest() = default;
  ~AdblockControllerTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    adblock::prefs::RegisterProfilePrefs(pref_service_.registry());
    std::unique_ptr<MockFilterEngine> mock_engine =
        std::make_unique<MockFilterEngine>();
    mock_engine_ = mock_engine.get();
    embedder_ = new MockAdblockPlatformEmbedder(
        std::move(mock_engine), task_environment()->GetMainThreadTaskRunner());
    controller_ =
        std::make_unique<AdblockControllerImpl>(&pref_service_, embedder_);
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
  MockFilterEngine* mock_engine_;
  scoped_refptr<MockAdblockPlatformEmbedder> embedder_;
  std::unique_ptr<AdblockControllerImpl> controller_;
};

TEST_F(AdblockControllerTest, RecommendedSubscriptionsAreCached) {
  pref_service_.SetString(prefs::kAdblockRecommendedSubscriptionsVersion,
                          "1.0.0");
  controller_->OnFilterEngineCreated(
      {Subscription(GURL("https://test1.com"), "test1", {"1"}),
       Subscription(GURL("https://test2.com"), "test2", {"1", "2", "3"})},
      {}, {});
  auto* list = pref_service_.GetList(prefs::kAdblockRecommendedSubscriptions);
  const char* expected =
      "[{\"languages\":\"1\",\"title\":\"test1\",\"url\":\"https://"
      "test1.com/"
      "\"},{\"languages\":\"1,2,3\",\"title\":\"test2\",\"url\":\"https://"
      "test2.com/\"}]";
  std::string serialized;
  JSONStringValueSerializer serializer(&serialized);
  serializer.Serialize(*list);
  EXPECT_EQ(std::string{expected}, serialized);
  controller_->OnFilterEngineCreated(
      {Subscription(GURL("https://test1.com"), "test1", {"1"}),
       Subscription(GURL("https://test2.com"), "test2", {"1"})},
      {}, {});
  list = pref_service_.GetList(prefs::kAdblockRecommendedSubscriptions);
  serialized.clear();
  serializer.Serialize(*list);
  // It does not change unless version changes.
  EXPECT_EQ(std::string{expected}, serialized);
}

TEST_F(AdblockControllerTest,
       VersionChangeInvalidatesRecommendedSubscriptionsCache) {
  pref_service_.SetString(prefs::kAdblockRecommendedSubscriptionsVersion,
                          "1.0.0");
  controller_->OnFilterEngineCreated(
      {Subscription(GURL("https://test1.com"), "test1", {"1"}),
       Subscription(GURL("https://test2.com"), "test2", {"1", "2", "3"})},
      {}, {});
  auto* list = pref_service_.GetList(prefs::kAdblockRecommendedSubscriptions);
  const char* expected1 =
      "[{\"languages\":\"1\",\"title\":\"test1\",\"url\":\"https://"
      "test1.com/"
      "\"},{\"languages\":\"1,2,3\",\"title\":\"test2\",\"url\":\"https://"
      "test2.com/\"}]";
  std::string serialized;
  JSONStringValueSerializer serializer(&serialized);
  serializer.Serialize(*list);
  EXPECT_EQ(std::string{expected1}, serialized);
  pref_service_.SetString(prefs::kAdblockRecommendedSubscriptionsVersion,
                          "1.0.1");
  controller_->OnFilterEngineCreated(
      {Subscription(GURL("https://test1.com"), "test1", {"1"}),
       Subscription(GURL("https://test2.com"), "test2", {"1"})},
      {}, {});
  list = pref_service_.GetList(prefs::kAdblockRecommendedSubscriptions);
  serialized.clear();
  serializer.Serialize(*list);
  const char* expected2 =
      "[{\"languages\":\"1\",\"title\":\"test1\",\"url\":\"https://"
      "test1.com/"
      "\"},{\"languages\":\"1\",\"title\":\"test2\",\"url\":\"https://"
      "test2.com/\"}]";
  EXPECT_EQ(std::string{expected2}, serialized);
}

TEST_F(AdblockControllerTest, AddCustomFilterWithoutAndWithFilterEngine) {
  // without filter engine
  std::unique_ptr<MockFilterEngine> engine = embedder_->TakeFilterEngine();
  embedder_->SetFilterEngine(nullptr);

  {
    base::RunLoop loop;
    EXPECT_CALL(*mock_engine_, GetFilter("test")).Times(0);
    EXPECT_CALL(*mock_engine_, AddFilter(testing::_)).Times(0);

    controller_->AddCustomFilter("test");

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    embedder_->SetFilterEngine(std::move(engine));

    AdblockPlus::Filter filter(std::make_unique<MockFilterImpl>(
        "test", AdblockPlus::Filter::Type::TYPE_BLOCKING));

    EXPECT_CALL(*mock_engine_, GetFilter("test"))
        .Times(1)
        .WillOnce(testing::Return(filter));
    EXPECT_CALL(*mock_engine_, AddFilter(filter)).Times(1);

    embedder_->NotifyOnEngineCreated();

    loop.RunUntilIdle();
  }
}

TEST_F(AdblockControllerTest,
       DISABLED_RemoveCustomFilterWithoutAndWithFilterEngine) {
  // without filter engine
  std::unique_ptr<MockFilterEngine> engine = embedder_->TakeFilterEngine();
  embedder_->SetFilterEngine(nullptr);

  {
    base::RunLoop loop;
    EXPECT_CALL(*mock_engine_, GetFilter("test")).Times(0);
    EXPECT_CALL(*mock_engine_, RemoveFilter(testing::_)).Times(0);

    controller_->RemoveCustomFilter("test");

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    embedder_->SetFilterEngine(std::move(engine));

    AdblockPlus::Filter filter(std::make_unique<MockFilterImpl>(
        "test", AdblockPlus::Filter::Type::TYPE_BLOCKING));

    EXPECT_CALL(*mock_engine_, GetFilter("test"))
        .Times(1)
        .WillOnce(testing::Return(filter));
    EXPECT_CALL(*mock_engine_, RemoveFilter(filter)).Times(1);

    embedder_->NotifyOnEngineCreated();

    loop.RunUntilIdle();
  }
}

TEST_F(AdblockControllerTest, DISABLED_QueueOrder) {
  std::unique_ptr<MockFilterEngine> engine = embedder_->TakeFilterEngine();
  embedder_->SetFilterEngine(nullptr);

  {
    base::RunLoop loop;
    EXPECT_CALL(*mock_engine_, GetFilter(testing::_)).Times(0);
    EXPECT_CALL(*mock_engine_, AddFilter(testing::_)).Times(0);
    EXPECT_CALL(*mock_engine_, RemoveFilter(testing::_)).Times(0);

    controller_->AddCustomFilter("test_add");
    controller_->RemoveCustomFilter("test_remove");

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    embedder_->SetFilterEngine(std::move(engine));

    AdblockPlus::Filter filter(std::make_unique<MockFilterImpl>(
        "test", AdblockPlus::Filter::Type::TYPE_BLOCKING));

    testing::InSequence sequence;
    EXPECT_CALL(*mock_engine_, GetFilter("test_add"))
        .Times(1)
        .WillOnce(testing::Return(filter));
    EXPECT_CALL(*mock_engine_, AddFilter(filter)).Times(1);
    EXPECT_CALL(*mock_engine_, GetFilter("test_remove"))
        .Times(1)
        .WillOnce(testing::Return(filter));
    EXPECT_CALL(*mock_engine_, RemoveFilter(filter)).Times(1);

    embedder_->NotifyOnEngineCreated();
    loop.RunUntilIdle();
  }
}

}  // namespace adblock

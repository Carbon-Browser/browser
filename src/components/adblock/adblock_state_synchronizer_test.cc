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

#include "components/adblock/adblock_state_synchronizer.h"

#include "base/run_loop.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/fake_subscription_impl.h"
#include "components/adblock/mock_adblock_platform_embedder.h"
#include "components/adblock/mock_filter_engine.h"
#include "components/prefs/pref_member.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace testing;
using namespace adblock;

namespace {

class AdblockStateSynchronizerTest : public testing::Test {
 public:
  AdblockStateSynchronizerTest() {}
  ~AdblockStateSynchronizerTest() override {}

  void SetUp() override {
    adblock::prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  AdblockStateSynchronizerTest(const AdblockStateSynchronizerTest&) = delete;
  AdblockStateSynchronizerTest& operator=(const AdblockStateSynchronizerTest&) =
      delete;

  base::test::TaskEnvironment task_env_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(AdblockStateSynchronizerTest, SynchronizeSubscriptions) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription1_url{"https://foo.bar"};
  GURL subscription2_url{"https://bar.baz"};
  std::unique_ptr<FakeSubscriptionImpl> mock_subscription1 =
      std::make_unique<FakeSubscriptionImpl>(subscription1_url, "title1",
                                             std::vector<std::string>{"pl"},
                                             kSynchronizationOkStatus, false);
  std::unique_ptr<FakeSubscriptionImpl> mock_subscription2 =
      std::make_unique<FakeSubscriptionImpl>(subscription2_url, "title2",
                                             std::vector<std::string>{"pl"},
                                             kSynchronizationOkStatus, false);
  AdblockPlus::Subscription subscription1(std::move(mock_subscription1));
  AdblockPlus::Subscription subscription2(std::move(mock_subscription2));

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription1_url.spec()))
      .WillOnce(Return(subscription1));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription2_url.spec()))
      .WillOnce(Return(subscription2));
  EXPECT_CALL(*mock_engine, RemoveSubscription(subscription1)).Times(1);
  EXPECT_CALL(*mock_engine, AddSubscription(subscription2)).Times(1);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  StringListPrefMember subscriptions;
  subscriptions.Init(adblock::prefs::kAdblockSubscriptions, &pref_service_);
  synchronizer.Init(&pref_service_);
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();
  subscriptions.SetValue({subscription2_url.spec()});
  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, SubscriptionsChangeBeforeEngineIsCreated) {
  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(nullptr,
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  StringListPrefMember subscriptions;
  subscriptions.Init(adblock::prefs::kAdblockSubscriptions, &pref_service_);
  synchronizer.Init(&pref_service_);

  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription_url{"https://foo.bar"};
  auto mock_subscription = std::make_unique<FakeSubscriptionImpl>(
      subscription_url, "title", std::vector<std::string>{"pl"},
      kSynchronizationOkStatus, false);

  AdblockPlus::Subscription subscription(std::move(mock_subscription));

  // Set after init but before the engine is created. Changes have to be
  // spotted.
  subscriptions.SetValue({subscription_url.spec()});
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription}));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription_url.spec()))
      .WillOnce(Return(subscription));
  EXPECT_CALL(*mock_engine, AddSubscription(subscription)).Times(1);
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->SetFilterEngine(std::move(mock_engine));

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, CustomSubscriptions) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription1_url{"https://foo.bar"};
  GURL subscription2_url{"https://bar.baz"};
  auto mock_subscription1 = std::make_unique<FakeSubscriptionImpl>(
      subscription1_url, "title1", std::vector<std::string>{"pl"},
      kSynchronizationOkStatus, false);
  auto mock_subscription2 = std::make_unique<FakeSubscriptionImpl>(
      subscription2_url, "title2", std::vector<std::string>{"pl"},
      kSynchronizationOkStatus, false);
  AdblockPlus::Subscription subscription1(std::move(mock_subscription1));
  AdblockPlus::Subscription subscription2(std::move(mock_subscription2));

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription1_url.spec()))
      .WillOnce(Return(subscription1));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription2_url.spec()))
      .WillOnce(Return(subscription2));
  EXPECT_CALL(*mock_engine, RemoveSubscription(subscription1)).Times(1);
  EXPECT_CALL(*mock_engine, AddSubscription(subscription2)).Times(1);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  StringListPrefMember subscriptions;
  subscriptions.Init(adblock::prefs::kAdblockCustomSubscriptions,
                     &pref_service_);
  synchronizer.Init(&pref_service_);
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();
  subscriptions.SetValue({subscription2_url.spec()});
  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest,
       CustomSubscriptionsChangeBeforeEngineIsCreated) {
  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(nullptr,
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  StringListPrefMember subscriptions;
  subscriptions.Init(adblock::prefs::kAdblockCustomSubscriptions,
                     &pref_service_);
  synchronizer.Init(&pref_service_);

  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription_url{"https://foo.bar"};
  auto mock_subscription = std::make_unique<FakeSubscriptionImpl>(
      subscription_url, "title", std::vector<std::string>{"pl"},
      kSynchronizationOkStatus, false);

  AdblockPlus::Subscription subscription(std::move(mock_subscription));

  // Set after init but before the engine is created. Changes have to be
  // spotted.
  subscriptions.SetValue({subscription_url.spec()});
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription}));
  EXPECT_CALL(*mock_engine, GetSubscription(subscription_url.spec()))
      .WillOnce(Return(subscription));
  EXPECT_CALL(*mock_engine, AddSubscription(subscription)).Times(1);
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->SetFilterEngine(std::move(mock_engine));

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, EnableAdblock) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  EXPECT_CALL(*mock_engine, GetListedSubscriptions()).Times(1);

  // Init to true before wire everything is because the Adblock is
  // enabled by default so we want to be in the same state.
  BooleanPrefMember enabled;
  enabled.Init(adblock::prefs::kEnableAdblock, &pref_service_);
  enabled.SetValue(true);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);
  EXPECT_TRUE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(synchronizer.IsAdblockEnabled(), *enabled);

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();
  enabled.SetValue(false);
  EXPECT_FALSE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(synchronizer.IsAdblockEnabled(), *enabled);

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, EnableAA) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  {
    InSequence seq;
    EXPECT_CALL(*mock_engine, SetAAEnabled(true)).Times(1);
    EXPECT_CALL(*mock_engine, SetAAEnabled(false)).Times(1);
  }

  // Init to true before wire everything is because the Adblock is
  // enabled by default so we want to be in the same state.
  BooleanPrefMember aa_enabled;
  aa_enabled.Init(adblock::prefs::kEnableAcceptableAds, &pref_service_);
  aa_enabled.SetValue(true);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);
  EXPECT_TRUE(synchronizer.IsAdblockEnabled());

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();
  aa_enabled.SetValue(false);

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, AllowedDomains) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  EXPECT_CALL(*mock_engine, GetFilter(_)).Times(4);
  EXPECT_CALL(*mock_engine, AddFilter(_)).Times(3);
  EXPECT_CALL(*mock_engine, RemoveFilter(_)).Times(1);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);
  EXPECT_TRUE(synchronizer.IsAdblockEnabled());

  std::string test_domain1("foo.com");
  std::string test_domain2("bar.com");
  StringListPrefMember domain;
  domain.Init(adblock::prefs::kAdblockAllowedDomains, &pref_service_);
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  // 1st change to add domain1
  domain.SetValue({test_domain1});
  base::RunLoop().RunUntilIdle();

  // 2nd change to remove domain1 and (re)add domain1 and add domain2
  domain.SetValue({test_domain1, test_domain2});
  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest,
       AllowedDomainsChangeBefereEngineIsCreated) {
  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(nullptr,
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);

  std::string test_domain("foo.com");
  StringListPrefMember domain;
  domain.Init(adblock::prefs::kAdblockAllowedDomains, &pref_service_);

  // Change the value after init but before engine creation. All the changes
  // should be noticed.
  domain.SetValue({test_domain});
  base::RunLoop().RunUntilIdle();

  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  EXPECT_CALL(*mock_engine, GetFilter(_)).Times(1);
  EXPECT_CALL(*mock_engine, AddFilter(_)).Times(1);

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->SetFilterEngine(std::move(mock_engine));
  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, AllowedConnectionType) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription1_url{"https://foo.bar"};
  auto mock_subscription1 = std::make_unique<FakeSubscriptionImpl>(
      subscription1_url, "title1", std::vector<std::string>{"pl"},
      kSynchronizationOkStatus, false);
  AdblockPlus::Subscription subscription1(std::move(mock_subscription1));

  std::string any = AllowedConnectionTypeToString(AllowedConnectionType::kAny);
  std::string wifi =
      AllowedConnectionTypeToString(AllowedConnectionType::kWiFi);
  std::string none =
      AllowedConnectionTypeToString(AllowedConnectionType::kNone);

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(any))).Times(1);
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(wifi))).Times(1);
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(none))).Times(1);

  StringPrefMember connection_type;
  connection_type.Init(adblock::prefs::kAdblockAllowedConnectionType,
                       &pref_service_);
  connection_type.SetValue(wifi);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);
  EXPECT_TRUE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionTypeFromString(connection_type.GetValue()));

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  connection_type.SetValue(any);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionTypeFromString(connection_type.GetValue()));
  EXPECT_EQ(subscription1.GetSynchronizationStatus(), kSynchronizationOkStatus);
  connection_type.SetValue(none);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionTypeFromString(connection_type.GetValue()));
  EXPECT_EQ(subscription1.GetSynchronizationStatus(), kSynchronizationOkStatus);

  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockStateSynchronizerTest, AllowedConnectionType_AdbDisabled) {
  std::unique_ptr<MockFilterEngine> mock_engine =
      std::make_unique<MockFilterEngine>();

  GURL subscription1_url{"https://foo.bar"};
  bool has_updated_filters = false;
  auto mock_subscription1 = std::make_unique<FakeSubscriptionImpl>(
      subscription1_url, "title1", std::vector<std::string>{"pl"},
      kSynchronizationErrorStatus, false, &has_updated_filters);
  AdblockPlus::Subscription subscription1(std::move(mock_subscription1));

  std::string any = AllowedConnectionTypeToString(AllowedConnectionType::kAny);
  std::string wifi =
      AllowedConnectionTypeToString(AllowedConnectionType::kWiFi);
  std::string none =
      AllowedConnectionTypeToString(AllowedConnectionType::kNone);

  EXPECT_CALL(*mock_engine, FetchAvailableSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, GetListedSubscriptions())
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}))
      .WillOnce(Return(std::vector<AdblockPlus::Subscription>{subscription1}));
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(any))).Times(1);
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(wifi))).Times(1);
  EXPECT_CALL(*mock_engine, SetAllowedConnectionType(Pointee(none))).Times(2);

  StringPrefMember connection_type;
  connection_type.Init(adblock::prefs::kAdblockAllowedConnectionType,
                       &pref_service_);
  connection_type.SetValue(wifi);

  scoped_refptr<AdblockPlatformEmbedder> embedder =
      new MockAdblockPlatformEmbedder(std::move(mock_engine),
                                      task_env_.GetMainThreadTaskRunner());
  AdblockStateSynchronizer synchronizer(embedder);
  synchronizer.Init(&pref_service_);

  static_cast<MockAdblockPlatformEmbedder*>(embedder.get())
      ->NotifyOnEngineCreated();

  EXPECT_TRUE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(subscription1.GetSynchronizationStatus(),
            kSynchronizationErrorStatus);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionTypeFromString(*connection_type));

  pref_service_.SetBoolean(adblock::prefs::kEnableAdblock, false);
  EXPECT_FALSE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(subscription1.GetSynchronizationStatus(),
            kSynchronizationErrorStatus);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionType::kNone);

  connection_type.SetValue(any);
  EXPECT_EQ(subscription1.GetSynchronizationStatus(),
            kSynchronizationErrorStatus);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionType::kNone);

  pref_service_.SetBoolean(adblock::prefs::kEnableAdblock, true);
  EXPECT_TRUE(synchronizer.IsAdblockEnabled());
  EXPECT_EQ(subscription1.GetSynchronizationStatus(),
            kSynchronizationErrorStatus);
  EXPECT_EQ(synchronizer.GetAllowedConnectionType(),
            AllowedConnectionTypeFromString(*connection_type));  // any

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(has_updated_filters);
}

}  // namespace

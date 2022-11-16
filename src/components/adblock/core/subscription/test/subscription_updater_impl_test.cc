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

#include "components/adblock/core/subscription/subscription_updater_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/test/mock_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {
constexpr auto kInitialDelay = base::Minutes(1);
constexpr auto kCheckInterval = base::Hours(1);
}  // namespace

class AdblockSubscriptionUpdaterImplTest : public testing::Test {
 public:
  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  std::unique_ptr<SubscriptionUpdaterImpl> MakeUpdater() {
    return std::make_unique<SubscriptionUpdaterImpl>(
        &pref_service_, &subscription_service_, &persistent_metadata_,
        kInitialDelay, kCheckInterval);
  }

  void ExpectUpdateCheckWillRun() {
    // Subscriptions 1 and 2 are installed.
    // Subscription 3 and 4 are installing (with a preloaded fallback and
    // without, should make no difference for the Updater).
    EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
        .WillRepeatedly(testing::Return(std::vector<
                                        scoped_refptr<Subscription>>{
            MakeMockSubscription(GURL("https://subscription.com/1.txt"),
                                 Subscription::InstallationState::Installed),
            MakeMockSubscription(GURL("https://subscription.com/2.txt"),
                                 Subscription::InstallationState::Installed),
            MakeMockSubscription(GURL("https://subscription.com/3.txt"),
                                 Subscription::InstallationState::Preloaded),
            MakeMockSubscription(GURL("https://subscription.com/4.txt"),
                                 Subscription::InstallationState::Installing),
        }));

    // Subscription 1 is expired, subscription 2 is not.
    EXPECT_CALL(persistent_metadata_,
                IsExpired(GURL("https://subscription.com/1.txt")))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(persistent_metadata_,
                IsExpired(GURL("https://subscription.com/2.txt")))
        .WillRepeatedly(testing::Return(false));
    // Subscription 3 won't be queried, as it is not installed yet.
    EXPECT_CALL(persistent_metadata_,
                IsExpired(GURL("https://subscription.com/3.txt")))
        .Times(0);
    // Subscription 4 won't be queried, as it is not installed yet.
    EXPECT_CALL(persistent_metadata_,
                IsExpired(GURL("https://subscription.com/4.txt")))
        .Times(0);
    // Acceptable Ads subscription is not installed, but will be queried in
    // order to establish whether to send a ping request.
    EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
        .WillRepeatedly(testing::Return(false));

    // Only subscription 1 will be updated.
    EXPECT_CALL(subscription_service_,
                DownloadAndInstallSubscription(
                    GURL("https://subscription.com/1.txt"), testing::_));
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  testing::StrictMock<MockSubscriptionPersistentMetadata> persistent_metadata_;
  testing::StrictMock<MockSubscriptionService> subscription_service_;
};

TEST_F(AdblockSubscriptionUpdaterImplTest,
       StartsAfterAdblockingBecomesEnabled) {
  // SubscriptionService is initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);
  auto updater = MakeUpdater();
  updater->StartSchedule();
  // No calls made, even as time passes.
  task_environment_.FastForwardBy(base::Days(7));
  // Adblocking gets enabled.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  // The update check runs after initial delay.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       StartsAfterServiceBecomesInitialized) {
  // Adblocking is enabled but SubscriptionService is not initialized.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));
  // Updater will wait for initialization with a pending task.
  base::OnceClosure pending_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillOnce(
          [&](base::OnceClosure task) { pending_task = std::move(task); });

  auto updater = MakeUpdater();
  updater->StartSchedule();
  // No calls made, even as time passes.
  task_environment_.FastForwardBy(base::Days(7));
  // SubscriptionService becomes initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(pending_task).Run();

  // The update check runs after initial delay.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       StartsAfterServiceBecomesInitializedAndAdblockBecomesInitialized) {
  // Adblocking is disabled and SubscriptionService is not initialized.
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));
  // Updater will wait for initialization with a pending task.
  base::OnceClosure pending_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillOnce(
          [&](base::OnceClosure task) { pending_task = std::move(task); });

  auto updater = MakeUpdater();
  updater->StartSchedule();
  // No calls made, even as time passes.
  task_environment_.FastForwardBy(base::Days(7));
  // Adblocking gets enabled.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  // SubscriptionService becomes initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(pending_task).Run();

  // The update check runs after initial delay.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest, UpdateCheckRepeatsInIntervals) {
  // Adblocking is enabled and SubscriptionService is initialized, the schedule
  // can start immediately.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // The update check runs after initial delay.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kInitialDelay);

  // Then it runs after every kCheckInterval.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kCheckInterval);
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kCheckInterval);
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kCheckInterval);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       UpdateChecksStopWhenAdblockingDisabled) {
  // Adblocking is enabled and SubscriptionService is initialized, the schedule
  // can start immediately.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // The update check runs after initial delay.
  ExpectUpdateCheckWillRun();
  task_environment_.FastForwardBy(kInitialDelay);

  // Now we disable adblocking.
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);

  // No calls made, even as time passes.
  task_environment_.FastForwardBy(base::Days(7));
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       AcceptableAdsPingNotSentWhenInstalled) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // Acceptable Ads subscription already installed.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(std::vector<scoped_refptr<Subscription>>{
          MakeMockSubscription(AcceptableAdsUrl(),
                               Subscription::InstallationState::Installed)}));

  // AA subscription is not expired yet.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(false));

  // Nothing updated, no ping sent.
  EXPECT_CALL(subscription_service_,
              DownloadAndInstallSubscription(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(subscription_service_, PingAcceptableAds(testing::_)).Times(0);

  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       AcceptableAdsPingNotSentWhenPending) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // Acceptable Ads subscription not installed yet, but pending installation.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(std::vector<scoped_refptr<Subscription>>{
          MakeMockSubscription(AcceptableAdsUrl(),
                               Subscription::InstallationState::Installing)}));

  // Since it's not installed yet, we don't check expiration.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl())).Times(0);

  // Nothing updated, no ping sent.
  EXPECT_CALL(subscription_service_,
              DownloadAndInstallSubscription(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(subscription_service_, PingAcceptableAds(testing::_)).Times(0);

  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest,
       AcceptableAdsPingNotSentWhenNotExpired) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // Acceptable Ads subscription not enabled.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<Subscription>>{}));

  // We check whether the subscription is expired despite it not being
  // installed. This will query the expiration metadata maintained via
  // previous ping responses.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(false));

  // Nothing updated, no ping sent.
  EXPECT_CALL(subscription_service_,
              DownloadAndInstallSubscription(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(subscription_service_, PingAcceptableAds(testing::_)).Times(0);

  task_environment_.FastForwardBy(kInitialDelay);
}

TEST_F(AdblockSubscriptionUpdaterImplTest, AcceptableAdsPingSentWhenExpired) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  auto updater = MakeUpdater();
  updater->StartSchedule();

  // Acceptable Ads subscription not enabled.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<Subscription>>{}));

  // Previous ping response came back with a date that is now past expiration
  // time.
  EXPECT_CALL(persistent_metadata_, IsExpired(AcceptableAdsUrl()))
      .WillRepeatedly(testing::Return(true));

  // AA subscription is not downloaded, but ping is sent.
  EXPECT_CALL(subscription_service_,
              DownloadAndInstallSubscription(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(subscription_service_, PingAcceptableAds(testing::_)).Times(1);

  task_environment_.FastForwardBy(kInitialDelay);
}

}  // namespace adblock

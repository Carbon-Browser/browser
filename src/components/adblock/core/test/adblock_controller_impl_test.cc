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

#include "components/adblock/core/adblock_controller_impl.h"

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/task_environment.h"
#include "base/value_iterators.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/subscription/test/mock_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using InstallationState = Subscription::InstallationState;

namespace {

class MockObserver : public AdblockController::Observer {
 public:
  MOCK_METHOD(void, OnSubscriptionUpdated, (const GURL& url), (override));
};

}  // namespace

bool operator==(const KnownSubscriptionInfo& lhs,
                const KnownSubscriptionInfo& rhs) {
  return lhs.url == rhs.url && lhs.languages == rhs.languages &&
         lhs.title == rhs.title;
}

class AdblockControllerImplTest : public testing::Test {
 public:
  AdblockControllerImplTest() {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    RecreateController();
    custom_subscriptions_pref_.Init(prefs::kAdblockCustomSubscriptions,
                                    &pref_service_);
    subscriptions_pref_.Init(prefs::kAdblockSubscriptions, &pref_service_);
  }

  void RecreateController(std::string locale = "pl-PL") {
    testee_ = std::make_unique<AdblockControllerImpl>(
        &pref_service_, &subscription_service_, locale);
  }

  void ExpectInstallationTriggered(const GURL& subscription_url) {
    EXPECT_CALL(subscription_service_,
                DownloadAndInstallSubscription(subscription_url, testing::_))
        .WillOnce(base::test::RunOnceCallback<1>(true));
  }

  void ExpectNoInstallation(const GURL& subscription_url) {
    EXPECT_CALL(subscription_service_,
                DownloadAndInstallSubscription(subscription_url, testing::_))
        .Times(0);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  MockSubscriptionService subscription_service_;
  StringListPrefMember custom_subscriptions_pref_;
  StringListPrefMember subscriptions_pref_;
  std::unique_ptr<AdblockControllerImpl> testee_;
  const GURL kRecommendedSubscription{
      "https://easylist-downloads.adblockplus.org/easylistpolish+easylist.txt"};
};

namespace allowed_domain_test_data {
std::string domain_google = "google.com";
std::string domain_google_upper = "GooGlE.com";
std::string domain_filter_google = "@@||google.com^$document,domain=google.com";
std::vector<std::string> with_domain_google{domain_google};
std::vector<std::string> with_domain_filter_google{domain_filter_google};

std::string domain_facebook = "facebook.com";
std::string domain_filter_facebook =
    "@@||facebook.com^$document,domain=facebook.com";
std::vector<std::string> with_domain_facebook{domain_facebook};
std::vector<std::string> with_domain_filter_facebook{domain_filter_facebook};

std::vector<std::string> with_domain_google_and_facebook{domain_google,
                                                         domain_facebook};
std::vector<std::string> with_domain_filter_google_and_facebook{
    domain_filter_google, domain_filter_facebook};

std::vector<std::string> empty;
}  // namespace allowed_domain_test_data

TEST_F(AdblockControllerImplTest, EnableAndDisableAdBlocking) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  // By default, ad-blocking is enabled.
  EXPECT_TRUE(testee_->IsAdblockEnabled());
  // Disabling it is registered.
  testee_->SetAdblockEnabled(false);
  EXPECT_FALSE(testee_->IsAdblockEnabled());
  // The disabled state persists after restart.
  RecreateController();
  EXPECT_FALSE(testee_->IsAdblockEnabled());
}

TEST_F(AdblockControllerImplTest, SetUpdateConstent) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  // Initially, default value is returned.
  EXPECT_EQ(testee_->GetUpdateConsent(), AllowedConnectionType::kDefault);
  // Setting it to a different value is registered.
  testee_->SetUpdateConsent(AllowedConnectionType::kWiFi);
  EXPECT_EQ(testee_->GetUpdateConsent(), AllowedConnectionType::kWiFi);
  // The setting persists after restart.
  RecreateController();
  EXPECT_EQ(testee_->GetUpdateConsent(), AllowedConnectionType::kWiFi);
}

TEST_F(AdblockControllerImplTest, SelectedBuiltInSubscriptionsReadFromPrefs) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  const GURL subscription1("https://subscription.com/1.txt");
  const GURL subscription2("https://subscription.com/2.txt");
  const GURL subscription_not_in_prefs("https://subscription.com/3.txt");

  const std::vector<scoped_refptr<Subscription>> current_subscriptions = {
      MakeMockSubscription(subscription1), MakeMockSubscription(subscription2),
      MakeMockSubscription(subscription_not_in_prefs),
      MakeMockSubscription(AcceptableAdsUrl())};
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(current_subscriptions));

  // so that anti-cv isn't tagged along.
  BooleanPrefMember first_run;
  first_run.Init(prefs::kInstallFirstStartSubscriptions, &pref_service_);
  first_run.SetValue(false);

  StringListPrefMember list;
  list.Init(prefs::kAdblockSubscriptions, &pref_service_);
  list.SetValue({subscription1.spec(), subscription2.spec()});

  // subscription_not_in_prefs is not returned as it is not in
  // kAdblockSubscriptions. Acceptable Ads subscription is returned despite
  // not being in kAdblockSubscriptions prefs because AA is enabled (a separate
  // pref).
  EXPECT_THAT(testee_->GetSelectedBuiltInSubscriptions(),
              testing::UnorderedElementsAre(current_subscriptions[0],
                                            current_subscriptions[1],
                                            current_subscriptions[3]));
}

TEST_F(AdblockControllerImplTest, CustomSubscriptionsReadFromPrefs) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  const GURL subscription1("https://subscription.com/1.txt");
  const GURL subscription2("https://subscription.com/2.txt");
  const GURL subscription_not_in_prefs("https://subscription.com/3.txt");

  const std::vector<scoped_refptr<Subscription>> current_subscriptions = {
      MakeMockSubscription(subscription1), MakeMockSubscription(subscription2),
      MakeMockSubscription(subscription_not_in_prefs),
      MakeMockSubscription(AcceptableAdsUrl())};
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(current_subscriptions));

  StringListPrefMember list;
  list.Init(prefs::kAdblockCustomSubscriptions, &pref_service_);
  list.SetValue({subscription1.spec(), subscription2.spec()});

  // subscription_not_in_prefs is not returned as it is not in
  // kAdblockCustomSubscriptions. Acceptable Ads subscription is not returned
  // because it is not a custom subscription.
  EXPECT_THAT(testee_->GetCustomSubscriptions(),
              testing::UnorderedElementsAre(current_subscriptions[0],
                                            current_subscriptions[1]));
}

TEST_F(AdblockControllerImplTest, GetKnownSubscriptions) {
  auto subscriptions = config::GetKnownSubscriptions();
  subscriptions.emplace_back(AcceptableAdsUrl(), std::string("Acceptable Ads"),
                             std::vector<std::string>{},
                             SubscriptionUiVisibility::Invisible,
                             SubscriptionFirstRunBehavior::Subscribe,
                             SubscriptionPrivilegedFilterStatus::Forbidden);
  EXPECT_EQ(testee_->GetKnownSubscriptions(), subscriptions);
}

TEST_F(AdblockControllerImplTest,
       AddingCustomSubscriptionTriggersInstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  const GURL subscription_url("https://subscription.com/1.txt");
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  ExpectInstallationTriggered(subscription_url);

  testee_->AddCustomSubscription(subscription_url);
  testee_->RemoveObserver(&observer);
  // URL was added to prefs.
  EXPECT_EQ(*custom_subscriptions_pref_,
            std::vector<std::string>{subscription_url.spec()});
}

TEST_F(AdblockControllerImplTest,
       SelectingBuiltInSubscriptionTriggersInstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  ASSERT_GE(config::GetKnownSubscriptions().size(), 1u);
  const GURL subscription_url = config::GetKnownSubscriptions()[0].url;
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  ExpectInstallationTriggered(subscription_url);

  testee_->SelectBuiltInSubscription(subscription_url);
  testee_->RemoveObserver(&observer);
  // URL was added to prefs.
  EXPECT_THAT(*subscriptions_pref_, testing::Contains(subscription_url));
}

TEST_F(AdblockControllerImplTest, EnablingAcceptableAdsInstallsSubscription) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  pref_service_.SetBoolean(prefs::kEnableAcceptableAds, false);
  const GURL subscription_url = AcceptableAdsUrl();
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(std::vector<scoped_refptr<Subscription>>{}));
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  ExpectInstallationTriggered(subscription_url);

  testee_->SetAcceptableAdsEnabled(true);
  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest, SelectingAcceptableAdsInstallsSubscription) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  pref_service_.SetBoolean(prefs::kEnableAcceptableAds, false);
  const GURL subscription_url = AcceptableAdsUrl();
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(std::vector<scoped_refptr<Subscription>>{}));
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  ExpectInstallationTriggered(subscription_url);

  testee_->SelectBuiltInSubscription(subscription_url);
  EXPECT_TRUE(testee_->IsAcceptableAdsEnabled());
  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest,
       RemovingCustomSubscriptionTriggersUninstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  const GURL subscription_url("https://subscription.com/1.txt");
  // The URL is initially in Prefs.
  custom_subscriptions_pref_.SetValue({subscription_url.spec()});
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  EXPECT_CALL(subscription_service_, UninstallSubscription(subscription_url));

  testee_->RemoveCustomSubscription(subscription_url);
  testee_->RemoveObserver(&observer);
  // The URL has been removed from Prefs.
  EXPECT_TRUE(custom_subscriptions_pref_.GetValue().empty());
}

TEST_F(AdblockControllerImplTest,
       UnselectingBuiltInSubscriptionTriggersUninstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  // so that anti-cv ins't tagged along.
  BooleanPrefMember first_run;
  first_run.Init(prefs::kInstallFirstStartSubscriptions, &pref_service_);
  first_run.SetValue(false);

  ASSERT_GE(config::GetKnownSubscriptions().size(), 1u);
  const GURL subscription_url = config::GetKnownSubscriptions()[0].url;
  // The URL is initially in Prefs.
  subscriptions_pref_.SetValue({subscription_url.spec()});
  MockObserver observer;
  testee_->AddObserver(&observer);
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  EXPECT_CALL(subscription_service_, UninstallSubscription(subscription_url));

  testee_->UnselectBuiltInSubscription(subscription_url);
  testee_->RemoveObserver(&observer);
  // The URL has been removed from Prefs.
  EXPECT_TRUE(subscriptions_pref_.GetValue().empty());
}

TEST_F(AdblockControllerImplTest,
       DisablingAcceptableAdsTriggersUninstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  const GURL subscription_url = AcceptableAdsUrl();
  MockObserver observer;
  testee_->AddObserver(&observer);
  // Acceptable Ads subscription currently installed, returned by
  // SubscriptionService::GetCurrentSubscriptions().
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(
          std::vector<scoped_refptr<Subscription>>{MakeMockSubscription(
              AcceptableAdsUrl(), InstallationState::Installed)}));
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  EXPECT_CALL(subscription_service_, UninstallSubscription(subscription_url));

  testee_->SetAcceptableAdsEnabled(false);
  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest,
       UnselectingAcceptableAdsTriggersUninstallation) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  const GURL subscription_url = AcceptableAdsUrl();
  MockObserver observer;
  testee_->AddObserver(&observer);
  // Acceptable Ads subscription currently installed, returned by
  // SubscriptionService::GetCurrentSubscriptions().
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(
          std::vector<scoped_refptr<Subscription>>{MakeMockSubscription(
              AcceptableAdsUrl(), InstallationState::Installed)}));
  EXPECT_CALL(observer, OnSubscriptionUpdated(subscription_url));
  EXPECT_CALL(subscription_service_, UninstallSubscription(subscription_url));

  testee_->UnselectBuiltInSubscription(subscription_url);
  EXPECT_FALSE(testee_->IsAcceptableAdsEnabled());
  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest,
       SynchronizingSubscriptionsWhenServiceBecomesInitialized) {
  // SubscriptionService reports its not initialized yet.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));
  // This is not a first run.
  pref_service_.SetBoolean(prefs::kInstallFirstStartSubscriptions, false);

  // SynchronizeWithPrefsWhenPossible() will schedule a delayed task.
  base::OnceClosure delayed_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly(
          [&](base::OnceClosure task) { delayed_task = std::move(task); });
  testee_->SynchronizeWithPrefsWhenPossible();

  const GURL new_built_in_subscription = config::GetKnownSubscriptions()[0].url;
  const GURL installed_built_in_subscription =
      config::GetKnownSubscriptions()[1].url;
  const GURL installed_custom_subscription("https://subscription.com/1.txt");
  const GURL temporary_custom_subscription("https://subscription.com/2.txt");
  const GURL unexpected_installed_subscription(
      "https://subscription.com/4.txt");

  MockObserver observer;
  testee_->AddObserver(&observer);

  // For now, we expect no notifications and no calls to SubscriptionService, as
  // it is not initialized.
  EXPECT_CALL(subscription_service_,
              DownloadAndInstallSubscription(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(observer, OnSubscriptionUpdated(testing::_)).Times(0);

  // Call methods that install and uninstall subscriptions, they will only
  // modify Prefs.
  testee_->AddCustomSubscription(installed_custom_subscription);
  testee_->AddCustomSubscription(temporary_custom_subscription);
  testee_->RemoveCustomSubscription(temporary_custom_subscription);
  testee_->SelectBuiltInSubscription(new_built_in_subscription);
  testee_->SelectBuiltInSubscription(installed_built_in_subscription);

  // There are no current subscriptions yet as the SubscriptionService is not
  // initialized. The SubscriptionService is not asked to provide subscriptions
  // as it is not legal to query an uninitialized SubscriptionService.
  // TODO(mpawlowski) those getters could instead return dummy Subscriptions
  // populated based on prefs. Is it worth the complexity, considering how
  // fast SubscriptionService normally starts up?
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions()).Times(0);
  EXPECT_TRUE(testee_->GetSelectedBuiltInSubscriptions().empty());
  EXPECT_TRUE(testee_->GetCustomSubscriptions().empty());

  // Since |temporary_custom_subscription| was added and then removed while the
  // service was initializing, there will be no installation attempt.
  ExpectNoInstallation(temporary_custom_subscription);
  // |installed_custom_subscription| and |installed_built_in_subscription| are
  // already installed. They don't need to be installed.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(std::vector<scoped_refptr<Subscription>>{
          MakeMockSubscription(installed_custom_subscription,
                               InstallationState::Installed),
          MakeMockSubscription(unexpected_installed_subscription,
                               InstallationState::Installed),
          MakeMockSubscription(installed_built_in_subscription,
                               InstallationState::Installed)}));

  ExpectNoInstallation(installed_custom_subscription);
  ExpectNoInstallation(installed_built_in_subscription);

  // |kRecommendedSubscription| will be installed.
  ExpectInstallationTriggered(kRecommendedSubscription);
  EXPECT_CALL(observer, OnSubscriptionUpdated(kRecommendedSubscription));

  // |new_built_in_subscription| isn't available, so it will be installed.
  ExpectInstallationTriggered(new_built_in_subscription);
  EXPECT_CALL(observer, OnSubscriptionUpdated(new_built_in_subscription));

  // |unexpected_installed_subscription| is installed but not accounted for in
  // Prefs, it will be uninstalled.
  EXPECT_CALL(subscription_service_,
              UninstallSubscription(unexpected_installed_subscription));
  EXPECT_CALL(observer,
              OnSubscriptionUpdated(unexpected_installed_subscription));

  // Since Acceptable Ads are also enabled, but weren't reported as installed,
  // the Acceptable Ads subscription is also downloaded.
  ExpectInstallationTriggered(AcceptableAdsUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AcceptableAdsUrl()));

  // Now, SubscriptionService becomes initialized.
  // Run the delayed task posted by SynchronizeWithPrefsWhenPossible();
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(delayed_task).Run();

  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest, SetCustomFilters) {
  std::string custom_filter = "test.com/ad";
  std::vector<std::string> with_filter{custom_filter};
  std::vector<std::string> empty;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  EXPECT_EQ(empty, testee_->GetCustomFilters());
  EXPECT_CALL(subscription_service_, SetCustomFilters(with_filter));
  testee_->AddCustomFilter(custom_filter);
  EXPECT_EQ(with_filter, testee_->GetCustomFilters());
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->AddCustomFilter(custom_filter);
  EXPECT_EQ(with_filter, testee_->GetCustomFilters());
  EXPECT_CALL(subscription_service_, SetCustomFilters(empty));
  testee_->RemoveCustomFilter(custom_filter);
  EXPECT_EQ(empty, testee_->GetCustomFilters());
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->RemoveCustomFilter(custom_filter);
  EXPECT_EQ(empty, testee_->GetCustomFilters());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAdd) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  EXPECT_EQ(empty, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(with_domain_filter_google));
  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAddEmpty) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->AddAllowedDomain("");
  EXPECT_EQ(empty, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAddMultiple) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(with_domain_filter_google_and_facebook));
  testee_->AddAllowedDomain(domain_facebook);
  EXPECT_EQ(with_domain_google_and_facebook, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAddExisting) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAddUppercase) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  EXPECT_EQ(empty, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(with_domain_filter_google));
  testee_->AddAllowedDomain(domain_google_upper);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainAddExistingUppercase) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->AddAllowedDomain(domain_google_upper);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainRemove) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_, SetCustomFilters(empty));
  testee_->RemoveAllowedDomain(domain_google);
  EXPECT_EQ(empty, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainRemoveMultiple) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  testee_->AddAllowedDomain(domain_facebook);
  EXPECT_EQ(with_domain_google_and_facebook, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(with_domain_filter_facebook));
  testee_->RemoveAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_facebook, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainRemoveEmpty) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->RemoveAllowedDomain("");
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainRemoveNotExisting) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_, SetCustomFilters(testing::_)).Times(0);
  testee_->RemoveAllowedDomain(domain_facebook);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, AllowedDomainRemoveUppercase) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_domain_google, testee_->GetAllowedDomains());
  EXPECT_CALL(subscription_service_, SetCustomFilters(empty));
  testee_->RemoveAllowedDomain(domain_google_upper);
  EXPECT_EQ(empty, testee_->GetAllowedDomains());
}

TEST_F(AdblockControllerImplTest, SynchronizeCustomFiltersAndAllowedDomains) {
  using namespace allowed_domain_test_data;
  std::string custom_filter = "test.com/ad";
  std::vector<std::string> with_custom_and_allowed_domain_filter{
      custom_filter, domain_filter_google};
  std::vector<std::string> with_custom_filter{custom_filter};
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(subscription_service_, SetCustomFilters(with_custom_filter));
  testee_->AddCustomFilter(custom_filter);
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(with_custom_and_allowed_domain_filter));
  testee_->AddAllowedDomain(domain_google);
  EXPECT_EQ(with_custom_filter, testee_->GetCustomFilters());
}

TEST_F(
    AdblockControllerImplTest,
    SynchronizeCustomFiltersAndAllowedDomains_NoCustomFiltersOrAllowedDomains) {
  using namespace allowed_domain_test_data;
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(subscription_service_, SetCustomFilters(empty));
  testee_->SynchronizeWithPrefsWhenPossible();
}

TEST_F(AdblockControllerImplTest,
       SynchronizeCustomFiltersWhenServiceAvailable) {
  std::string custom_filter = "test.com/aaa";
  std::vector<base::OnceClosure> delayed_tasks;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly([&](base::OnceClosure task) {
        delayed_tasks.push_back(std::move(task));
      });
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(std::vector<std::string>{custom_filter}))
      .Times(0);
  testee_->SynchronizeWithPrefsWhenPossible();
  testee_->AddCustomFilter(custom_filter);

  ASSERT_EQ(delayed_tasks.size(), 1u);
  EXPECT_CALL(subscription_service_,
              SetCustomFilters(std::vector<std::string>{custom_filter}));
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  // should call OnSubscriptionServiceReady
  for (auto& task : delayed_tasks) {
    std::move(task).Run();
  }
}

TEST_F(AdblockControllerImplTest,
       InstallLanguageBasedRecommendationAndAntiCvOnFirstRun) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));

  // SynchronizeWithPrefsWhenPossible() will schedule a delayed task.
  base::OnceClosure delayed_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly(
          [&](base::OnceClosure task) { delayed_task = std::move(task); });
  MockObserver observer;
  testee_->AddObserver(&observer);
  testee_->SynchronizeWithPrefsWhenPossible();

  // There are no subscriptions on disk.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(std::vector<scoped_refptr<Subscription>>{}));

  // Default prefs state indicates a first run situation. Since there aren't any
  // pre-existing subscriptions in prefs, we will install a language-based
  // recommendation.
  ExpectInstallationTriggered(kRecommendedSubscription);
  EXPECT_CALL(observer, OnSubscriptionUpdated(kRecommendedSubscription));
  // By default, Acceptable Ads are enabled, so they will be installed as well.
  ExpectInstallationTriggered(AcceptableAdsUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AcceptableAdsUrl()));
  // Anti-CV filter list is installed as well, to enable snippets.
  ExpectInstallationTriggered(AntiCVUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AntiCVUrl()));

  // Now, SubscriptionService becomes initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(delayed_task).Run();

  // After synchronization is complete, the first-run flag is disabled.
  EXPECT_FALSE(
      pref_service_.GetBoolean(prefs::kInstallFirstStartSubscriptions));
  // Controller has the recommended subscription installed.
  const std::vector<scoped_refptr<Subscription>> installed_subscriptions = {
      MakeMockSubscription(kRecommendedSubscription),
      MakeMockSubscription(AntiCVUrl()),
      MakeMockSubscription(AcceptableAdsUrl())};
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(installed_subscriptions));
  EXPECT_THAT(testee_->GetSelectedBuiltInSubscriptions(),
              testing::UnorderedElementsAreArray(installed_subscriptions));

  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest,
       InstallDefaultRecommendationForNonMatchingLanguage) {
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));

  // SynchronizeWithPrefsWhenPossible() will schedule a delayed task.
  base::OnceClosure delayed_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly(
          [&](base::OnceClosure task) { delayed_task = std::move(task); });
  MockObserver observer;
  pref_service_.ClearPref(prefs::kAdblockSubscriptions);
  RecreateController("th-TH");
  testee_->AddObserver(&observer);
  testee_->SynchronizeWithPrefsWhenPossible();

  // There are no subscriptions on disk.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(std::vector<scoped_refptr<Subscription>>{}));

  // Default prefs state indicates a first run situation. Since there aren't any
  // pre-existing subscriptions in prefs, we will install a default
  // subscription because we don't find a language-based recommendation.
  ExpectInstallationTriggered(DefaultSubscriptionUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(DefaultSubscriptionUrl()));

  // By default, Acceptable Ads are enabled, so they will be installed as well.
  ExpectInstallationTriggered(AcceptableAdsUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AcceptableAdsUrl()));
  // Anti-CV filter list is installed as well, to enable snippets.
  ExpectInstallationTriggered(AntiCVUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AntiCVUrl()));

  // Now, SubscriptionService becomes initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(delayed_task).Run();

  testee_->RemoveObserver(&observer);
}

TEST_F(AdblockControllerImplTest, InstallAntiCvOnFirstRunIfMigrating) {
  RecreateController("pl-PL");
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(false));

  // SynchronizeWithPrefsWhenPossible() will schedule a delayed task.
  base::OnceClosure delayed_task;
  EXPECT_CALL(subscription_service_, RunWhenInitialized(testing::_))
      .WillRepeatedly(
          [&](base::OnceClosure task) { delayed_task = std::move(task); });
  MockObserver observer;
  testee_->AddObserver(&observer);
  testee_->SynchronizeWithPrefsWhenPossible();

  // There are no subscriptions on disk.
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillOnce(testing::Return(std::vector<scoped_refptr<Subscription>>{}));

  // Acceptable Ads were disabled in the old eyeo Chromium SDK version.
  pref_service_.SetBoolean(prefs::kEnableAcceptableAds, false);

  // Previous version of eyeo Chromium SDK had a specific subscription
  // installed. Note that this is a different subscription from the recommended
  // one for the user's locale. It will be installed on first run in the current
  // version.
  const GURL kMigratedSubscription{
      "https://easylist-downloads.adblockplus.org/"
      "easylistgermany+easylist.txt"};
  subscriptions_pref_.SetValue({kMigratedSubscription.spec()});

  ExpectInstallationTriggered(kMigratedSubscription);
  EXPECT_CALL(observer, OnSubscriptionUpdated(kMigratedSubscription));
  // Anti-CV filter list is installed as well, to enable snippets.
  ExpectInstallationTriggered(AntiCVUrl());
  EXPECT_CALL(observer, OnSubscriptionUpdated(AntiCVUrl()));
  // Acceptable Ads is not installed since it was disabled in old version.
  ExpectNoInstallation(AcceptableAdsUrl());

  // Now, SubscriptionService becomes initialized.
  EXPECT_CALL(subscription_service_, IsInitialized())
      .WillRepeatedly(testing::Return(true));
  std::move(delayed_task).Run();

  // After synchronization is complete, the first-run flag is disabled.
  EXPECT_FALSE(
      pref_service_.GetBoolean(prefs::kInstallFirstStartSubscriptions));
  // Controller has the recommended subscription installed.
  const std::vector<scoped_refptr<Subscription>> installed_subscriptions = {
      MakeMockSubscription(kMigratedSubscription),
      MakeMockSubscription(AntiCVUrl())};
  EXPECT_CALL(subscription_service_, GetCurrentSubscriptions())
      .WillRepeatedly(testing::Return(installed_subscriptions));
  EXPECT_THAT(testee_->GetSelectedBuiltInSubscriptions(),
              testing::UnorderedElementsAreArray(installed_subscriptions));

  testee_->RemoveObserver(&observer);
}

}  // namespace adblock

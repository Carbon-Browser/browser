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

#include "components/adblock/core/subscription/subscription_service_impl.h"

#include <iterator>
#include <memory>
#include <tuple>
#include <vector>

#include "absl/types/optional.h"
#include "base/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_piece_forward.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/common/header_filter_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/converter/converter.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription.h"
#include "components/adblock/core/subscription/test/mock_subscription_downloader.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/prefs/testing_pref_service.h"
#include "gmock/gmock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {

class FakePersistentStorage final : public SubscriptionPersistentStorage {
 public:
  MOCK_METHOD(void, MockLoadSubscriptions, ());

  void LoadSubscriptions(LoadCallback on_initialized) final {
    on_initialized_ = std::move(on_initialized);
    MockLoadSubscriptions();
  }

  void StoreSubscription(std::unique_ptr<FlatbufferData> raw_data,
                         StoreCallback on_finished) final {
    store_subscription_calls_.emplace_back(std::move(raw_data),
                                           std::move(on_finished));
  }
  void RemoveSubscription(
      scoped_refptr<InstalledSubscription> subscription) final {
    remove_subscription_calls_.push_back(std::move(subscription));
  }

  LoadCallback on_initialized_;
  std::vector<std::pair<std::unique_ptr<FlatbufferData>, StoreCallback>>
      store_subscription_calls_;
  std::vector<scoped_refptr<InstalledSubscription>> remove_subscription_calls_;
};

class FakeBuffer final : public FlatbufferData {
 public:
  const uint8_t* data() const final { return nullptr; }
  size_t size() const final { return 0u; }
};

class FakeSubscription final : public InstalledSubscription {
 public:
  explicit FakeSubscription(
      std::string name,
      InstallationState state = InstallationState::Installed)
      : name_(std::move(name)), state_(state) {}

  GURL GetSourceUrl() const final {
    if (GURL{name_}.is_valid())
      return GURL{name_};
    return GURL{"https://easylist-downloads.adblockplus.org/" + name_};
  }
  std::string GetTitle() const final { return name_; }

  std::string GetCurrentVersion() const final { return name_; }

  InstallationState GetInstallationState() const final { return state_; }

  base::Time GetInstallationTime() const final { return base::Time(); }

  base::TimeDelta GetExpirationInterval() const final { return base::Days(5); }

  bool HasUrlFilter(const GURL& url,
                    const std::string& document_domain,
                    ContentType type,
                    const SiteKey& sitekey,
                    FilterCategory category) const final {
    return false;
  }
  bool HasPopupFilter(const GURL& url,
                      const GURL& opener_url,
                      const SiteKey& sitekey,
                      FilterCategory category) const final {
    return false;
  }
  bool HasSpecialFilter(SpecialFilterType type,
                        const GURL& url,
                        const std::string& document_domain,
                        const SiteKey& sitekey) const final {
    return false;
  }
  absl::optional<base::StringPiece> FindCspFilter(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const final {
    return absl::nullopt;
  }
  absl::optional<base::StringPiece> FindRewriteFilter(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const final {
    return absl::nullopt;
  }
  void FindHeaderFilters(const GURL& url,
                         ContentType type,
                         const std::string& document_domain,
                         FilterCategory category,
                         std::set<HeaderFilterData>& results) const final {}
  Selectors GetElemhideSelectors(const GURL& url,
                                 bool domain_specific) const final {
    Selectors result;
    result.elemhide_selectors = {name_};
    return result;
  }
  Selectors GetElemhideEmulationSelectors(const GURL& url) const final {
    return {};
  }

  std::vector<Snippet> MatchSnippets(
      const std::string& document_domain) const final {
    return {};
  }

  void MarkForPermanentRemoval() final {}

  std::string name_;
  InstallationState state_;

 private:
  ~FakeSubscription() final = default;
};

class MockPreloadedSubscriptionProvider : public PreloadedSubscriptionProvider {
 public:
  MOCK_METHOD(void,
              UpdateSubscriptions,
              (std::vector<GURL> installed_subscriptions,
               std::vector<GURL> pending_subscriptions),
              (override));
  MOCK_METHOD(std::vector<scoped_refptr<InstalledSubscription>>,
              GetCurrentPreloadedSubscriptions,
              (),
              (override, const));
};

}  // namespace

class AdblockSubscriptionServiceImplTest : public testing::Test {
 public:
  AdblockSubscriptionServiceImplTest() {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    // Set adblock disabled as default
    pref_service_.SetBoolean(prefs::kEnableAdblock, false);
    auto storage = std::make_unique<FakePersistentStorage>();
    storage_ = storage.get();
    auto downloader = std::make_unique<MockSubscriptionDownloader>();
    downloader_ = downloader.get();
    auto preloaded_subscription_provider =
        std::make_unique<MockPreloadedSubscriptionProvider>();
    preloaded_subscription_provider_ = preloaded_subscription_provider.get();

    service_ = std::make_unique<SubscriptionServiceImpl>(
        &pref_service_, std::move(storage), std::move(downloader),
        std::move(preloaded_subscription_provider),
        base::BindRepeating(
            &AdblockSubscriptionServiceImplTest::CreateCustomSubscription,
            base::Unretained(this)),
        &persistent_metadata_);
  }

  scoped_refptr<InstalledSubscription> CreateCustomSubscription(
      const std::vector<std::string>& filters) {
    return base::MakeRefCounted<FakeSubscription>(CustomFiltersUrl().spec());
  }

  void AddSubscription(
      scoped_refptr<InstalledSubscription> subscription,
      SubscriptionDownloader::RetryPolicy expected_retry_policy =
          SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded) {
    // The downloader will be called to fetch the raw_data for subscription.
    EXPECT_CALL(*downloader_, StartDownload(subscription->GetSourceUrl(),
                                            expected_retry_policy, testing::_))
        .WillOnce([](const GURL&, SubscriptionDownloader::RetryPolicy,
                     base::OnceCallback<void(std::unique_ptr<FlatbufferData>)>
                         callback) {
          // The downloader responds by running the callback with a new
          // buffer, simulating a successful download.
          std::move(callback).Run(std::make_unique<FakeBuffer>());
        });
    // DownloadAndInstallSubscription gets called.
    base::MockCallback<SubscriptionService::InstallationCallback> on_finished;
    service_->DownloadAndInstallSubscription(subscription->GetSourceUrl(),
                                             on_finished.Get());
    // Storage was asked to store the buffer provided by downloader.
    ASSERT_EQ(storage_->store_subscription_calls_.size(), 1u);
    EXPECT_TRUE(storage_->store_subscription_calls_[0].first);
    // Storage runs the callback provided by SubscriptionService to indicate
    // store succeeded. This triggers the final InstallationCallback to run with
    // a successful result.
    EXPECT_CALL(on_finished, Run(true));
    std::move(storage_->store_subscription_calls_[0].second).Run(subscription);
    storage_->store_subscription_calls_.clear();
  }

  void RemoveSubscription(scoped_refptr<FakeSubscription> subscription) {
    // Simulates a single call to UninstallSubscription that forwards the
    // subscription to storage_ for removal.
    EXPECT_CALL(persistent_metadata_,
                RemoveMetadata(subscription->GetSourceUrl()));
    service_->UninstallSubscription(subscription->GetSourceUrl());
    ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
    EXPECT_EQ(storage_->remove_subscription_calls_[0], subscription);
    storage_->remove_subscription_calls_.clear();
  }

  void InitializeServiceWithNoSubscriptions() {
    pref_service_.SetBoolean(prefs::kEnableAdblock, true);
    std::move(storage_->on_initialized_).Run({});
  }

  const GURL kRequestUrl{"https://domain.com/resource.jpg"};
  const GURL kParentUrl{"https://domain.com"};
  const SiteKey kSitekey{"abc"};

  FakePersistentStorage* storage_;
  MockPreloadedSubscriptionProvider* preloaded_subscription_provider_;
  MockSubscriptionDownloader* downloader_;
  MockSubscriptionPersistentMetadata persistent_metadata_;
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SubscriptionServiceImpl> service_;
};

TEST_F(AdblockSubscriptionServiceImplTest, InitializationScheduledImmediately) {
  // Adblocking is enabled by default
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  // Service has called Initialize() on persistent storage.
  ASSERT_TRUE(storage_->on_initialized_);
  // Service is not initialized until storage finishes initialization.
  EXPECT_FALSE(service_->IsInitialized());
  // Tasks can be scheduled to execute after initialization.
  testing::InSequence seq;
  base::MockOnceClosure task1;
  base::MockOnceClosure task2;
  EXPECT_CALL(task1, Run());
  EXPECT_CALL(task2, Run());
  service_->RunWhenInitialized(task1.Get());
  service_->RunWhenInitialized(task2.Get());

  // Storage completes initialization, loads two subscriptions.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  std::move(storage_->on_initialized_)
      .Run({fake_subscription1, fake_subscription2});

  // Service is now initialized:
  EXPECT_TRUE(service_->IsInitialized());
  // The subscriptions provided by storage are visible.
  EXPECT_THAT(
      service_->GetCurrentSubscriptions(),
      testing::UnorderedElementsAre(fake_subscription1, fake_subscription2));
}

TEST_F(
    AdblockSubscriptionServiceImplTest,
    SubscriptionServiceGetsInitializedWhenEnablingAdblockingForTheFirstTime) {
  // Service is not initialized:
  EXPECT_FALSE(service_->IsInitialized());
  // Subscriptions get loaded when the service gets initialized
  EXPECT_CALL(*storage_, MockLoadSubscriptions()).Times(1);
  // Enabled adblocking.
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  std::move(storage_->on_initialized_).Run({});
  // Service is now initialized:
  EXPECT_TRUE(service_->IsInitialized());
}

TEST_F(AdblockSubscriptionServiceImplTest, SubscriptionsGetLoadedOnlyOnce) {
  // Storage has no initial subscriptions:
  InitializeServiceWithNoSubscriptions();
  // Subscriptions should not be reloaded when re-enabling adblock.
  EXPECT_CALL(*storage_, MockLoadSubscriptions()).Times(0);
  // Toggle adblock enabled:
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
}

TEST_F(AdblockSubscriptionServiceImplTest, AddSubscription) {
  // Storage has no initial subscriptions:
  InitializeServiceWithNoSubscriptions();

  // When storage calls its callback, the provided subscription is added to the
  // service and |on_finished| is triggered with the parsed URL.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  AddSubscription(fake_subscription1);

  // Added subscription is reflected in |GetCurrentSubscriptions|.
  EXPECT_THAT(service_->GetCurrentSubscriptions(),
              testing::ElementsAre(fake_subscription1));

  // The snapshot is a SubscriptionCollection that queries the added
  // subscription. We can check whether FakeSubscription's title appears in
  // Elemhide selectors.
  auto subscription_collection = service_->GetCurrentSnapshot();
  auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_THAT(selectors, testing::ElementsAre(fake_subscription1->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest,
       AddMultipleSubscriptionsAndRemoveOne) {
  InitializeServiceWithNoSubscriptions();

  // Add 3 subscriptions.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  auto fake_subscription3 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription3");
  AddSubscription(fake_subscription1);
  AddSubscription(fake_subscription2);
  AddSubscription(fake_subscription3);

  // Remove middle one.
  RemoveSubscription(fake_subscription2);

  // Two remaining subscription are reflected in |GetInstalledSubscriptions|.
  EXPECT_THAT(
      service_->GetCurrentSubscriptions(),
      testing::UnorderedElementsAre(fake_subscription1, fake_subscription3));

  // The snapshot is a SubscriptionCollection that queries the remaining
  // subscriptions. We can check whether FakeSubscription's title appears in
  // Elemhide selectors.
  auto subscription_collection = service_->GetCurrentSnapshot();
  auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription3->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest,
       SnapshotNotAffectedByFutureAddition) {
  InitializeServiceWithNoSubscriptions();
  // Add one subscription
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  AddSubscription(fake_subscription1);

  // Take snapshot now.
  auto subscription_collection = service_->GetCurrentSnapshot();

  // Add new subscription after snapshot.
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  AddSubscription(fake_subscription2);

  // Snapshot only contains the first subscription.
  auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest, SnapshotNotAffectedByFutureRemoval) {
  InitializeServiceWithNoSubscriptions();
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription2");
  AddSubscription(fake_subscription1);
  AddSubscription(fake_subscription2);

  // Take snapshot now.
  auto subscription_collection = service_->GetCurrentSnapshot();

  // Remove second subscription.
  RemoveSubscription(fake_subscription2);

  // Snapshot still contains both subscriptions.
  auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription2->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest, UpgradeExistingSubscription) {
  InitializeServiceWithNoSubscriptions();
  auto existing_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  AddSubscription(existing_subscription);

  auto upgraded_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  // Upgraded subscription has the same URL as |existing_subscription|.
  ASSERT_EQ(existing_subscription->GetSourceUrl(),
            upgraded_subscription->GetSourceUrl());
  // The request to download the new subscription will have a current version of
  // the existing subscription.
  AddSubscription(upgraded_subscription,
                  SubscriptionDownloader::RetryPolicy::DoNotRetry);

  // Service noticed we're updating an existing subscription, it uninstalls
  // the old version.
  ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
  EXPECT_EQ(storage_->remove_subscription_calls_[0], existing_subscription);

  // Current subscriptions contains only one URL.
  EXPECT_THAT(service_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(upgraded_subscription));

  // Selectors returned by snapshot contain entry from only one subscription
  // (it would be duplicated if both subscriptions were present).
  const auto subscription_collection = service_->GetCurrentSnapshot();
  const auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_EQ(selectors, std::vector<base::StringPiece>{"subscription"});
}

TEST_F(AdblockSubscriptionServiceImplTest, RemoveDuplicatesDuringInitialLoad) {
  // Adblocking is enabled by default
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  // Storage returns 3 subscriptions in initial load, however there is a
  // duplicate, due to a race condition or corruption.
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  auto fake_subscription2 =
      base::MakeRefCounted<FakeSubscription>("unique_subscription");
  auto fake_subscription3 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  ASSERT_EQ(fake_subscription1->GetSourceUrl(),
            fake_subscription3->GetSourceUrl());

  std::move(storage_->on_initialized_)
      .Run({fake_subscription1, fake_subscription2, fake_subscription3});

  // Service noticed one subscription is duplicated and it removes one instance
  // - it is unspecified which.
  ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
  EXPECT_EQ(storage_->remove_subscription_calls_[0]->GetSourceUrl(),
            fake_subscription1->GetSourceUrl());

  // Installed subscriptions do not contain duplicates.
  std::vector<GURL> current_subscriptions_urls;
  base::ranges::transform(service_->GetCurrentSubscriptions(),
                          std::back_inserter(current_subscriptions_urls),
                          [](const auto& sub) { return sub->GetSourceUrl(); });
  EXPECT_THAT(
      current_subscriptions_urls,
      testing::UnorderedElementsAre(fake_subscription1->GetSourceUrl(),
                                    fake_subscription2->GetSourceUrl()));

  // Selectors returned by snapshot do not contain duplicates.
  const auto subscription_collection = service_->GetCurrentSnapshot();
  const auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_EQ(selectors.size(), 2u);
  EXPECT_THAT(selectors,
              testing::UnorderedElementsAre(fake_subscription1->GetTitle(),
                                            fake_subscription2->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest,
       CancellingInstallationDuringDownload_WithPreloadedFallback) {
  // Storage has no initial subscriptions:
  InitializeServiceWithNoSubscriptions();

  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  const GURL& url = fake_subscription1->GetSourceUrl();

  // SubscriptionDownloader will be called to fetch the subscription. We will
  // trigger the response later, after cancelling installation.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(url, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            download_completed_callback = std::move(callback);
          });

  // There is a preloaded fallback available for this URL.
  auto preloaded_subscription = base::MakeRefCounted<FakeSubscription>(
      "fake_subscription1", Subscription::InstallationState::Preloaded);
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{
              preloaded_subscription}));

  // Start installation.
  base::MockCallback<SubscriptionService::InstallationCallback> on_finished;
  service_->DownloadAndInstallSubscription(url, on_finished.Get());

  // We should see the preloaded fallback in GetCurrentSubscriptions().
  EXPECT_THAT(service_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(preloaded_subscription));

  // We now uninstall the subscription, this should cancel the download.
  // The installation callback is notified with a failure indication.
  EXPECT_CALL(on_finished, Run(false));
  // The downloader is told to cancel the download.
  EXPECT_CALL(*downloader_, CancelDownload(url));
  service_->UninstallSubscription(url);

  // The subscription is no longer listed.
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{}));
  EXPECT_TRUE(service_->GetCurrentSubscriptions().empty());

  // Even when the download callback delivers the FakeBuffer, it will not
  // be sent to storage.
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  // There are no attempts to store the buffer received from Downloader.
  EXPECT_TRUE(storage_->store_subscription_calls_.empty());
  EXPECT_TRUE(service_->GetCurrentSubscriptions().empty());
}

TEST_F(AdblockSubscriptionServiceImplTest,
       CancellingInstallationDuringStorage_NoFallback) {
  // Storage has no initial subscriptions:
  InitializeServiceWithNoSubscriptions();

  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("fake_subscription1");
  const GURL& url = fake_subscription1->GetSourceUrl();

  // There are no preloaded fallback available for this URL.
  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillRepeatedly(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{}));
  // SubscriptionDownloader will be called to fetch the subscription. It is
  // immediately successful.
  EXPECT_CALL(*downloader_, StartDownload(url, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            std::move(callback).Run(std::make_unique<FakeBuffer>());
          });

  // Start installation.
  base::MockCallback<SubscriptionService::InstallationCallback> on_finished;
  service_->DownloadAndInstallSubscription(url, on_finished.Get());

  // The downloader immediately returned a FakeBuffer, it should have been sent
  // to storage.
  ASSERT_EQ(storage_->store_subscription_calls_.size(), 1u);

  // We should see the ongoing installation in GetCurrentSubscriptions().
  const auto current_subscriptions = service_->GetCurrentSubscriptions();
  ASSERT_EQ(current_subscriptions.size(), 1u);
  EXPECT_EQ(current_subscriptions[0]->GetSourceUrl(), url);
  EXPECT_EQ(current_subscriptions[0]->GetInstallationState(),
            Subscription::InstallationState::Installing);

  // We now uninstall the subscription, this should cancel the installation.
  // The installation callback is notified with a failure indication.
  EXPECT_CALL(on_finished, Run(false));
  service_->UninstallSubscription(url);

  // The subscription is no longer listed.
  EXPECT_TRUE(service_->GetCurrentSubscriptions().empty());

  // Even when the storage callback delivers the Subscription, it will not
  // be installed in SubscriptionService.
  std::move(storage_->store_subscription_calls_[0].second)
      .Run(fake_subscription1);
  // In fact, the subscription will be scheduled for removal from storage, it
  // is not desired.
  ASSERT_EQ(storage_->remove_subscription_calls_.size(), 1u);
  EXPECT_EQ(storage_->remove_subscription_calls_[0], fake_subscription1);

  EXPECT_TRUE(service_->GetCurrentSubscriptions().empty());
}

TEST_F(AdblockSubscriptionServiceImplTest, CustomFilterIsAdded) {
  auto fake_subscription1 =
      base::MakeRefCounted<FakeSubscription>("subscription");
  InitializeServiceWithNoSubscriptions();
  AddSubscription(fake_subscription1);
  std::vector<std::string> filters{"test"};
  service_->SetCustomFilters(filters);

  // The in-memory subscription containing the custom filter is not reported
  // among current subscriptions, only the subscription added by client is.
  EXPECT_THAT(service_->GetCurrentSubscriptions(),
              testing::UnorderedElementsAre(fake_subscription1));

  // However, the SubscriptionCollection *does* get the custom filter
  // subscription.
  auto subscription_collection = service_->GetCurrentSnapshot();
  auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_THAT(selectors, testing::UnorderedElementsAre(
                             CustomFiltersUrl().spec(), "subscription"));
}

TEST_F(AdblockSubscriptionServiceImplTest, PingStoresAAversion) {
  const std::string version("202107210821");

  InitializeServiceWithNoSubscriptions();
  SubscriptionDownloader::HeadRequestCallback download_completed_callback;
  EXPECT_CALL(*downloader_, DoHeadRequest(AcceptableAdsUrl(), testing::_))
      .WillOnce([&](const GURL&,
                    SubscriptionDownloader::HeadRequestCallback callback) {
        download_completed_callback = std::move(callback);
      });

  base::MockCallback<SubscriptionService::InstallationCallback> on_finished;
  service_->PingAcceptableAds(on_finished.Get());
  // Version delivered by HEAD response is stored persistently in metadata.
  // It will be read by SubscriptionDownloader in next DoHeadRequest().
  EXPECT_CALL(persistent_metadata_, SetVersion(AcceptableAdsUrl(), version));
  // The next ping should happen in a day.
  EXPECT_CALL(persistent_metadata_,
              SetExpirationInterval(AcceptableAdsUrl(), base::Days(1)));
  std::move(download_completed_callback).Run(version);
}

TEST_F(AdblockSubscriptionServiceImplTest,
       PreloadedSubscriptionProviderUpdatedDuringChanges) {
  testing::InSequence sequence;
  // Initially, update about no installed and no pending subscriptions.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}));
  InitializeServiceWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            download_completed_callback = std::move(callback);
          });
  service_->DownloadAndInstallSubscription(first_subscription->GetSourceUrl(),
                                           base::DoNothing());
  // When download completes, update the provider about new installed
  // subscription, and no pending subscriptions.
  EXPECT_CALL(
      *preloaded_subscription_provider_,
      UpdateSubscriptions(std::vector<GURL>{first_subscription->GetSourceUrl()},
                          std::vector<GURL>{}));

  // Download completes.
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  std::move(storage_->store_subscription_calls_.back().second)
      .Run(first_subscription);

  // Second subscription added.
  auto second_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription2");
  // Provider updated with both the old installed subscription and the new
  // ongoing download.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{first_subscription->GetSourceUrl()},
                  std::vector<GURL>{second_subscription->GetSourceUrl()}));

  // Second download starts.
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            download_completed_callback = std::move(callback);
          });
  service_->DownloadAndInstallSubscription(second_subscription->GetSourceUrl(),
                                           base::DoNothing());

  // When second download completes, provider has two installed and zero pending
  // subscriptions.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{first_subscription->GetSourceUrl(),
                                    second_subscription->GetSourceUrl()},
                  std::vector<GURL>{}));
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
  std::move(storage_->store_subscription_calls_.back().second)
      .Run(second_subscription);

  // First subscription is uninstalled, provider informed about new state
  // containing only the second subscription.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{second_subscription->GetSourceUrl()},
                  std::vector<GURL>{}));
  service_->UninstallSubscription(first_subscription->GetSourceUrl());
}

TEST_F(AdblockSubscriptionServiceImplTest,
       PreloadedSubscriptionProviderUpdatedDuringFailedDownload) {
  testing::InSequence sequence;
  // Initially, update about no installed and no pending subscriptions.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}));
  InitializeServiceWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            download_completed_callback = std::move(callback);
          });
  service_->DownloadAndInstallSubscription(first_subscription->GetSourceUrl(),
                                           base::DoNothing());
  // When download fails, inform the provider about returning to previous state.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}));

  // Download fails.
  std::move(download_completed_callback).Run(nullptr);
}

TEST_F(AdblockSubscriptionServiceImplTest,
       PreloadedSubscriptionProviderUpdatedWhenInstallationCancelled) {
  testing::InSequence sequence;
  // Initially, update about no installed and no pending subscriptions.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}));
  InitializeServiceWithNoSubscriptions();
  // When starting a download, inform provider about new pending subscription.
  auto first_subscription =
      base::MakeRefCounted<FakeSubscription>("subscription");
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(
                  std::vector<GURL>{},
                  std::vector<GURL>{first_subscription->GetSourceUrl()}));

  // Start download.
  SubscriptionDownloader::DownloadCompletedCallback download_completed_callback;
  EXPECT_CALL(*downloader_, StartDownload(testing::_, testing::_, testing::_))
      .WillOnce(
          [&](const GURL&, SubscriptionDownloader::RetryPolicy,
              SubscriptionDownloader::DownloadCompletedCallback callback) {
            download_completed_callback = std::move(callback);
          });
  service_->DownloadAndInstallSubscription(first_subscription->GetSourceUrl(),
                                           base::DoNothing());
  // When installation is cancelled, inform the provider about returning to
  // previous state.
  EXPECT_CALL(*preloaded_subscription_provider_,
              UpdateSubscriptions(std::vector<GURL>{}, std::vector<GURL>{}))
      .Times(testing::AtLeast(1));
  service_->UninstallSubscription(first_subscription->GetSourceUrl());

  // Download completes, but the installation was cancelled in the mean time.
  std::move(download_completed_callback).Run(std::make_unique<FakeBuffer>());
}

TEST_F(AdblockSubscriptionServiceImplTest,
       PreloadedSubscriptionProviderConsultedForSnapshot) {
  auto subscription_in_service =
      base::MakeRefCounted<FakeSubscription>("subscription_in_service");
  auto preloaded_subscription = base::MakeRefCounted<FakeSubscription>(
      "preloaded_subscription", Subscription::InstallationState::Preloaded);
  InitializeServiceWithNoSubscriptions();
  AddSubscription(subscription_in_service);

  EXPECT_CALL(*preloaded_subscription_provider_,
              GetCurrentPreloadedSubscriptions())
      .WillOnce(
          testing::Return(std::vector<scoped_refptr<InstalledSubscription>>{
              preloaded_subscription}));

  // Snapshot provides both the subscription in service and the preloaded
  // subscription returned by provider.
  const auto subscription_collection = service_->GetCurrentSnapshot();
  const auto selectors =
      subscription_collection->GetElementHideSelectors(GURL(), {}, SiteKey());
  EXPECT_EQ(selectors.size(), 2u);
  EXPECT_THAT(selectors, testing::UnorderedElementsAre(
                             subscription_in_service->GetTitle(),
                             preloaded_subscription->GetTitle()));
}

TEST_F(AdblockSubscriptionServiceImplTest, AcceptableAdsMetadataRetained) {
  auto aa_subscription =
      base::MakeRefCounted<FakeSubscription>("exceptionrules.txt");
  auto easylist_subscription =
      base::MakeRefCounted<FakeSubscription>("easylist.txt");
  InitializeServiceWithNoSubscriptions();
  AddSubscription(aa_subscription);
  AddSubscription(easylist_subscription);

  // Removing EasyList clears the subscription's metadata.
  EXPECT_CALL(persistent_metadata_,
              RemoveMetadata(easylist_subscription->GetSourceUrl()));
  service_->UninstallSubscription(easylist_subscription->GetSourceUrl());

  // Removing the Acceptable Ads subscription retains metadata, in order to
  // allow sending continued HEAD-only update-like requests with consistent
  // expiry date.
  EXPECT_CALL(persistent_metadata_,
              RemoveMetadata(aa_subscription->GetSourceUrl()))
      .Times(0);
  service_->UninstallSubscription(aa_subscription->GetSourceUrl());
}

}  // namespace adblock

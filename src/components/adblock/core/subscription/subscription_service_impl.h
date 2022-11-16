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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_

#include <memory>
#include <set>
#include <vector>

#include "base/callback.h"

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/preloaded_subscription_provider.h"
#include "components/adblock/core/subscription/subscription_downloader.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/adblock/core/subscription/subscription_persistent_storage.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"

namespace adblock {

class SubscriptionServiceImpl final : public SubscriptionService {
 public:
  using SubscriptionCreator =
      base::RepeatingCallback<scoped_refptr<InstalledSubscription>(
          const std::vector<std::string>&)>;
  SubscriptionServiceImpl(
      PrefService* prefs,
      std::unique_ptr<SubscriptionPersistentStorage> storage,
      std::unique_ptr<SubscriptionDownloader> downloader,
      std::unique_ptr<PreloadedSubscriptionProvider>
          preloaded_subscription_provider,
      const SubscriptionCreator& custom_subscription_creator,
      SubscriptionPersistentMetadata* persistent_metadata);
  ~SubscriptionServiceImpl() final;
  bool IsInitialized() const final;
  void RunWhenInitialized(base::OnceClosure task) final;
  std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions()
      const final;
  std::unique_ptr<SubscriptionCollection> GetCurrentSnapshot() const final;
  void DownloadAndInstallSubscription(const GURL& subscription_url,
                                      InstallationCallback on_finished) final;
  void PingAcceptableAds(PingCallback on_finished) final;
  void UninstallSubscription(const GURL& subscription_url) final;
  void SetCustomFilters(const std::vector<std::string>& filters) final;

 private:
  enum class Status {
    Initialized,
    LoadingSubscriptions,
    Uninitialized,
  };
  class OngoingInstallation;
  void OnSubscriptionDataAvailable(
      InstallationCallback on_finished,
      scoped_refptr<OngoingInstallation> ongoing_installation,
      std::unique_ptr<FlatbufferData> raw_data);
  void StorageInitialized(
      std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions);
  void SynchronizeWithPrefState();
  void RemoveDuplicateSubscriptions();
  void SubscriptionAddedToStorage(
      InstallationCallback on_finished,
      scoped_refptr<OngoingInstallation> ongoing_installation,
      scoped_refptr<InstalledSubscription> subscription);
  void OnHeadRequestDone(InstallationCallback on_finished,
                         const std::string version);
  void UninstallSubscriptionInternal(const GURL& subscription_url);
  void UpdatePreloadedSubscriptionProvider();
  void UpdatePreloadedSubscriptionProviderAndRun(
      InstallationCallback on_finished,
      bool success);
  std::vector<GURL> GetReadySubscriptions() const;
  std::vector<GURL> GetPendingSubscriptions() const;

  SEQUENCE_CHECKER(sequence_checker_);
  Status status_ = Status::Uninitialized;
  BooleanPrefMember enable_adblock_;
  std::vector<base::OnceClosure> queued_tasks_;
  std::unique_ptr<SubscriptionPersistentStorage> storage_;
  std::unique_ptr<SubscriptionDownloader> downloader_;
  std::unique_ptr<PreloadedSubscriptionProvider>
      preloaded_subscription_provider_;
  std::set<scoped_refptr<OngoingInstallation>> ongoing_installations_;
  std::vector<scoped_refptr<InstalledSubscription>> current_state_;
  scoped_refptr<InstalledSubscription> custom_filters_;
  SubscriptionCreator custom_subscription_creator_;
  // TODO(mpawlowski): Should not need to update metadata after DPD-1154, when
  // HEAD requests are removed. Move all use of SubscriptionPersistentMetadata
  // into SubscriptionPersistentStorage.
  SubscriptionPersistentMetadata* persistent_metadata_;
  base::WeakPtrFactory<SubscriptionServiceImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_IMPL_H_

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

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/parameter_pack.h"
#include "base/ranges/algorithm.h"
#include "base/trace_event/common/trace_event_common.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"

namespace adblock {
namespace {
constexpr base::TimeDelta kDefaultHeadRequestExpirationInterval = base::Days(1);

}  // namespace

class SubscriptionServiceImpl::OngoingInstallation final : public Subscription {
 public:
  explicit OngoingInstallation(const GURL& url) : url_(url) {}

  GURL GetSourceUrl() const final { return url_; }
  std::string GetTitle() const final { return {}; }
  std::string GetCurrentVersion() const final { return {}; }
  InstallationState GetInstallationState() const final {
    return InstallationState::Installing;
  }
  base::Time GetInstallationTime() const final { return {}; }
  base::TimeDelta GetExpirationInterval() const final { return {}; }

 private:
  friend class base::RefCountedThreadSafe<OngoingInstallation>;
  ~OngoingInstallation() final = default;
  GURL url_;
};

SubscriptionServiceImpl::SubscriptionServiceImpl(
    PrefService* prefs,
    std::unique_ptr<SubscriptionPersistentStorage> storage,
    std::unique_ptr<SubscriptionDownloader> downloader,
    std::unique_ptr<PreloadedSubscriptionProvider>
        preloaded_subscription_provider,
    const SubscriptionCreator& custom_subscription_creator,
    SubscriptionPersistentMetadata* persistent_metadata)
    : storage_(std::move(storage)),
      downloader_(std::move(downloader)),
      preloaded_subscription_provider_(
          std::move(preloaded_subscription_provider)),
      custom_subscription_creator_(custom_subscription_creator),
      persistent_metadata_(persistent_metadata) {
  enable_adblock_.Init(
      prefs::kEnableAdblock, prefs,
      base::BindRepeating(&SubscriptionServiceImpl::SynchronizeWithPrefState,
                          weak_ptr_factory_.GetWeakPtr()));
  SynchronizeWithPrefState();
}

SubscriptionServiceImpl::~SubscriptionServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool SubscriptionServiceImpl::IsInitialized() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return status_ == Status::Initialized;
}

void SubscriptionServiceImpl::RunWhenInitialized(base::OnceClosure task) {
  DCHECK(!IsInitialized());
  queued_tasks_.push_back(std::move(task));
}

std::vector<scoped_refptr<Subscription>>
SubscriptionServiceImpl::GetCurrentSubscriptions() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  // Result will contain the currently installed subscriptions:
  std::vector<scoped_refptr<Subscription>> result;
  base::ranges::copy(current_state_, std::back_inserter(result));
  // And all preloaded subscriptions:
  auto preloaded_subscriptions =
      preloaded_subscription_provider_->GetCurrentPreloadedSubscriptions();
  base::ranges::move(preloaded_subscriptions, std::back_inserter(result));
  // Also, dummy subscriptions that represent ongoing installations (unless
  // already present, in which case they'd represent updates).
  base::ranges::copy_if(
      ongoing_installations_, std::back_inserter(result),
      [&](const auto& ongoing_installation) {
        return base::ranges::find(result, ongoing_installation->GetSourceUrl(),
                                  &Subscription::GetSourceUrl) == result.end();
      });
  return result;
}

std::vector<GURL> SubscriptionServiceImpl::GetReadySubscriptions() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  std::vector<GURL> result;
  result.reserve(current_state_.size());
  base::ranges::transform(current_state_, std::back_inserter(result),
                          &Subscription::GetSourceUrl);
  return result;
}

std::vector<GURL> SubscriptionServiceImpl::GetPendingSubscriptions() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  std::vector<GURL> result;
  result.reserve(ongoing_installations_.size());
  base::ranges::transform(ongoing_installations_, std::back_inserter(result),
                          &Subscription::GetSourceUrl);
  return result;
}

std::unique_ptr<SubscriptionCollection>
SubscriptionServiceImpl::GetCurrentSnapshot() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  auto state = current_state_;
  if (custom_filters_)
    state.push_back(custom_filters_);
  auto preloaded_subscriptions =
      preloaded_subscription_provider_->GetCurrentPreloadedSubscriptions();
  std::move(preloaded_subscriptions.begin(), preloaded_subscriptions.end(),
            std::back_inserter(state));
  return std::make_unique<SubscriptionCollectionImpl>(state);
}

void SubscriptionServiceImpl::DownloadAndInstallSubscription(
    const GURL& subscription_url,
    InstallationCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  const bool is_an_update =
      base::ranges::any_of(current_state_, [&](const auto candidate) {
        return candidate->GetSourceUrl() == subscription_url;
      });

  // We do not retry downloading subscription updates, they will be retried
  // by the SubscriptionUpdater in due time anyway.
  auto retry_policy =
      is_an_update ? SubscriptionDownloader::RetryPolicy::DoNotRetry
                   : SubscriptionDownloader::RetryPolicy::RetryUntilSucceeded;

  auto ongoing_installation =
      base::MakeRefCounted<OngoingInstallation>(subscription_url);
  ongoing_installations_.insert(ongoing_installation);
  UpdatePreloadedSubscriptionProvider();

  downloader_->StartDownload(
      subscription_url, retry_policy,
      base::BindOnce(
          &SubscriptionServiceImpl::OnSubscriptionDataAvailable,
          weak_ptr_factory_.GetWeakPtr(),
          base::BindOnce(&SubscriptionServiceImpl::
                             UpdatePreloadedSubscriptionProviderAndRun,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(on_finished)),
          ongoing_installation));
}

void SubscriptionServiceImpl::PingAcceptableAds(PingCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  downloader_->DoHeadRequest(
      AcceptableAdsUrl(),
      base::BindOnce(&SubscriptionServiceImpl::OnHeadRequestDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_finished)));
}

void SubscriptionServiceImpl::UninstallSubscription(
    const GURL& subscription_url) {
  UninstallSubscriptionInternal(subscription_url);
  if (subscription_url != AcceptableAdsUrl()) {
    // Remove metadata associated with the subscription. Retain (forever)
    // metadata of the Acceptable Ads subscription even when it's no longer
    // installed, to allow continued HEAD-only pings for user counting purposes.
    persistent_metadata_->RemoveMetadata(subscription_url);
  }
  UpdatePreloadedSubscriptionProvider();
}

void SubscriptionServiceImpl::SetCustomFilters(
    const std::vector<std::string>& filters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());

  if (filters.empty()) {
    custom_filters_.reset();
    return;
  }

  custom_filters_ = custom_subscription_creator_.Run(filters);
}

void SubscriptionServiceImpl::OnHeadRequestDone(
    InstallationCallback on_finished,
    const std::string version) {
  if (version.empty()) {
    std::move(on_finished).Run(false);
    return;
  }
  persistent_metadata_->SetVersion(AcceptableAdsUrl(), version);
  persistent_metadata_->SetExpirationInterval(
      AcceptableAdsUrl(), kDefaultHeadRequestExpirationInterval);
  std::move(on_finished).Run(true);
}

void SubscriptionServiceImpl::OnSubscriptionDataAvailable(
    InstallationCallback on_finished,
    scoped_refptr<OngoingInstallation> ongoing_installation,
    std::unique_ptr<FlatbufferData> raw_data) {
  if (ongoing_installations_.find(ongoing_installation) ==
      ongoing_installations_.end()) {
    // Installation was canceled.
    std::move(on_finished).Run(false);
    return;
  }
  if (!raw_data) {
    // Download failed.
    ongoing_installations_.erase(ongoing_installation);
    std::move(on_finished).Run(false);
    return;
  }

  storage_->StoreSubscription(
      std::move(raw_data),
      base::BindOnce(&SubscriptionServiceImpl::SubscriptionAddedToStorage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_finished),
                     ongoing_installation));
}

void SubscriptionServiceImpl::StorageInitialized(
    std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(current_state_.empty())
      << "current state was modified before initial state was loaded";
  current_state_ = std::move(loaded_subscriptions);
  // SubscriptionPersistentStorage allows multiple Subscriptions with same URL,
  // which is a legal transitive state during ex. installing an update.
  // However, SubscriptionService's current_state_ should always contain only
  // one subscription with a given URL. This normally happens automatically by
  // virtue of |SubscriptionAddedToStorage| calling |UninstallSubscription| but
  // this invariant might not hold if ex. the application exits after
  // SubscriptionPersistentStorage stores the update but before
  // SubscriptionServiceImpl uninstalls the old version. It's difficult to
  // make installing subscription updates atomic, so solve potential race
  // condition here:
  RemoveDuplicateSubscriptions();
  status_ = Status::Initialized;
  UpdatePreloadedSubscriptionProvider();
  TRACE_EVENT_ASYNC_END0("eyeo", "SubscriptionService::Initialize",
                         TRACE_ID_LOCAL(this));
  DLOG(INFO) << "[eyeo] Subscription service initialized, will now run "
             << queued_tasks_.size() << " pending tasks.";
  while (!queued_tasks_.empty()) {
    std::move(queued_tasks_.front()).Run();
    queued_tasks_.erase(queued_tasks_.begin());
  }
}

void SubscriptionServiceImpl::SynchronizeWithPrefState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (enable_adblock_.GetValue() && status_ == Status::Uninitialized) {
    DLOG(INFO) << "[eyeo] Subscription service initialization starting.";
    TRACE_EVENT_ASYNC_BEGIN0("eyeo", "SubscriptionService::Initialize",
                             TRACE_ID_LOCAL(this));
    status_ = Status::LoadingSubscriptions;
    storage_->LoadSubscriptions(
        base::BindOnce(&SubscriptionServiceImpl::StorageInitialized,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void SubscriptionServiceImpl::RemoveDuplicateSubscriptions() {
  // std::sort + std::unique is not good for this use case, as we need to
  // perform actions on the duplicates, not just discard them, and std::unique
  // leaves moved elements in unspecified state. std::adjacent_find or
  // std::unique_copy could be used as well, but using a helper std::set seems
  // simplest.
  const auto comparator = [](const auto& lhs, const auto& rhs) {
    return lhs->GetSourceUrl() < rhs->GetSourceUrl();
  };
  std::set<scoped_refptr<InstalledSubscription>, decltype(comparator)>
      unique_subscriptions(comparator);
  for (auto subscription : current_state_) {
    if (!unique_subscriptions.insert(subscription).second) {
      // This element already exists in the set, we found a duplicate.
      storage_->RemoveSubscription(subscription);
    }
  }
  current_state_.assign(unique_subscriptions.begin(),
                        unique_subscriptions.end());
}

void SubscriptionServiceImpl::SubscriptionAddedToStorage(
    InstallationCallback on_finished,
    scoped_refptr<OngoingInstallation> ongoing_installation,
    scoped_refptr<InstalledSubscription> subscription) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ongoing_installations_.find(ongoing_installation) ==
      ongoing_installations_.end()) {
    // Installation was canceled. We must now remove the subscription from
    // storage. Do not add it to |current_state|.
    storage_->RemoveSubscription(subscription);
    std::move(on_finished).Run(false);
    return;
  }
  ongoing_installations_.erase(ongoing_installation);

  if (!subscription) {
    // There was an error adding subscription to storage.
    LOG(WARNING) << "[eyeo] Failed to add subscription, current number "
                 << "of subscriptions: " << current_state_.size();
    std::move(on_finished).Run(false);
    return;
  }
  // Remove any subscription that already exists with the same URL
  UninstallSubscriptionInternal(subscription->GetSourceUrl());
  // Add the new subscription
  current_state_.push_back(subscription);
  VLOG(1) << "[eyeo] Added subscription " << subscription->GetSourceUrl()
          << ", current number of subscriptions: " << current_state_.size();

  std::move(on_finished).Run(true);
}

void SubscriptionServiceImpl::UninstallSubscriptionInternal(
    const GURL& subscription_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  auto it = base::ranges::find(current_state_, subscription_url,
                               &Subscription::GetSourceUrl);
  if (it != current_state_.end()) {
    DLOG(INFO) << "[eyeo] Removing subscription " << subscription_url;
    storage_->RemoveSubscription(*it);
    current_state_.erase(it);
    VLOG(1) << "[eyeo] Removed subscription " << subscription_url;
  }

  auto ongoing_installation_it = base::ranges::find(
      ongoing_installations_, subscription_url, &Subscription::GetSourceUrl);
  if (ongoing_installation_it != ongoing_installations_.end()) {
    ongoing_installations_.erase(ongoing_installation_it);
    DLOG(INFO) << "[eyeo] Canceling installation of subscription "
               << subscription_url;
    downloader_->CancelDownload(subscription_url);
    VLOG(1) << "[eyeo] Canceled installation of subscription "
            << subscription_url;
  }
}

void SubscriptionServiceImpl::UpdatePreloadedSubscriptionProvider() {
  preloaded_subscription_provider_->UpdateSubscriptions(
      GetReadySubscriptions(), GetPendingSubscriptions());
}

void SubscriptionServiceImpl::UpdatePreloadedSubscriptionProviderAndRun(
    InstallationCallback on_finished,
    bool success) {
  UpdatePreloadedSubscriptionProvider();
  std::move(on_finished).Run(success);
}

}  // namespace adblock

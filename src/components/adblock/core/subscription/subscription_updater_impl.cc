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

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"

namespace adblock {

SubscriptionUpdaterImpl::SubscriptionUpdaterImpl(
    PrefService* prefs,
    SubscriptionService* subscription_service,
    SubscriptionPersistentMetadata* persistent_metadata,
    base::TimeDelta initial_delay,
    base::TimeDelta check_interval)
    : subscription_service_(subscription_service),
      persistent_metadata_(persistent_metadata),
      initial_delay_(initial_delay),
      check_interval_(check_interval) {
  enable_adblock_.Init(
      prefs::kEnableAdblock, prefs,
      base::BindRepeating(&SubscriptionUpdaterImpl::SynchronizeWithPrefState,
                          weak_ptr_factory_.GetWeakPtr()));
}

SubscriptionUpdaterImpl::~SubscriptionUpdaterImpl() = default;

void SubscriptionUpdaterImpl::StartSchedule() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (subscription_service_->IsInitialized()) {
    SynchronizeWithPrefState();
  } else {
    subscription_service_->RunWhenInitialized(
        base::BindOnce(&SubscriptionUpdaterImpl::SynchronizeWithPrefState,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void SubscriptionUpdaterImpl::StopSchedule() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  timer_.Stop();
}

void SubscriptionUpdaterImpl::SynchronizeWithPrefState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!subscription_service_->IsInitialized()) {
    DCHECK(!timer_.IsRunning());
    // We'll be called again when |subscription_service_| becomes initialized
    // and we'll start running update checks then, provided the pref state
    // allows.
    return;
  }
  if (enable_adblock_.GetValue() && !timer_.IsRunning()) {
    DLOG(INFO) << "[eyeo] Starting update schedule, first check scheduled for "
               << base::Time::Now() + initial_delay_;
    timer_.Start(FROM_HERE, initial_delay_,
                 base::BindOnce(&SubscriptionUpdaterImpl::RunUpdateCheck,
                                weak_ptr_factory_.GetWeakPtr()));
  } else if (!enable_adblock_.GetValue() && timer_.IsRunning()) {
    DLOG(INFO) << "[eyeo] Stopping update schedule";
    timer_.Stop();
  }
}

void SubscriptionUpdaterImpl::RunUpdateCheck() {
  DLOG(INFO) << "[eyeo] Running subscription update check";
  const auto available_subscriptions =
      subscription_service_->GetCurrentSubscriptions();
  for (const auto& subscription : available_subscriptions) {
    if (subscription->GetInstallationState() !=
        Subscription::InstallationState::Installed) {
      DLOG(INFO) << "[eyeo] Skipping update of " << subscription->GetSourceUrl()
                 << ", it's currently being installed or updated";
      continue;
    }

    if (persistent_metadata_->IsExpired(subscription->GetSourceUrl())) {
      DLOG(INFO) << "[eyeo] Downloading update for subscription "
                 << subscription->GetSourceUrl();
      subscription_service_->DownloadAndInstallSubscription(
          subscription->GetSourceUrl(), base::DoNothing());
    } else {
      DLOG(INFO) << "[eyeo] Subscription " << subscription->GetSourceUrl()
                 << " not expired yet, not updating";
    }
  }
  SendPingIfNecessary(available_subscriptions);
  DLOG(INFO)
      << "[eyeo] Subscription update check completed, next one scheduled for "
      << base::Time::Now() + check_interval_;
  timer_.Start(FROM_HERE, check_interval_,
               base::BindOnce(&SubscriptionUpdaterImpl::RunUpdateCheck,
                              weak_ptr_factory_.GetWeakPtr()));
}

void SubscriptionUpdaterImpl::SendPingIfNecessary(
    const std::vector<scoped_refptr<Subscription>>& available_subscriptions) {
  // When an Acceptable Ads subscription is not installed, we will not send an
  // update download request. But to know how many users have AA disabled,
  // we send a special HEAD request, or a ping. It contains the same data as
  // a download request, but the response has no payload.
  const bool has_acceptable_ads =
      base::ranges::find(available_subscriptions, AcceptableAdsUrl(),
                         &Subscription::GetSourceUrl) !=
      available_subscriptions.end();
  if (!has_acceptable_ads) {
    if (persistent_metadata_->IsExpired(AcceptableAdsUrl())) {
      DLOG(INFO) << "[eyeo] Acceptable Ads disabled, sending ping";
      subscription_service_->PingAcceptableAds(base::DoNothing());
    } else {
      DLOG(INFO)
          << "[eyeo] Acceptable Ads disabled, subscription not expired yet";
    }
  }
}

}  // namespace adblock

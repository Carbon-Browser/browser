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

#ifndef COMPONENTS_ADBLOCK_CORE_ADBLOCK_CONTROLLER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_ADBLOCK_CONTROLLER_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/prefs/pref_member.h"

class GURL;
class PrefService;

namespace adblock {

/**
 * @brief Implementation of the AdblockController interface. Uses preferences
 * as the backend for all the state set via the interface.
 * Only reads and writes the preferences.
 */
class AdblockControllerImpl : public AdblockController {
 public:
  AdblockControllerImpl(PrefService* pref_service,
                        SubscriptionService* subscription_service,
                        const std::string& locale);
  ~AdblockControllerImpl() override;

  AdblockControllerImpl(const AdblockControllerImpl&) = delete;
  AdblockControllerImpl& operator=(const AdblockControllerImpl&) = delete;
  AdblockControllerImpl(AdblockControllerImpl&&) = delete;
  AdblockControllerImpl& operator=(AdblockControllerImpl&&) = delete;

  void AddObserver(AdblockController::Observer* observer) override;
  void RemoveObserver(AdblockController::Observer* observer) override;

  void SetAdblockEnabled(bool enabled) override;
  bool IsAdblockEnabled() const override;
  void SetAcceptableAdsEnabled(bool enabled) override;
  bool IsAcceptableAdsEnabled() const override;
  void SelectBuiltInSubscription(const GURL& url) override;
  void UnselectBuiltInSubscription(const GURL& url) override;
  std::vector<scoped_refptr<Subscription>> GetSelectedBuiltInSubscriptions()
      const override;

  void AddCustomSubscription(const GURL& url) override;
  void RemoveCustomSubscription(const GURL& url) override;
  std::vector<scoped_refptr<Subscription>> GetCustomSubscriptions()
      const override;

  void AddAllowedDomain(const std::string& domain) override;
  void RemoveAllowedDomain(const std::string& domain) override;
  std::vector<std::string> GetAllowedDomains() const override;

  void SetUpdateConsent(AllowedConnectionType consent) override;
  AllowedConnectionType GetUpdateConsent() const override;

  void AddCustomFilter(const std::string& filter) override;
  void RemoveCustomFilter(const std::string& filter) override;
  std::vector<std::string> GetCustomFilters() const override;

  std::vector<KnownSubscriptionInfo> GetKnownSubscriptions() const override;

  // Synchronizes the state of SubscriptionService with the content of Prefs.
  void SynchronizeWithPrefsWhenPossible();

 protected:
  SEQUENCE_CHECKER(sequence_checker_);
  bool HasAcceptableAdsInstalled() const;
  void NotifySubscriptionChanged(const GURL& subscription_url);
  void UninstallSubscription(const GURL& subscription_url);
  void DownloadAndInstallSubscription(const GURL& subscription_url);
  void OnSubscriptionDownloaded(const GURL& subscription_url, bool success);
  void SynchronizeWithSubscriptionService();
  void SynchronizeCustomFiltersAndAllowedDomains();
  void SynchronizeSubscriptions();
  void InstallMissingSubscriptions(
      const std::vector<GURL>& subscriptions_in_prefs,
      const std::vector<GURL>& subscriptions_in_service);
  void RemoveUnexpectedSubscriptions(
      const std::vector<GURL>& subscriptions_in_prefs,
      const std::vector<GURL>& subscriptions_in_service);
  GURL FindLanguageBasedRecommendedSubscription() const;
  std::vector<scoped_refptr<Subscription>> GetSubscriptionsThatMatchPref(
      const StringListPrefMember& url_list) const;

  PrefService* prefs_{nullptr};
  BooleanPrefMember enable_adblock_;
  BooleanPrefMember enable_aa_;
  StringPrefMember allowed_connection_type_;
  StringListPrefMember allowed_domains_;
  StringListPrefMember custom_filters_;
  StringListPrefMember subscriptions_;
  StringListPrefMember custom_subscriptions_;
  SubscriptionService* subscription_service_;
  std::string language_;
  base::ObserverList<AdblockController::Observer> observers_;
  base::WeakPtrFactory<AdblockControllerImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_CONTROLLER_IMPL_H_

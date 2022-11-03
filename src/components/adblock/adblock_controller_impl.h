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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_CONTROLLER_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_CONTROLLER_IMPL_H_

#include <string>
#include <vector>

#include "base/sequence_checker.h"
#include "components/adblock/adblock_controller.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_state_synchronizer.h"
#include "components/prefs/pref_member.h"

class GURL;
class PrefService;

namespace adblock {

/**
 * @brief Implementation of the AdblockController interface. Uses preferences
 * as the backend for all the state set via the interface.
 * Only reads and writes the preferences. AdblockStateSynchronizer is
 * responsible for synchronizing Filter Engine's state with these preferences.
 * @see AdblockStateSynchronizer
 */
class AdblockControllerImpl : public AdblockController,
                              public AdblockPlatformEmbedder::Observer {
 public:
  AdblockControllerImpl(PrefService* service,
                        scoped_refptr<AdblockPlatformEmbedder> embedder);
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
  std::vector<GURL> GetSelectedBuiltInSubscriptions() const override;

  void AddCustomSubscription(const GURL& url) override;
  void RemoveCustomSubscription(const GURL& url) override;
  std::vector<GURL> GetCustomSubscriptions() const override;

  void AddAllowedDomain(const std::string& domain) override;
  void RemoveAllowedDomain(const std::string& domain) override;
  std::vector<std::string> GetAllowedDomains() const override;

  void SetUpdateConsent(AllowedConnectionType consent) override;
  AllowedConnectionType GetUpdateConsent() const override;

  void AddCustomFilter(const std::string& filter) override;
  void RemoveCustomFilter(const std::string& filter) override;
  std::vector<std::string> GetCustomFilters() const override;

  std::vector<Subscription> GetRecommendedSubscriptions() const override;

  // AdblockPlatformEmbedder::Observer:
  void OnFilterEngineCreated(const std::vector<Subscription>& recommended,
                             const std::vector<GURL>& listed,
                             const GURL& acceptable_ads) override;
  void OnSubscriptionUpdated(const GURL& url) override;

 private:
  void Init(PrefService* service);

  SEQUENCE_CHECKER(sequence_checker_);

  BooleanPrefMember enable_adblock_;
  BooleanPrefMember enable_acceptable_ads_;
  StringListPrefMember allowed_domains_;
  StringListPrefMember custom_filters_;
  StringListPrefMember custom_subscriptions_;
  StringListPrefMember subscriptions_;
  StringPrefMember allowed_connection_type_;
  PrefService* prefs_{nullptr};
  scoped_refptr<AdblockPlatformEmbedder> embedder_;
  base::ObserverList<AdblockController::Observer> observers_;
  // TODO AdblockControllerImpl and AdblockStateSynchronizer are tightly
  // coupled, so they're stored together. May refactor this in DPD-612.
  AdblockStateSynchronizer state_synchronizer_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_CONTROLLER_IMPL_H_

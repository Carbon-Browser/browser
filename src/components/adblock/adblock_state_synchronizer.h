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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_STATE_SYNCHRONIZER_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_STATE_SYNCHRONIZER_H_

#include "base/sequence_checker.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/prefs/pref_member.h"

class PrefService;

namespace adblock {
/**
 * @brief Responsible for synchronizing configuration state between UI Settings
 * and FilterEngine. Listens to perf change on UI and sets the corresponding
 * setting on Filter Engine. Listens to embedder observer events and updates UI.
 * Lives in browser Process UI main thread.
 */
class AdblockStateSynchronizer final
    : public adblock::AdblockPlatformEmbedder::Observer {
 public:
  explicit AdblockStateSynchronizer(
      scoped_refptr<adblock::AdblockPlatformEmbedder> embedder);
  ~AdblockStateSynchronizer() final;
  AdblockStateSynchronizer& operator=(const AdblockStateSynchronizer& other) =
      delete;
  AdblockStateSynchronizer& operator=(AdblockStateSynchronizer&& other) =
      delete;
  AdblockStateSynchronizer(const AdblockStateSynchronizer& other) = delete;
  AdblockStateSynchronizer(AdblockStateSynchronizer&& other) = delete;

  void Init(PrefService* service);
  bool IsAdblockEnabled() const;
  bool IsAcceptableAdsEnabled() const;
  AllowedConnectionType GetAllowedConnectionType() const;

 private:
  enum class AllowedConnectionTypeChangeAction { kNone, kUpdateSubscriptions };
  void OnPrefChanged(const std::string& name);
  void SynchronizeAll();
  void SynchronizeAllowedConnectionTypeBasedOnABPState(
      AllowedConnectionTypeChangeAction action);

  // Overrides from adblock::AdblockPlatformEmbedder::Observer:
  void OnFilterEngineCreated(
      const std::vector<adblock::Subscription>& recommended,
      const std::vector<GURL>& listed,
      const GURL& acceptable_ads) final;
  void OnSubscriptionUpdated(const GURL& url) final;
  void MaybeUpdateLastValue(std::vector<std::string>* member,
                            std::vector<std::string>&& pref,
                            bool synchronization_done);

  SEQUENCE_CHECKER(sequence_checker_);
  BooleanPrefMember enable_adblock_;
  BooleanPrefMember enable_acceptable_ads_;
  StringListPrefMember allowed_domains_;
  std::vector<std::string> last_allowed_domains_;
  StringListPrefMember custom_filters_;
  std::vector<std::string> last_custom_filters_;
  StringListPrefMember custom_subscriptions_;
  std::vector<std::string> last_custom_subscriptions_;
  StringListPrefMember subscriptions_;
  std::vector<std::string> last_subscriptions_;
  StringPrefMember allowed_network_type_;
  scoped_refptr<adblock::AdblockPlatformEmbedder> embedder_;
#if DCHECK_IS_ON()
  bool initialized_{false};
#endif
  base::WeakPtrFactory<AdblockStateSynchronizer> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_STATE_SYNCHRONIZER_H_

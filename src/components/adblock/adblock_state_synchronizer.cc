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

#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/utf_string_conversions.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_utils.h"
#include "components/adblock/allowed_connection_type.h"
#include "url/url_canon.h"
#include "url/url_util.h"

#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

namespace adblock {

namespace {

std::vector<std::string> Convert(std::vector<GURL> urls) {
  std::vector<std::string> result;
  for (const auto& url : urls) {
    result.emplace_back(url.spec());
  }
  return result;
}

void SynchronizeAllowedConnectionType(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    AllowedConnectionType allowed_type,
    bool update_blocked_subscriptions) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  DCHECK(allowed_type == AllowedConnectionType::kAny ||
         allowed_type == AllowedConnectionType::kWiFi ||
         allowed_type == AllowedConnectionType::kNone);
  DCHECK(!update_blocked_subscriptions ||
         allowed_type != AllowedConnectionType::kNone);
  if (!embedder->GetFilterEngine())
    return;

  const std::string allowed_type_str =
      AllowedConnectionTypeToString(allowed_type);
  embedder->GetFilterEngine()->SetAllowedConnectionType(&allowed_type_str);
  // Setting allowed connection to "none" makes the list downloads to fail
  // with "synchronize_connection_error". We do not want to download them
  // if not needed otherwise it may affect our user counting which is based on
  // lists downloads. This is why when changing the connection type from "none"
  // with |update_blocked_subscriptions| we update only subscriptions which most
  // likely failed due to connections being not allowed.
  if (update_blocked_subscriptions) {
    auto subscriptions = embedder->GetFilterEngine()->GetListedSubscriptions();
    for (auto& subscription : subscriptions) {
      if (subscription.GetSynchronizationStatus() ==
          kSynchronizationErrorStatus) {
        DLOG(INFO) << "[ABP] Force updating " << subscription.GetUrl();
        subscription.UpdateFilters();
      }
    }
  }
}

bool SynchronizeAllowedDomains(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    const std::vector<std::string>& old_domains,
    const std::vector<std::string>& new_domains) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  if (!embedder->GetFilterEngine())
    return false;

  for (const auto& domain : old_domains) {
    std::string filter = adblock::utils::CreateDomainAllowlistingFilter(domain);
    embedder->GetFilterEngine()->RemoveFilter(
        embedder->GetFilterEngine()->GetFilter(filter));
  }

  for (const auto& domain : new_domains) {
    std::string filter = adblock::utils::CreateDomainAllowlistingFilter(domain);
    embedder->GetFilterEngine()->AddFilter(
        embedder->GetFilterEngine()->GetFilter(filter));
  }

  return true;
}

bool SynchronizeCustomFilters(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    const std::vector<std::string>& old_filters,
    const std::vector<std::string>& new_filters) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  if (!embedder->GetFilterEngine())
    return false;

  for (const auto& filter : old_filters) {
    embedder->GetFilterEngine()->RemoveFilter(
        embedder->GetFilterEngine()->GetFilter(filter));
  }

  for (const auto& filter : new_filters) {
    embedder->GetFilterEngine()->AddFilter(
        embedder->GetFilterEngine()->GetFilter(filter));
  }

  return true;
}

bool SynchronizeSubscriptions(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
    const std::vector<std::string>& old_subscriptions,
    const std::vector<std::string>& new_subscriptions) {
  DCHECK(embedder->GetAbpTaskRunner()->BelongsToCurrentThread());
  if (!embedder->GetFilterEngine())
    return false;

  // This (and below) is O(N*M) (twice).
  // More optimal would be just to remove all old urls and add all new ones
  // but this would cause DP-1061 from libabp-android to happen here because
  // the core removes files on RemoveFromList() and downloads them again on
  // AddToList() which in case of lack of connectivity can lead to be
  // out of lists completely and core at 3.9.1 version does not know about
  // exponential backoff retries yet and retries after 24h which leads to
  // adblock not blocking anything for 24h in the worse case scenario...
  for (const auto& url : old_subscriptions) {
    auto element =
        std::find(new_subscriptions.begin(), new_subscriptions.end(), url);
    if (element == new_subscriptions.end()) {
      LOG(INFO) << "[ABP] Removing subscription " << url;
      embedder->GetFilterEngine()->RemoveSubscription(
          embedder->GetFilterEngine()->GetSubscription(url));
    }
  }

  for (const auto& url : new_subscriptions) {
    auto element =
        std::find(old_subscriptions.begin(), old_subscriptions.end(), url);
    if (element == old_subscriptions.end()) {
      LOG(INFO) << "[ABP] Adding subscription " << url;
      embedder->GetFilterEngine()->AddSubscription(
          embedder->GetFilterEngine()->GetSubscription(url));
    }
  }

  return true;
}

void SynchronizeAA(scoped_refptr<adblock::AdblockPlatformEmbedder> embedder,
                   bool enabled) {
  if (!embedder->GetFilterEngine())
    return;
  embedder->GetFilterEngine()->SetAAEnabled(enabled);
}

}  // namespace

AdblockStateSynchronizer::AdblockStateSynchronizer(
    scoped_refptr<adblock::AdblockPlatformEmbedder> embedder)
    : embedder_(std::move(embedder)) {
  embedder_->AddObserver(this);
}

AdblockStateSynchronizer::~AdblockStateSynchronizer() {
  embedder_->RemoveObserver(this);
}

void AdblockStateSynchronizer::Init(PrefService* pref_service) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto observer = base::BindRepeating(&AdblockStateSynchronizer::OnPrefChanged,
                                      base::Unretained(this));

  enable_adblock_.Init(adblock::prefs::kEnableAdblock, pref_service, observer);
  enable_acceptable_ads_.Init(adblock::prefs::kEnableAcceptableAds,
                              pref_service, observer);
  allowed_domains_.Init(adblock::prefs::kAdblockAllowedDomains, pref_service,
                        observer);
  custom_filters_.Init(adblock::prefs::kAdblockCustomFilters, pref_service,
                       observer);
  custom_subscriptions_.Init(adblock::prefs::kAdblockCustomSubscriptions,
                             pref_service, observer);
  last_custom_subscriptions_ = custom_subscriptions_.GetValue();
  subscriptions_.Init(adblock::prefs::kAdblockSubscriptions, pref_service,
                      observer);
  last_subscriptions_ = subscriptions_.GetValue();
  allowed_network_type_.Init(adblock::prefs::kAdblockAllowedConnectionType,
                             pref_service, observer);
#if DCHECK_IS_ON()
  initialized_ = true;
#endif
}

bool AdblockStateSynchronizer::IsAdblockEnabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return enable_adblock_.GetValue();
}

bool AdblockStateSynchronizer::IsAcceptableAdsEnabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return enable_acceptable_ads_.GetValue();
}

void AdblockStateSynchronizer::OnPrefChanged(const std::string& name) {
  if (name == adblock::prefs::kEnableAdblock) {
    // This is a hacky way of preventing filter list downloads when ABP
    // is disabled. It's needed because the core isn't aware of the state
    // and tries to download. This is why if a user disables ABP this code
    // sets the special "none" allowed connection type (see
    // GetConnectionType()). Having the "none" allowed connection type results
    // in all core requests being disallowed (see IsConnectionTypeAllowed()
    // in adblock_platform_embedder_impl.cc ). When ABP is enabled this sets
    // a proper connection type read from the pref.
    AllowedConnectionTypeChangeAction action =
        enable_adblock_.GetValue()
            ? AllowedConnectionTypeChangeAction::kUpdateSubscriptions
            : AllowedConnectionTypeChangeAction::kNone;
    SynchronizeAllowedConnectionTypeBasedOnABPState(action);
  } else if (name == adblock::prefs::kEnableAcceptableAds) {
    embedder_->GetAbpTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&SynchronizeAA, embedder_,
                                  enable_acceptable_ads_.GetValue()));
  } else if (name == adblock::prefs::kAdblockAllowedDomains) {
    embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&SynchronizeAllowedDomains, embedder_,
                       last_allowed_domains_, allowed_domains_.GetValue()),
        base::BindOnce(&AdblockStateSynchronizer::MaybeUpdateLastValue,
                       weak_ptr_factory_.GetWeakPtr(), &last_allowed_domains_,
                       allowed_domains_.GetValue()));
  } else if (name == adblock::prefs::kAdblockCustomFilters) {
    embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&SynchronizeCustomFilters, embedder_,
                       last_custom_filters_, custom_filters_.GetValue()),
        base::BindOnce(&AdblockStateSynchronizer::MaybeUpdateLastValue,
                       weak_ptr_factory_.GetWeakPtr(), &last_custom_filters_,
                       custom_filters_.GetValue()));
  } else if (name == adblock::prefs::kAdblockAllowedConnectionType) {
    SynchronizeAllowedConnectionTypeBasedOnABPState(
        AllowedConnectionTypeChangeAction::kNone);
  } else if (name == adblock::prefs::kAdblockSubscriptions) {
    embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&SynchronizeSubscriptions, embedder_,
                       last_subscriptions_, subscriptions_.GetValue()),
        base::BindOnce(&AdblockStateSynchronizer::MaybeUpdateLastValue,
                       weak_ptr_factory_.GetWeakPtr(), &last_subscriptions_,
                       subscriptions_.GetValue()));
  } else if (name == adblock::prefs::kAdblockCustomSubscriptions) {
    embedder_->GetAbpTaskRunner()->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&SynchronizeSubscriptions, embedder_,
                       last_custom_subscriptions_,
                       custom_subscriptions_.GetValue()),
        base::BindOnce(&AdblockStateSynchronizer::MaybeUpdateLastValue,
                       weak_ptr_factory_.GetWeakPtr(),
                       &last_custom_subscriptions_,
                       custom_subscriptions_.GetValue()));
  }
}

void AdblockStateSynchronizer::MaybeUpdateLastValue(
    std::vector<std::string>* member,
    std::vector<std::string>&& pref,
    bool synchronization_performed) {
  if (!synchronization_performed)
    return;

  *member = std::move(pref);
}

void AdblockStateSynchronizer::SynchronizeAllowedConnectionTypeBasedOnABPState(
    AllowedConnectionTypeChangeAction action) {
  embedder_->GetAbpTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SynchronizeAllowedConnectionType, embedder_,
          GetAllowedConnectionType(),
          action == AllowedConnectionTypeChangeAction::kUpdateSubscriptions));
}

void AdblockStateSynchronizer::OnFilterEngineCreated(
    const std::vector<adblock::Subscription>& recommended,
    const std::vector<GURL>& listed,
    const GURL& acceptable_ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::vector<GURL> subscriptions;
  std::vector<GURL> custom_subscriptions;
  for (const auto& url : listed) {
    if (std::find_if(recommended.begin(), recommended.end(),
                     [&url](const adblock::Subscription& sub) {
                       return url == sub.url;
                     }) != recommended.end()) {
      subscriptions.push_back(url);
    } else if (url != acceptable_ads) {
      custom_subscriptions.push_back(url);
    }
  }

  if (subscriptions_.GetValue().empty()) {
    subscriptions_.SetValue(Convert(subscriptions));
    last_subscriptions_ = subscriptions_.GetValue();
  }

  if (custom_subscriptions_.GetValue().empty() &&
      !custom_subscriptions.empty()) {
    custom_subscriptions_.SetValue(Convert(custom_subscriptions));
    last_custom_subscriptions_ = custom_subscriptions_.GetValue();
  }

  SynchronizeAll();
}

void AdblockStateSynchronizer::OnSubscriptionUpdated(const GURL& url) {}

void AdblockStateSynchronizer::SynchronizeAll() {
  OnPrefChanged(adblock::prefs::kAdblockAllowedConnectionType);
  OnPrefChanged(adblock::prefs::kAdblockSubscriptions);
  OnPrefChanged(adblock::prefs::kAdblockCustomFilters);
  OnPrefChanged(adblock::prefs::kAdblockCustomSubscriptions);
  OnPrefChanged(adblock::prefs::kAdblockAllowedDomains);
  OnPrefChanged(adblock::prefs::kEnableAcceptableAds);
}

AllowedConnectionType AdblockStateSynchronizer::GetAllowedConnectionType()
    const {
#if DCHECK_IS_ON()
  DCHECK(initialized_) << "call Init() first!";
#endif
  if (enable_adblock_.GetValue()) {
    auto parsed_allow_type =
        AllowedConnectionTypeFromString(allowed_network_type_.GetValue());
    return parsed_allow_type ? *parsed_allow_type
                             : AllowedConnectionType::kWiFi;
  }
  return AllowedConnectionType::kNone;
}

}  // namespace adblock

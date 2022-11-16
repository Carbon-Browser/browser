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

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/version.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/allowed_connection_type.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/language/core/common/locale_util.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "url/gurl.h"

namespace adblock {

namespace {

bool AddToPref(const std::string& to_add, StringListPrefMember* member) {
  if (to_add.empty())
    return false;
  auto current_value = member->GetValue();
  if (std::find(current_value.begin(), current_value.end(), to_add) !=
      current_value.end()) {
    return false;
  }
  current_value.emplace_back(to_add);
  member->SetValue(current_value);
  return true;
}

bool RemoveFromPref(const std::string& to_remove,
                    StringListPrefMember* member) {
  if (to_remove.empty())
    return false;
  auto current_value = member->GetValue();
  auto elem = std::find(current_value.begin(), current_value.end(), to_remove);
  if (elem == current_value.end())
    return false;
  current_value.erase(elem);
  member->SetValue(current_value);
  return true;
}

std::vector<GURL> UrlsFromPref(const StringListPrefMember& member) {
  std::vector<GURL> subscriptions_in_prefs;
  for (const auto& sub : *member)
    subscriptions_in_prefs.emplace_back(sub);
  return subscriptions_in_prefs;
}

void SortAndRemoveDuplicates(std::vector<GURL>& collection) {
  std::sort(collection.begin(), collection.end());
  auto last = std::unique(collection.begin(), collection.end());
  if (last != collection.end())
    collection.erase(last, collection.end());
}

}  // namespace

AdblockControllerImpl::AdblockControllerImpl(
    PrefService* pref_service,
    SubscriptionService* subscription_service,
    const std::string& locale)
    : prefs_(pref_service),
      subscription_service_(subscription_service),
      language_(language::ExtractBaseLanguage(locale)) {
  auto on_change_cb = base::BindRepeating(
      &AdblockControllerImpl::SynchronizeWithSubscriptionService,
      base::Unretained(this));

  enable_adblock_.Init(adblock::prefs::kEnableAdblock, pref_service,
                       on_change_cb);
  enable_aa_.Init(adblock::prefs::kEnableAcceptableAds, pref_service,
                  on_change_cb);
  allowed_connection_type_.Init(adblock::prefs::kAdblockAllowedConnectionType,
                                pref_service, on_change_cb);

  allowed_domains_.Init(adblock::prefs::kAdblockAllowedDomains, pref_service,
                        on_change_cb);
  custom_filters_.Init(adblock::prefs::kAdblockCustomFilters, pref_service,
                       on_change_cb);
  subscriptions_.Init(adblock::prefs::kAdblockSubscriptions, pref_service,
                      on_change_cb);
  custom_subscriptions_.Init(adblock::prefs::kAdblockCustomSubscriptions,
                             pref_service, on_change_cb);

  // language::ExtractBaseLanguage is pretty basic, if it doesn't return
  // something that looks like a valid language, fallback to English and use the
  // default EasyList.
  if (language_.size() != 2u)
    language_ = "en";

  if (prefs_->GetBoolean(prefs::kInstallFirstStartSubscriptions) &&
      subscriptions_.IsDefaultValue() &&
      custom_subscriptions_.IsDefaultValue()) {
    // There are no subscriptions migrated from previous versions, we shall
    // install a language-based recommendation.
    AddToPref(FindLanguageBasedRecommendedSubscription().spec(),
              &subscriptions_);
  }
}

AdblockControllerImpl::~AdblockControllerImpl() = default;

void AdblockControllerImpl::AddObserver(AdblockController::Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void AdblockControllerImpl::RemoveObserver(
    AdblockController::Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void AdblockControllerImpl::SetAdblockEnabled(bool enabled) {
  enable_adblock_.SetValue(enabled);
}

bool AdblockControllerImpl::IsAdblockEnabled() const {
  return enable_adblock_.GetValue();
}

void AdblockControllerImpl::SetAcceptableAdsEnabled(bool enabled) {
  enable_aa_.SetValue(enabled);
  if (!subscription_service_->IsInitialized()) {
    // Subscription will be installed or removed in SynchronizeSubscriptions().
    return;
  }
  const bool has_acceptable_ads_installed = HasAcceptableAdsInstalled();
  if (IsAcceptableAdsEnabled() && !has_acceptable_ads_installed)
    DownloadAndInstallSubscription(AcceptableAdsUrl());
  else if (!IsAcceptableAdsEnabled() && has_acceptable_ads_installed)
    UninstallSubscription(AcceptableAdsUrl());
}

bool AdblockControllerImpl::IsAcceptableAdsEnabled() const {
  return enable_aa_.GetValue();
}

void AdblockControllerImpl::SelectBuiltInSubscription(const GURL& url) {
  if (url == AcceptableAdsUrl()) {
    SetAcceptableAdsEnabled(true);
  } else if (AddToPref(url.spec(), &subscriptions_)) {
    DownloadAndInstallSubscription(url);
  }
}

void AdblockControllerImpl::UnselectBuiltInSubscription(const GURL& url) {
  if (url == AcceptableAdsUrl()) {
    SetAcceptableAdsEnabled(false);
  } else if (RemoveFromPref(url.spec(), &subscriptions_)) {
    UninstallSubscription(url);
  }
}

std::vector<scoped_refptr<Subscription>>
AdblockControllerImpl::GetSelectedBuiltInSubscriptions() const {
  if (!subscription_service_->IsInitialized())
    return {};
  auto selected = GetSubscriptionsThatMatchPref(subscriptions_);
  // The Acceptable Ads subscription is not stored in the |subscriptions_| pref,
  // it's installation is implied by |enable_aa_| pref being true. Simplify
  // during DPD-1269.
  if (IsAcceptableAdsEnabled()) {
    const auto current_subscriptions =
        subscription_service_->GetCurrentSubscriptions();
    const auto aa_it = base::ranges::find(
        current_subscriptions, AcceptableAdsUrl(), &Subscription::GetSourceUrl);
    // If Acceptable Ads pref is true *and* the subscription is installed in
    // SubscriptionService, report it along with other built-in subscriptions.
    if (aa_it != current_subscriptions.end())
      selected.push_back(*aa_it);
  }
  return selected;
}

void AdblockControllerImpl::AddCustomSubscription(const GURL& url) {
  if (AddToPref(url.spec(), &custom_subscriptions_))
    DownloadAndInstallSubscription(url);
}

void AdblockControllerImpl::RemoveCustomSubscription(const GURL& url) {
  if (RemoveFromPref(url.spec(), &custom_subscriptions_))
    UninstallSubscription(url);
}

std::vector<scoped_refptr<Subscription>>
AdblockControllerImpl::GetCustomSubscriptions() const {
  if (!subscription_service_->IsInitialized())
    return {};
  return GetSubscriptionsThatMatchPref(custom_subscriptions_);
}

void AdblockControllerImpl::AddAllowedDomain(const std::string& domain) {
  if (AddToPref(base::ToLowerASCII(domain), &allowed_domains_) &&
      subscription_service_->IsInitialized())
    SynchronizeCustomFiltersAndAllowedDomains();
}

void AdblockControllerImpl::RemoveAllowedDomain(const std::string& domain) {
  if (RemoveFromPref(base::ToLowerASCII(domain), &allowed_domains_) &&
      subscription_service_->IsInitialized())
    SynchronizeCustomFiltersAndAllowedDomains();
}

std::vector<std::string> AdblockControllerImpl::GetAllowedDomains() const {
  return allowed_domains_.GetValue();
}

void AdblockControllerImpl::SetUpdateConsent(AllowedConnectionType value) {
  allowed_connection_type_.SetValue(AllowedConnectionTypeToString(value));
}

AllowedConnectionType AdblockControllerImpl::GetUpdateConsent() const {
  auto allowed_connection_type =
      AllowedConnectionTypeFromString(allowed_connection_type_.GetValue());
  if (!allowed_connection_type)
    return AllowedConnectionType::kWiFi;  // it's default
  return *allowed_connection_type;
}

void AdblockControllerImpl::AddCustomFilter(const std::string& filter) {
  if (AddToPref(filter, &custom_filters_) &&
      subscription_service_->IsInitialized())
    SynchronizeCustomFiltersAndAllowedDomains();
}

void AdblockControllerImpl::RemoveCustomFilter(const std::string& filter) {
  if (RemoveFromPref(filter, &custom_filters_) &&
      subscription_service_->IsInitialized())
    SynchronizeCustomFiltersAndAllowedDomains();
}

std::vector<std::string> AdblockControllerImpl::GetCustomFilters() const {
  return custom_filters_.GetValue();
}

std::vector<KnownSubscriptionInfo>
AdblockControllerImpl::GetKnownSubscriptions() const {
  auto subscriptions = config::GetKnownSubscriptions();
  subscriptions.emplace_back(AcceptableAdsUrl(), std::string("Acceptable Ads"),
                             std::vector<std::string>{},
                             SubscriptionUiVisibility::Invisible,
                             SubscriptionFirstRunBehavior::Subscribe,
                             SubscriptionPrivilegedFilterStatus::Forbidden);
  return subscriptions;
}

void AdblockControllerImpl::SynchronizeWithPrefsWhenPossible() {
  if (subscription_service_->IsInitialized()) {
    SynchronizeWithSubscriptionService();
  } else {
    subscription_service_->RunWhenInitialized(base::BindOnce(
        &AdblockControllerImpl::SynchronizeWithSubscriptionService,
        weak_ptr_factory_.GetWeakPtr()));
  }
}

bool AdblockControllerImpl::HasAcceptableAdsInstalled() const {
  const auto current_subscriptions =
      subscription_service_->GetCurrentSubscriptions();
  return base::ranges::find(current_subscriptions, AcceptableAdsUrl(),
                            &Subscription::GetSourceUrl) !=
         current_subscriptions.end();
}

void AdblockControllerImpl::NotifySubscriptionChanged(
    const GURL& subscription_url) {
  for (auto& observer : observers_)
    observer.OnSubscriptionUpdated(subscription_url);
}

void AdblockControllerImpl::UninstallSubscription(
    const GURL& subscription_url) {
  if (!subscription_service_->IsInitialized()) {
    // The subscription will be removed in SynchronizeSubscriptions().
    return;
  }
  NotifySubscriptionChanged(subscription_url);
  subscription_service_->UninstallSubscription(subscription_url);
}

void AdblockControllerImpl::DownloadAndInstallSubscription(
    const GURL& subscription_url) {
  if (!subscription_service_->IsInitialized()) {
    // The download will be started in SynchronizeSubscriptions().
    return;
  }
  subscription_service_->DownloadAndInstallSubscription(
      subscription_url,
      base::BindOnce(&AdblockControllerImpl::OnSubscriptionDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), subscription_url));
}

void AdblockControllerImpl::OnSubscriptionDownloaded(
    const GURL& subscription_url,
    bool success) {
  // Currently, we have no means of notifying the user about an unsuccessful
  // installation, so we ignore the value of |success|.
  // The content of the Prefs (and the lists returned by this Controller)
  // represent the *intent*, rf what the user wants installed, not the *current
  // state*, which may be different if downloads failed or if they were not
  // allowed to start. Failed downloads will be retried
  // (SynchronizeSubscriptions()) and we hope for eventual consistency.
  NotifySubscriptionChanged(subscription_url);
}

void AdblockControllerImpl::SynchronizeWithSubscriptionService() {
  if (!subscription_service_->IsInitialized()) {
    return;
  }
  SynchronizeCustomFiltersAndAllowedDomains();
  SynchronizeSubscriptions();
}

void AdblockControllerImpl::SynchronizeCustomFiltersAndAllowedDomains() {
  std::vector<std::string> custom_filters_and_allowed_domains =
      custom_filters_.GetValue();
  for (const auto& domain : allowed_domains_.GetValue())
    custom_filters_and_allowed_domains.push_back(
        utils::CreateDomainAllowlistingFilter(domain));
  subscription_service_->SetCustomFilters(custom_filters_and_allowed_domains);
}

void AdblockControllerImpl::SynchronizeSubscriptions() {
  // If the state of installed subscriptions in SubscriptionService is different
  // than the state in prefs, prefs take precedence.
  std::vector<GURL> subscriptions_in_prefs;
  if (prefs_->GetBoolean(prefs::kInstallFirstStartSubscriptions)) {
    // On first run, install additional subscriptions.
    for (const auto& cur : config::GetKnownSubscriptions()) {
      if (cur.first_run == SubscriptionFirstRunBehavior::Subscribe) {
        AddToPref(cur.url.spec(), &subscriptions_);
      }
    }
    prefs_->SetBoolean(prefs::kInstallFirstStartSubscriptions, false);
  }

  for (const auto& sub : *subscriptions_)
    subscriptions_in_prefs.emplace_back(sub);
  for (const auto& sub : *custom_subscriptions_)
    subscriptions_in_prefs.emplace_back(sub);

  if (enable_aa_.GetValue()) {
    // If prefs::kEnableAcceptableAds is true, also expect the Acceptable Ads
    // subscription to be installed.
    subscriptions_in_prefs.push_back(AcceptableAdsUrl());
  }
  SortAndRemoveDuplicates(subscriptions_in_prefs);

  std::vector<GURL> subscriptions_in_service;
  base::ranges::transform(subscription_service_->GetCurrentSubscriptions(),
                          std::back_inserter(subscriptions_in_service),
                          &Subscription::GetSourceUrl);
  base::ranges::sort(subscriptions_in_service);

  InstallMissingSubscriptions(subscriptions_in_prefs, subscriptions_in_service);
  RemoveUnexpectedSubscriptions(subscriptions_in_prefs,
                                subscriptions_in_service);
}

void AdblockControllerImpl::InstallMissingSubscriptions(
    const std::vector<GURL>& subscriptions_in_prefs,
    const std::vector<GURL>& subscriptions_in_service) {
  // Prefs can contain subscriptions not present in SubscriptionService when:
  // - This is a first run with flatbuffer-based implementation after
  // libadblockplus-based implementation (migration).
  // - The schema changed in a non-backward-compatible way and installed
  // subscriptions were invalidated.
  // - Calls were made to Controller's methods before the SubscriptionService
  // was initialized.
  std::vector<GURL> subscriptions_to_install;
  std::set_difference(
      subscriptions_in_prefs.begin(), subscriptions_in_prefs.end(),
      subscriptions_in_service.begin(), subscriptions_in_service.end(),
      std::back_inserter(subscriptions_to_install));
  for (const auto& sub : subscriptions_to_install) {
    DLOG(INFO) << "[eyeo] Installing missing subscription: " << sub;
    DownloadAndInstallSubscription(sub);
  }
}

void AdblockControllerImpl::RemoveUnexpectedSubscriptions(
    const std::vector<GURL>& subscriptions_in_prefs,
    const std::vector<GURL>& subscriptions_in_service) {
  // A Subscription can be present in Service (which means: on disk) but not
  // present in Prefs when:
  // - Uninstallation has started, and it removed the subscription from Prefs
  // immediately, but the async filesystem task didn't execute before browser
  // shut down.
  // - Calls were made to Controller's methods before the SubscriptionService
  // was initialized.
  std::vector<GURL> subscriptions_to_uninstall;
  std::set_difference(
      subscriptions_in_service.begin(), subscriptions_in_service.end(),
      subscriptions_in_prefs.begin(), subscriptions_in_prefs.end(),
      std::back_inserter(subscriptions_to_uninstall));
  for (const auto& sub : subscriptions_to_uninstall) {
    DLOG(INFO) << "[eyeo] Uninstalling unexpected subscription: " << sub;
    UninstallSubscription(sub);
  }
}

GURL AdblockControllerImpl::FindLanguageBasedRecommendedSubscription() const {
  const auto& recommended_subscriptions = config::GetKnownSubscriptions();
  auto language_specific_subscription = std::find_if(
      recommended_subscriptions.begin(), recommended_subscriptions.end(),
      [&](const KnownSubscriptionInfo& subscription) {
        return subscription.first_run ==
                   SubscriptionFirstRunBehavior::SubscribeIfLocaleMatch &&
               std::find(subscription.languages.begin(),
                         subscription.languages.end(),
                         language_) != subscription.languages.end();
      });
  if (language_specific_subscription == recommended_subscriptions.end()) {
    DLOG(INFO) << "[eyeo] Using the default subscription for language \""
               << language_ << "\"";
    return DefaultSubscriptionUrl();
  }
  DLOG(INFO) << "[eyeo] Using recommended subscription for language \""
             << language_ << "\": " << language_specific_subscription->title;
  return language_specific_subscription->url;
}

std::vector<scoped_refptr<Subscription>>
AdblockControllerImpl::GetSubscriptionsThatMatchPref(
    const StringListPrefMember& url_list) const {
  const auto subscription_urls_in_prefs = UrlsFromPref(url_list);
  auto current_subscriptions = subscription_service_->GetCurrentSubscriptions();
  // Remove Subscriptions that have URLs not present in
  // subscription_urls_in_prefs.
  current_subscriptions.erase(
      base::ranges::remove_if(current_subscriptions,
                              [&](const auto& installed_sub) {
                                return base::ranges::find(
                                           subscription_urls_in_prefs,
                                           installed_sub->GetSourceUrl()) ==
                                       subscription_urls_in_prefs.end();
                              }),
      current_subscriptions.end());
  return current_subscriptions;
}

}  // namespace adblock

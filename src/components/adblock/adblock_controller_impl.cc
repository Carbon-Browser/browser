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
#include "components/adblock/adblock_controller_impl.h"

#include <numeric>

#include "base/version.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_utils.h"
#include "components/adblock/allowed_connection_type.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace adblock {

namespace {

constexpr char kRecommendedSubscriptionObjectUrlKey[] = "url";
constexpr char kRecommendedSubscriptionObjectLanguagesKey[] = "languages";
constexpr char kRecommendedSubscriptionObjectTitleKey[] = "title";

void AddToPref(const std::string& to_add, StringListPrefMember* member) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto current_value = member->GetValue();
  if (std::find(current_value.begin(), current_value.end(), to_add) ==
      current_value.end()) {
    current_value.emplace_back(to_add);
    member->SetValue(current_value);
  }
}

void RemoveFromPref(const std::string& to_remove,
                    StringListPrefMember* member) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto current_value = member->GetValue();
  auto elem = std::find(current_value.begin(), current_value.end(), to_remove);
  if (elem != current_value.end()) {
    current_value.erase(elem);
    member->SetValue(current_value);
  }
}

std::vector<GURL> ConvertURLs(const std::vector<std::string>& input) {
  std::vector<GURL> output;
  output.reserve(input.size());
  std::transform(std::begin(input), std::end(input), std::back_inserter(output),
                 [](const std::string& url) { return GURL{url}; });
  return output;
}

}  // namespace

AdblockControllerImpl::AdblockControllerImpl(
    PrefService* service,
    scoped_refptr<AdblockPlatformEmbedder> embedder)
    : embedder_(std::move(embedder)), state_synchronizer_(embedder_.get()) {
  Init(service);
  state_synchronizer_.Init(service);
  embedder_->AddObserver(this);
}

AdblockControllerImpl::~AdblockControllerImpl() {
  embedder_->RemoveObserver(this);
}

void AdblockControllerImpl::Init(PrefService* pref_service) {
  prefs_ = pref_service;
  enable_adblock_.Init(adblock::prefs::kEnableAdblock, pref_service);
  enable_acceptable_ads_.Init(adblock::prefs::kEnableAcceptableAds,
                              pref_service);
  allowed_domains_.Init(adblock::prefs::kAdblockAllowedDomains, pref_service);
  custom_filters_.Init(adblock::prefs::kAdblockCustomFilters, pref_service);
  custom_subscriptions_.Init(adblock::prefs::kAdblockCustomSubscriptions,
                             pref_service);
  subscriptions_.Init(adblock::prefs::kAdblockSubscriptions, pref_service);
  allowed_connection_type_.Init(adblock::prefs::kAdblockAllowedConnectionType,
                                pref_service);
}

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
  enable_acceptable_ads_.SetValue(enabled);
}

bool AdblockControllerImpl::IsAcceptableAdsEnabled() const {
  return enable_acceptable_ads_.GetValue();
}

void AdblockControllerImpl::SelectBuiltInSubscription(const GURL& url) {
  adblock::AddToPref(url.spec(), &subscriptions_);
}

void AdblockControllerImpl::UnselectBuiltInSubscription(const GURL& url) {
  adblock::RemoveFromPref(url.spec(), &subscriptions_);
}

std::vector<GURL> AdblockControllerImpl::GetSelectedBuiltInSubscriptions()
    const {
  return ConvertURLs(subscriptions_.GetValue());
}

void AdblockControllerImpl::AddCustomSubscription(const GURL& url) {
  adblock::AddToPref(url.spec(), &custom_subscriptions_);
}

void AdblockControllerImpl::RemoveCustomSubscription(const GURL& url) {
  adblock::RemoveFromPref(url.spec(), &custom_subscriptions_);
}

std::vector<GURL> AdblockControllerImpl::GetCustomSubscriptions() const {
  return ConvertURLs(custom_subscriptions_.GetValue());
}

void AdblockControllerImpl::AddAllowedDomain(const std::string& domain) {
  adblock::AddToPref(domain, &allowed_domains_);
}

void AdblockControllerImpl::RemoveAllowedDomain(const std::string& domain) {
  adblock::RemoveFromPref(domain, &allowed_domains_);
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
  adblock::AddToPref(filter, &custom_filters_);
}

void AdblockControllerImpl::RemoveCustomFilter(const std::string& filter) {
  adblock::RemoveFromPref(filter, &custom_filters_);
}

std::vector<std::string> AdblockControllerImpl::GetCustomFilters() const {
  return custom_filters_.GetValue();
}

std::vector<Subscription> AdblockControllerImpl::GetRecommendedSubscriptions()
    const {
  std::vector<Subscription> result;
  auto list = prefs_->Get(prefs::kAdblockRecommendedSubscriptions)->GetList();
  for (auto& item : list) {
    GURL url;
    auto* url_str =
        item.FindStringKey(adblock::kRecommendedSubscriptionObjectUrlKey);
    if (url_str)
      url = GURL{*url_str};
    auto title =
        item.FindStringKey(adblock::kRecommendedSubscriptionObjectTitleKey)
            ? *item.FindStringKey(
                  adblock::kRecommendedSubscriptionObjectTitleKey)
            : "";

    auto* languages_str =
        item.FindStringKey(adblock::kRecommendedSubscriptionObjectLanguagesKey);
    std::vector<std::string> languages;
    if (languages_str)
      languages = adblock::utils::DeserializeLanguages(*languages_str);

    result.emplace_back(Subscription{url, title, languages});
  }

  return result;
}

void AdblockControllerImpl::OnFilterEngineCreated(
    const std::vector<Subscription>& recommended,
    const std::vector<GURL>& listed,
    const GURL& acceptable_ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ListPrefUpdate list(prefs_, prefs::kAdblockRecommendedSubscriptions);
  auto current_version = version_info::GetVersion();
  base::Version recommended_subscription_version(
      prefs_->GetString(prefs::kAdblockRecommendedSubscriptionsVersion));
  if (list->GetList().empty() ||
      current_version != recommended_subscription_version) {
    list->ClearList();
    for (const auto& subscription : recommended) {
      std::unique_ptr<base::Value> object =
          std::make_unique<base::DictionaryValue>();

      auto combined_languages =
          adblock::utils::SerializeLanguages(subscription.languages);

      object->SetStringKey(kRecommendedSubscriptionObjectUrlKey,
                           subscription.url.spec());
      object->SetStringKey(kRecommendedSubscriptionObjectLanguagesKey,
                           combined_languages);
      object->SetStringKey(kRecommendedSubscriptionObjectTitleKey,
                           subscription.title);
      list->Append(std::move(object));
    }
    prefs_->SetString(prefs::kAdblockRecommendedSubscriptionsVersion,
                      current_version.GetString());
  }
  for (auto& observer : observers_)
    observer.OnRecommendedSubscriptionsAvailable(recommended);
}

void AdblockControllerImpl::OnSubscriptionUpdated(const GURL& url) {
  for (auto& observer : observers_)
    observer.OnSubscriptionUpdated(url);
}

}  // namespace adblock

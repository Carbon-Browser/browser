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

#include "components/adblock/core/subscription/subscription_persistent_metadata_impl.h"

#include "base/json/values_util.h"
#include "base/strings/string_piece_forward.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace adblock {
namespace {
constexpr base::StringPiece kExpirationTimeKey = "expiration_time";
constexpr base::StringPiece kLastInstallationTimeKey = "last_installation_time";
constexpr base::StringPiece kVersionKey = "version";
constexpr base::StringPiece kDownloadCountKey = "download_count";
constexpr base::StringPiece kErrorCountKey = "error_count";
}  // namespace

struct SubscriptionPersistentMetadataImpl::Metadata {
  base::Time expiration_time;
  base::Time last_installation_time;
  std::string version{"0"};
  int download_count{0};
  int error_count{0};
};

SubscriptionPersistentMetadataImpl::SubscriptionPersistentMetadataImpl(
    PrefService* prefs)
    : prefs_(prefs) {
  LoadFromPrefs();
}

SubscriptionPersistentMetadataImpl::~SubscriptionPersistentMetadataImpl() =
    default;

void SubscriptionPersistentMetadataImpl::SetExpirationInterval(
    const GURL& subscription_url,
    base::TimeDelta expires_in) {
  const auto now = base::Time::Now();
  metadata_map_[subscription_url].last_installation_time = now;
  metadata_map_[subscription_url].expiration_time = now + expires_in;
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::SetVersion(
    const GURL& subscription_url,
    std::string version) {
  metadata_map_[subscription_url].version = std::move(version);
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::IncrementDownloadSuccessCount(
    const GURL& subscription_url) {
  metadata_map_[subscription_url].download_count++;
  metadata_map_[subscription_url].error_count = 0;
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::IncrementDownloadErrorCount(
    const GURL& subscription_url) {
  metadata_map_[subscription_url].error_count++;
  UpdatePrefs();
}

bool SubscriptionPersistentMetadataImpl::IsExpired(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end())
    return true;
  return it->second.expiration_time <= base::Time::Now();
}

base::Time SubscriptionPersistentMetadataImpl::GetLastInstallationTime(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end())
    return base::Time();
  return it->second.last_installation_time;
}

std::string SubscriptionPersistentMetadataImpl::GetVersion(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end())
    return "0";
  return it->second.version;
}

int SubscriptionPersistentMetadataImpl::GetDownloadSuccessCount(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end())
    return 0;
  return it->second.download_count;
}

int SubscriptionPersistentMetadataImpl::GetDownloadErrorCount(
    const GURL& subscription_url) const {
  auto it = metadata_map_.find(subscription_url);
  if (it == metadata_map_.end())
    return 0;
  return it->second.error_count;
}

void SubscriptionPersistentMetadataImpl::RemoveMetadata(
    const GURL& subscription_url) {
  metadata_map_.erase(subscription_url);
  UpdatePrefs();
}

void SubscriptionPersistentMetadataImpl::UpdatePrefs() {
  base::Value dict(base::Value::Type::DICTIONARY);
  for (const auto& pair : metadata_map_) {
    base::Value subscription(base::Value::Type::DICTIONARY);
    subscription.SetKey(kExpirationTimeKey,
                        TimeToValue(pair.second.expiration_time));
    subscription.SetKey(kLastInstallationTimeKey,
                        TimeToValue(pair.second.last_installation_time));
    subscription.SetStringKey(kVersionKey, pair.second.version);
    subscription.SetIntKey(kDownloadCountKey, pair.second.download_count);
    subscription.SetIntKey(kErrorCountKey, pair.second.error_count);
    dict.SetKey(pair.first.spec(), std::move(subscription));
  }
  prefs_->Set(prefs::kSubscriptionMetadata, std::move(dict));
}

void SubscriptionPersistentMetadataImpl::LoadFromPrefs() {
  const base::Value* dict = prefs_->Get(prefs::kSubscriptionMetadata);
  DCHECK(dict);
  DCHECK(dict->is_dict());
  for (const auto dict_item : dict->DictItems()) {
    Metadata subscription;
    subscription.expiration_time =
        ValueToTime(dict_item.second.FindKey(kExpirationTimeKey))
            .value_or(base::Time());
    subscription.last_installation_time =
        ValueToTime(dict_item.second.FindKey(kLastInstallationTimeKey))
            .value_or(base::Time());
    const auto* version = dict_item.second.FindStringKey(kVersionKey);
    if (version)
      subscription.version = *version;
    subscription.error_count =
        dict_item.second.FindIntKey(kErrorCountKey).value_or(0);
    subscription.download_count =
        dict_item.second.FindIntKey(kDownloadCountKey).value_or(0);
    metadata_map_.emplace(dict_item.first, std::move(subscription));
  }
}

}  // namespace adblock

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

#include "components/adblock/fake_subscription_impl.h"

#include "base/notreached.h"

namespace adblock {

FakeSubscriptionImpl::FakeSubscriptionImpl(
    const GURL& url,
    const std::string& title,
    const std::vector<std::string>& languages,
    const std::string& status,
    bool is_aa,
    bool* has_updated_filters)
    : disabled_(false),
      is_aa_(is_aa),
      url_(url),
      title_(title),
      languages_(languages),
      status_(status),
      has_updated_filters_(has_updated_filters) {}

FakeSubscriptionImpl::~FakeSubscriptionImpl() = default;

bool FakeSubscriptionImpl::IsDisabled() const {
  return disabled_;
}

void FakeSubscriptionImpl::SetDisabled(bool disabled) {
  disabled_ = disabled;
}

void FakeSubscriptionImpl::UpdateFilters() {
  if (has_updated_filters_) {
    *has_updated_filters_ = true;
  }
}

bool FakeSubscriptionImpl::IsUpdating() const {
  return false;
}

bool FakeSubscriptionImpl::IsAA() const {
  return is_aa_;
}

std::string FakeSubscriptionImpl::GetTitle() const {
  return title_;
}

std::string FakeSubscriptionImpl::GetUrl() const {
  return url_.spec();
}

std::string FakeSubscriptionImpl::GetHomepage() const {
  return {};
}

std::string FakeSubscriptionImpl::GetAuthor() const {
  return {};
}

std::vector<std::string> FakeSubscriptionImpl::GetLanguages() const {
  return languages_;
}

int FakeSubscriptionImpl::GetFilterCount() const {
  return 1;
}

std::string FakeSubscriptionImpl::GetSynchronizationStatus() const {
  return status_;
}

int FakeSubscriptionImpl::GetLastDownloadAttemptTime() const {
  return 0;
}

int FakeSubscriptionImpl::GetLastDownloadSuccessTime() const {
  return 0;
}

bool FakeSubscriptionImpl::operator==(
    const AdblockPlus::ISubscriptionImplementation& other) const {
  return GetUrl() == other.GetUrl();
}

std::unique_ptr<AdblockPlus::ISubscriptionImplementation>
FakeSubscriptionImpl::Clone() const {
  auto copy = std::make_unique<FakeSubscriptionImpl>(
      url_, title_, languages_, status_, is_aa_, has_updated_filters_);
  copy->SetDisabled(disabled_);
  return copy;
}

int FakeSubscriptionImpl::GetVersion() const {
  return 0;
}

}  // namespace adblock

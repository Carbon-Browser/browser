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
 * along with Adblock Plus.  If not see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_FAKE_SUBSCRIPTION_IMPL_H_
#define COMPONENTS_ADBLOCK_FAKE_SUBSCRIPTION_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/ISubscriptionImplementation.h"
#include "url/gurl.h"

namespace adblock {

class FakeSubscriptionImpl
    : public testing::NiceMock<AdblockPlus::ISubscriptionImplementation> {
 public:
  FakeSubscriptionImpl(const GURL& url,
                       const std::string& title,
                       const std::vector<std::string>& languages,
                       const std::string& status,
                       bool is_aa,
                       bool* has_updated_filters = nullptr);
  ~FakeSubscriptionImpl() override;
  bool IsDisabled() const override;
  void SetDisabled(bool disabled) override;
  void UpdateFilters() override;
  bool IsUpdating() const override;
  bool IsAA() const override;
  std::string GetTitle() const override;
  std::string GetUrl() const override;
  std::string GetHomepage() const override;
  std::string GetAuthor() const override;
  std::vector<std::string> GetLanguages() const override;
  int GetFilterCount() const override;
  std::string GetSynchronizationStatus() const override;
  int GetLastDownloadAttemptTime() const override;
  int GetLastDownloadSuccessTime() const override;
  std::unique_ptr<AdblockPlus::ISubscriptionImplementation> Clone()
      const override;
  bool operator==(
      const AdblockPlus::ISubscriptionImplementation& other) const override;
  int GetVersion() const override;

 private:
  bool disabled_;
  bool is_aa_;
  GURL url_;
  std::string title_;
  std::vector<std::string> languages_;
  std::string status_;
  bool* has_updated_filters_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_FAKE_SUBSCRIPTION_IMPL_H_

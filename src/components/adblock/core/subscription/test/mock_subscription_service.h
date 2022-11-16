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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_

#include "components/adblock/core/subscription/subscription_service.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace adblock {

class MockSubscriptionService : public SubscriptionService {
 public:
  MockSubscriptionService();
  ~MockSubscriptionService() override;
  MOCK_METHOD(bool, IsInitialized, (), (override, const));
  MOCK_METHOD(void, RunWhenInitialized, (base::OnceClosure task), (override));
  MOCK_METHOD(std::vector<scoped_refptr<Subscription>>,
              GetCurrentSubscriptions,
              (),
              (override, const));
  MOCK_METHOD(std::unique_ptr<SubscriptionCollection>,
              GetCurrentSnapshot,
              (),
              (override, const));
  MOCK_METHOD(void,
              DownloadAndInstallSubscription,
              (const GURL& subscription_url,
               SubscriptionService::InstallationCallback on_finished),
              (override));
  MOCK_METHOD(void,
              PingAcceptableAds,
              (SubscriptionService::PingCallback on_finished),
              (override));
  MOCK_METHOD(void,
              UninstallSubscription,
              (const GURL& subscription_url),
              (override));
  MOCK_METHOD(void,
              SetCustomFilters,
              (const std::vector<std::string>& filters),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_TEST_MOCK_SUBSCRIPTION_SERVICE_H_

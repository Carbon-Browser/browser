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

#ifndef COMPONENTS_ADBLOCK_CORE_TEST_MOCK_ADBLOCK_CONTROLLER_H_
#define COMPONENTS_ADBLOCK_CORE_TEST_MOCK_ADBLOCK_CONTROLLER_H_

#include "base/observer_list.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/adblock/core/common/allowed_connection_type.h"
#include "components/adblock/core/subscription/installed_subscription.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace adblock {

class MockAdblockController : public AdblockController {
 public:
  MockAdblockController();
  ~MockAdblockController() override;

  void AddObserver(AdblockController::Observer* observer) override;
  void RemoveObserver(AdblockController::Observer* observer) override;
  MOCK_METHOD(void, SetAdblockEnabled, (bool enabled), (override));
  MOCK_METHOD(bool, IsAdblockEnabled, (), (const override));
  MOCK_METHOD(void, SetAcceptableAdsEnabled, (bool enabled), (override));
  MOCK_METHOD(bool, IsAcceptableAdsEnabled, (), (const override));
  MOCK_METHOD(void, SelectBuiltInSubscription, (const GURL& url), (override));
  MOCK_METHOD(void, UnselectBuiltInSubscription, (const GURL& url), (override));
  MOCK_METHOD(std::vector<scoped_refptr<Subscription>>,
              GetSelectedBuiltInSubscriptions,
              (),
              (const, override));
  MOCK_METHOD(void, AddCustomSubscription, (const GURL& url), (override));
  MOCK_METHOD(void, RemoveCustomSubscription, (const GURL& url), (override));
  MOCK_METHOD(std::vector<scoped_refptr<Subscription>>,
              GetCustomSubscriptions,
              (),
              (const, override));
  MOCK_METHOD(void, AddAllowedDomain, (const std::string& domain), (override));
  MOCK_METHOD(void,
              RemoveAllowedDomain,
              (const std::string& domain),
              (override));
  MOCK_METHOD(std::vector<std::string>,
              GetAllowedDomains,
              (),
              (const, override));
  MOCK_METHOD(std::vector<KnownSubscriptionInfo>,
              GetKnownSubscriptions,
              (),
              (const, override));
  MOCK_METHOD(void,
              SetUpdateConsent,
              (AllowedConnectionType consent),
              (override));
  MOCK_METHOD(AllowedConnectionType, GetUpdateConsent, (), (const override));
  MOCK_METHOD(void, AddCustomFilter, (const std::string& filter), (override));
  MOCK_METHOD(void,
              RemoveCustomFilter,
              (const std::string& filter),
              (override));
  MOCK_METHOD(std::vector<std::string>,
              GetCustomFilters,
              (),
              (const, override));

  base::ObserverList<AdblockController::Observer> observers_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_TEST_MOCK_ADBLOCK_CONTROLLER_H_

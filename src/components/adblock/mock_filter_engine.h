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

#ifndef COMPONENTS_ADBLOCK_MOCK_FILTER_ENGINE_H_
#define COMPONENTS_ADBLOCK_MOCK_FILTER_ENGINE_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

namespace adblock {

class MockFilterEngine : public testing::NiceMock<AdblockPlus::IFilterEngine> {
 public:
  MockFilterEngine();
  ~MockFilterEngine() override;
  MOCK_METHOD(void, StartSynchronization, (), (override));
  MOCK_METHOD(void, StopSynchronization, (), (override));
  MOCK_METHOD(AdblockPlus::Filter,
              GetFilter,
              (const std::string&),
              (const, override));
  MOCK_METHOD(AdblockPlus::Subscription,
              GetSubscription,
              (const std::string&),
              (const, override));
  MOCK_METHOD(std::vector<AdblockPlus::Filter>,
              GetListedFilters,
              (),
              (const, override));
  MOCK_METHOD(std::vector<AdblockPlus::Subscription>,
              GetListedSubscriptions,
              (),
              (const, override));
  MOCK_METHOD(std::vector<AdblockPlus::Subscription>,
              FetchAvailableSubscriptions,
              (),
              (const, override));
  MOCK_METHOD(void, SetAAEnabled, (bool), (override));
  MOCK_METHOD(bool, IsAAEnabled, (), (const, override));
  MOCK_METHOD(std::string, GetAAUrl, (), (const, override));
  MOCK_METHOD(AdblockPlus::Filter,
              Matches,
              (const std::string&,
               ContentTypeMask,
               const std::string&,
               const std::string&,
               bool),
              (const, override));
  MOCK_METHOD(bool,
              IsContentAllowlisted,
              (const std::string&,
               ContentTypeMask,
               const std::vector<std::string>&,
               const std::string&),
              (const, override));
  MOCK_METHOD(std::string,
              GetElementHidingStyleSheet,
              (const std::string&, bool),
              (const, override));
  MOCK_METHOD(std::vector<EmulationSelector>,
              GetElementHidingEmulationSelectors,
              (const std::string&),
              (const, override));
  MOCK_METHOD(void,
              AddEventObserver,
              (IFilterEngine::EventObserver*),
              (override));
  MOCK_METHOD(void,
              RemoveEventObserver,
              (IFilterEngine::EventObserver*),
              (override));
  MOCK_METHOD(void, SetAllowedConnectionType, (const std::string*), (override));
  MOCK_METHOD(std::unique_ptr<std::string>,
              GetAllowedConnectionType,
              (),
              (const, override));
  MOCK_METHOD(bool,
              VerifySignature,
              (const std::string&,
               const std::string&,
               const std::string&,
               const std::string&,
               const std::string&),
              (const, override));
  MOCK_METHOD(std::vector<std::string>,
              ComposeFilterSuggestions,
              (const AdblockPlus::IElement*),
              (const, override));
  MOCK_METHOD(void,
              AddSubscription,
              (const AdblockPlus::Subscription&),
              (override));
  MOCK_METHOD(void,
              RemoveSubscription,
              (const AdblockPlus::Subscription&),
              (override));
  MOCK_METHOD(void, AddFilter, (const AdblockPlus::Filter&), (override));
  MOCK_METHOD(void, RemoveFilter, (const AdblockPlus::Filter&), (override));

  MOCK_METHOD(std::vector<AdblockPlus::Subscription>,
              GetSubscriptionsFromFilter,
              (const AdblockPlus::Filter& filter),
              (const, override));
  MOCK_METHOD(std::string,
              GetSnippetScript,
              (const std::string& documentUrl,
               const std::string& librarySource),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_MOCK_FILTER_ENGINE_H_

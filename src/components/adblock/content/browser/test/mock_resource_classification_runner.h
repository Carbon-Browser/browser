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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_RESOURCE_CLASSIFICATION_RUNNER_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_RESOURCE_CLASSIFICATION_RUNNER_

#include "components/adblock/content/browser/resource_classification_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockResourceClassificationRunner : public ResourceClassificationRunner {
 public:
  MockResourceClassificationRunner();
  ~MockResourceClassificationRunner() override;

  MOCK_METHOD(void,
              AddObserver,
              (ResourceClassificationRunner::Observer*),
              (override));
  MOCK_METHOD(void,
              RemoveObserver,
              (ResourceClassificationRunner::Observer*),
              (override));

  MOCK_METHOD((mojom::FilterMatchResult),
              ShouldBlockPopup,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL&,
               const GURL&,
               content::RenderFrameHost*),
              (override));

  MOCK_METHOD(void,
              CheckRequestFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL&,
               int32_t,
               int32_t,
               int32_t,
               mojom::AdblockInterface::CheckFilterMatchCallback),
              (override));

  MOCK_METHOD(void,
              CheckRequestFilterMatchForWebSocket,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL&,
               content::RenderFrameHost*,
               mojom::AdblockInterface::CheckFilterMatchCallback),
              (override));

  MOCK_METHOD(void,
              CheckResponseFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL&,
               int32_t,
               int32_t,
               const scoped_refptr<net::HttpResponseHeaders>&,
               mojom::AdblockInterface::CheckFilterMatchCallback),
              (override));

  MOCK_METHOD(void,
              CheckRewriteFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL&,
               int32_t,
               int32_t,
               base::OnceCallback<void(const absl::optional<GURL>&)>),
              (override));

  base::ObserverList<ResourceClassificationRunner::Observer> observers_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_TEST_MOCK_RESOURCE_CLASSIFICATION_RUNNER_

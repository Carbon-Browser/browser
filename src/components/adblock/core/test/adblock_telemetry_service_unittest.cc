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

#include "components/adblock/core/adblock_telemetry_service.h"

#include <memory>

#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Return;

namespace adblock {
namespace {

class MockTopicProvider : public AdblockTelemetryService::TopicProvider {
 public:
  MOCK_METHOD(GURL, GetEndpointURL, (), (const, override));
  MOCK_METHOD(std::string, GetAuthToken, (), (const, override));
  MOCK_METHOD(std::string, GetPayload, (), (const, override));
  MOCK_METHOD(base::TimeDelta, GetTimeToNextRequest, (), (const, override));
  MOCK_METHOD(void, ParseResponse, (std::unique_ptr<std::string>), (override));
};
}  // namespace

class AdblockTelemetryServiceTest : public testing::Test {
 public:
  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    telemetry_service_ = std::make_unique<AdblockTelemetryService>(
        &pref_service_, test_shared_url_loader_factory_);
  }

  MockTopicProvider* RegisterFooMock() {
    auto* mock_provider = RegisterNewMockTopicProvider();
    ON_CALL(*mock_provider, GetEndpointURL()).WillByDefault(Return(kFooUrl));
    ON_CALL(*mock_provider, GetAuthToken()).WillByDefault(Return("foo_token"));
    ON_CALL(*mock_provider, GetTimeToNextRequest())
        .WillByDefault(Return(kFooDelay));
    ON_CALL(*mock_provider, GetPayload()).WillByDefault(Return("foo_data"));
    return mock_provider;
  }

  void ExpectFooMadeRequest(int pending_request_idx) {
    const auto& resource_request =
        test_url_loader_factory_.GetPendingRequest(pending_request_idx)
            ->request;
    EXPECT_EQ(resource_request.url, kFooUrl);
    AssertRequestContainsRequiredData(resource_request, "foo_data",
                                      "foo_token");
  }

  MockTopicProvider* RegisterBarMock() {
    auto* mock_provider = RegisterNewMockTopicProvider();
    ON_CALL(*mock_provider, GetEndpointURL()).WillByDefault(Return(kBarUrl));
    ON_CALL(*mock_provider, GetAuthToken()).WillByDefault(Return("bar_token"));
    ON_CALL(*mock_provider, GetTimeToNextRequest())
        .WillByDefault(Return(kBarDelay));
    ON_CALL(*mock_provider, GetPayload()).WillByDefault(Return("bar_data"));
    return mock_provider;
  }

  void ExpectBarMadeRequest(int pending_request_idx) {
    const auto& resource_request =
        test_url_loader_factory_.GetPendingRequest(pending_request_idx)
            ->request;
    EXPECT_EQ(resource_request.url, kBarUrl);
    AssertRequestContainsRequiredData(resource_request, "bar_data",
                                      "bar_token");
  }

 protected:
  MockTopicProvider* RegisterNewMockTopicProvider() {
    auto provider = std::make_unique<MockTopicProvider>();
    auto* provider_bare_ptr = provider.get();
    telemetry_service_->AddTopicProvider(std::move(provider));
    return provider_bare_ptr;
  }

  void AssertRequestContainsRequiredData(
      const network::ResourceRequest& resource_request,
      const std::string& expected_upload_data,
      const std::string& expected_auth_token) {
    // Cookies are not sent nor stored:
    EXPECT_FALSE(resource_request.SavesCookies());
    EXPECT_FALSE(resource_request.SendsCookies());

    // Authorization token is being sent:
    std::string auth_header;
    ASSERT_TRUE(resource_request.headers.GetHeader(
        net::HttpRequestHeaders::kAuthorization, &auth_header));
    EXPECT_EQ(auth_header, "Bearer " + expected_auth_token);

    // "Accept: application/json" is sent.
    std::string accept_header;
    ASSERT_TRUE(resource_request.headers.GetHeader(
        net::HttpRequestHeaders::kAccept, &accept_header));
    EXPECT_EQ(accept_header, "application/json");

    // Cache is being bypassed:
    EXPECT_EQ(resource_request.load_flags,
              net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE);

    // Payload is being sent:
    std::string upload_data;
    for (const auto& elem : *(resource_request.request_body->elements())) {
      auto piece = elem.As<network::DataElementBytes>().AsStringPiece();
      upload_data.append(piece.data(), piece.size());
    }
    EXPECT_EQ(upload_data, expected_upload_data);
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory>
      test_shared_url_loader_factory_{
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              &test_url_loader_factory_)};
  constexpr static base::TimeDelta kFooDelay{base::Seconds(5)};
  constexpr static base::TimeDelta kBarDelay{base::Seconds(6)};
  const GURL kFooUrl{"https://telemetry.com/topic/eyeo_foo/version/3"};
  const GURL kBarUrl{"https://telemetry.com/topic/eyeo_bar/version/2"};

  std::unique_ptr<AdblockTelemetryService> telemetry_service_;
};

TEST_F(AdblockTelemetryServiceTest,
       ScheduleStartsImmediatelyWhenAdblockEnabled) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  RegisterFooMock();

  telemetry_service_->Start();

  // No requests initially.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // The topic should have scheduled a request according to its
  // GetTimeToNextRequest().
  task_environment_.FastForwardBy(kBarDelay);

  // A request was sent to a URL built according to topic provider's data.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, ScheduleStartupDelayedWhenAdblockDisabled) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);

  RegisterFooMock();

  telemetry_service_->Start();

  // No requests initially.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // A lot of time passes.
  task_environment_.FastForwardBy(kBarDelay * 5);
  // There was no network request made, because prefs::kEnableAdblock is false.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // kEnableAdblock becomes true:
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  // Schedule is started, first request made after kBarDelay.
  task_environment_.FastForwardBy(kBarDelay);

  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

TEST_F(AdblockTelemetryServiceTest, ScheduleAbortedWhenAdblockDisabled) {
  // Schedule starts normally, without delay:
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  RegisterFooMock();
  telemetry_service_->Start();

  // User disables Adblocking.
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);

  // A lot of time passes with no requests, schedule is stopped.
  task_environment_.FastForwardBy(kBarDelay * 5);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);
}

TEST_F(AdblockTelemetryServiceTest, MultipleProvidersMakeRequests) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  RegisterFooMock();
  RegisterBarMock();

  telemetry_service_->Start();

  // No requests initially.
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 0);

  // Time for Foo topic provider to make a request.
  task_environment_.FastForwardBy(kFooDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  // Time for Bar topic provider to make a request.
  task_environment_.FastForwardBy(kBarDelay - kFooDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 2);
  ExpectBarMadeRequest(1);
}

TEST_F(AdblockTelemetryServiceTest, SuccessfulResponseReceived) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  auto* mock = RegisterFooMock();

  telemetry_service_->Start();

  task_environment_.FastForwardBy(kFooDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  const std::string response = "response_content";

  // Response will trigger TopicProvider's ParseResponse().
  EXPECT_CALL(*mock, ParseResponse(testing::Pointee(response)));
  test_url_loader_factory_.SimulateResponseForPendingRequest(kFooUrl.spec(),
                                                             response);
}

TEST_F(AdblockTelemetryServiceTest, Non200ResponseStillParsed) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  auto* mock = RegisterFooMock();

  telemetry_service_->Start();

  task_environment_.FastForwardBy(kFooDelay);
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  const std::string response = "error_content";

  // Even with a non-200 response code, TopicProvider is still shown the body
  // of the response. The response may contain an error description which the
  // TopicProvider may want to parse.
  EXPECT_CALL(*mock, ParseResponse(testing::Pointee(response)));
  test_url_loader_factory_.SimulateResponseForPendingRequest(
      kFooUrl.spec(), response, net::HTTP_FORBIDDEN);
}

TEST_F(AdblockTelemetryServiceTest, RequestAbortedWhenAdblockDisabled) {
  // Start schedule normally:
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);
  auto* mock = RegisterFooMock();
  telemetry_service_->Start();
  task_environment_.FastForwardBy(kFooDelay);
  // Request is made:
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);

  // Adblocking is disabled before respnose arrives:
  pref_service_.SetBoolean(prefs::kEnableAdblock, false);

  // Now, TopicProvider is not triggered even when response arrives.
  EXPECT_CALL(*mock, ParseResponse(testing::_)).Times(0);

  test_url_loader_factory_.SimulateResponseForPendingRequest(kFooUrl.spec(),
                                                             "response");
}

TEST_F(AdblockTelemetryServiceTest, NegativeTimeToNextRequest) {
  pref_service_.SetBoolean(prefs::kEnableAdblock, true);

  auto* mock = RegisterFooMock();
  // TopicProvider returns a negative time to next request, as if the request
  // was supposed to happen in the past. This is a normal scenario, ex. when
  // the browser was shut down for longer than the duration of request interval.
  EXPECT_CALL(*mock, GetTimeToNextRequest())
      .WillOnce(testing::Return(-base::Seconds(30)));

  telemetry_service_->Start();

  // Request is scheduled immediately.
  task_environment_.FastForwardBy(base::TimeDelta());
  // Request is made:
  ASSERT_EQ(test_url_loader_factory_.NumPending(), 1);
  ExpectFooMadeRequest(0);
}

}  // namespace adblock

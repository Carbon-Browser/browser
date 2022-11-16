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

#include "components/adblock/core/activeping_telemetry_topic_provider.h"

#include <string>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/strings/string_piece_forward.h"
#include "base/system/sys_info.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/prefs/pref_value_store.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

enum class AcceptableAds { Enabled, Disabled };

class AdblockActivepingTelemetryTopicProviderTest
    : public testing::TestWithParam<AcceptableAds> {
 public:
  void SetUp() override {
    app_info_.name = "MyApp";
    app_info_.version = "1234";
    app_info_.client_os = "UNUSED";  // base/sys_info.h used instead.
    prefs::RegisterProfilePrefs(pref_service_.registry());
    pref_service_.SetBoolean(prefs::kEnableAcceptableAds,
                             AcceptableAdsEnabled());
    RecreateProvider();
  }

  bool AcceptableAdsEnabled() const {
    return GetParam() == AcceptableAds::Enabled;
  }

  void RecreateProvider() {
    provider_ = std::make_unique<ActivepingTelemetryTopicProvider>(
        app_info_, &pref_service_, kBaseUrl, kAuthToken);
  }

  void ExpectPayloadAndTimeConsistentAfterRestart() {
    // Current state of provider_ is stored persistently and remains consitent
    // after recreating the provider_.
    const auto time_to_next_request = provider_->GetTimeToNextRequest();
    const auto payload = provider_->GetPayload();
    RecreateProvider();
    EXPECT_EQ(time_to_next_request, provider_->GetTimeToNextRequest());
    EXPECT_EQ(payload, provider_->GetPayload());
  }

  void ExpectPayloadContainsValue(const std::string& json,
                                  std::string key,
                                  base::Value expected_value) {
    auto root = base::JSONReader::Read(json);
    ASSERT_TRUE(root) << "JSON is invalid";
    auto* value = root->FindPath("payload." + key);
    ASSERT_TRUE(value);
    EXPECT_EQ(*value, expected_value);
  }

  void ExpectPayloadDoesNotContainValue(const std::string& json,
                                        std::string key) {
    auto value = base::JSONReader::Read(json);
    ASSERT_TRUE(value) << "JSON is invalid";
    EXPECT_FALSE(value->FindPath("payload." + key));
  }

  void ExpectPayloadContainsRequiredStaticValues(const std::string& json) {
    ExpectPayloadContainsValue(json, "aa_active",
                               base::Value(AcceptableAdsEnabled()));
    ExpectPayloadContainsValue(
        json, "platform", base::Value(base::SysInfo::OperatingSystemName()));
    ExpectPayloadContainsValue(
        json, "platform_version",
        base::Value(base::SysInfo::OperatingSystemVersion()));
    ExpectPayloadContainsValue(json, "application",
                               base::Value(app_info_.name));
    ExpectPayloadContainsValue(json, "application_version",
                               base::Value(app_info_.version));
    ExpectPayloadContainsValue(json, "addon_name",
                               base::Value("eyeo-chromium-sdk"));
    ExpectPayloadContainsValue(json, "addon_version", base::Value("2.0.0"));
  }

  void ExpectLastPingTagValid(const std::string& json,
                              base::GUID* parsed_tag = nullptr) {
    auto root = base::JSONReader::Read(json);
    ASSERT_TRUE(root);
    const std::string* tag = root->FindStringPath("payload.last_ping_tag");
    ASSERT_TRUE(tag);
    const auto uuid = base::GUID::ParseLowercase(*tag);
    EXPECT_TRUE(uuid.is_valid());
    if (parsed_tag)
      *parsed_tag = uuid;
  }

  void ExpectFailureAndRetryForResponse(
      std::unique_ptr<std::string> bad_response_contents) {
    const std::string first_attempt_payload = provider_->GetPayload();
    provider_->ParseResponse(std::move(bad_response_contents));

    // Next ping after shorter delay, since the previous one failed:
    EXPECT_EQ(provider_->GetTimeToNextRequest(), kRetryPingInterval);

    // Payload is the same as after first ping.
    const std::string retry_payload = provider_->GetPayload();
    EXPECT_EQ(first_attempt_payload, retry_payload);
    ExpectPayloadAndTimeConsistentAfterRestart();
  }

  static constexpr base::TimeDelta kInitialDelay = base::Seconds(10);
  static constexpr base::TimeDelta kNormalPingInterval = base::Hours(8);
  static constexpr base::TimeDelta kRetryPingInterval = base::Hours(1);
  const GURL kBaseUrl{"https://telemetry.com/"};
  const std::string kAuthToken{"AUTH_TOKEN"};
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  utils::AppInfo app_info_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ActivepingTelemetryTopicProvider> provider_;
};

TEST_P(AdblockActivepingTelemetryTopicProviderTest, StaticFields) {
  EXPECT_EQ(provider_->GetEndpointURL(),
            "https://telemetry.com/topic/eyeochromium_activeping/version/1");
  EXPECT_EQ(provider_->GetAuthToken(), kAuthToken);
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, DefaultBaseUrl) {
  EXPECT_EQ(ActivepingTelemetryTopicProvider::DefaultBaseUrl(),
            GURL(EYEO_TELEMETRY_SERVER_URL));
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, DefaultAuthToken) {
#if defined(EYEO_TELEMETRY_ACTIVEPING_AUTH_TOKEN)
  EXPECT_EQ(ActivepingTelemetryTopicProvider::DefaultAuthToken(),
            EYEO_TELEMETRY_ACTIVEPING_AUTH_TOKEN);
#else
  EXPECT_EQ(ActivepingTelemetryTopicProvider::DefaultAuthToken(), "");
#endif
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, FirstRun) {
  // On first run, next ping should happen after an initial delay.
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kInitialDelay);

  // There are no values for "last_ping", "previous_last_ping", "last_ping_tag"
  // and "first_ping". But there are values for other required fields:
  const std::string payload = provider_->GetPayload();
  ExpectPayloadContainsRequiredStaticValues(payload);
  ExpectPayloadDoesNotContainValue(payload, "last_ping");
  ExpectPayloadDoesNotContainValue(payload, "previous_last_ping");
  ExpectPayloadDoesNotContainValue(payload, "last_ping_tag");
  ExpectPayloadDoesNotContainValue(payload, "first_ping");
  ExpectPayloadAndTimeConsistentAfterRestart();
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, FirstRunSucceeded) {
  // Successful server response:
  auto response = std::make_unique<std::string>(R"({"token":"Monday"})");
  provider_->ParseResponse(std::move(response));

  // Next ping after normal delay, since the previous one succeeded:
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kNormalPingInterval);

  // Payload now contains "first_ping" and "last_ping" set to "Monday", as this
  // was the "token" received from server:
  const std::string payload = provider_->GetPayload();
  ExpectPayloadContainsRequiredStaticValues(payload);
  ExpectPayloadContainsValue(payload, "last_ping", base::Value("Monday"));
  ExpectPayloadContainsValue(payload, "first_ping", base::Value("Monday"));
  // There's no "previous_last_ping" because we need at least two successful
  // pings for that.
  ExpectPayloadDoesNotContainValue(payload, "previous_last_ping");

  // "last_ping_tag" is some valid UUID4.
  ExpectLastPingTagValid(payload);
  ExpectPayloadAndTimeConsistentAfterRestart();
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest,
       FirstRunFailedDueToErrorInResponse) {
  // "error" in server response:
  ExpectFailureAndRetryForResponse(
      std::make_unique<std::string>(R"({"error":"1111","token":"Monday"})"));
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest,
       FirstRunFailedDueToNoResponse) {
  // No server response:
  ExpectFailureAndRetryForResponse(nullptr);
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest,
       FirstRunFailedDueToNoTokenInResponse) {
  // No "token" in server response:
  ExpectFailureAndRetryForResponse(
      std::make_unique<std::string>(R"({"data":"1111"})"));
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest,
       FirstRunFailedDueToInvalidJsonResponse) {
  // Not a valid JSON:
  ExpectFailureAndRetryForResponse(
      std::make_unique<std::string>(R"(rubbish data)"));
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, SecondRunSucceeded) {
  // Successful first server response:
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Monday"})"));

  // Store first last_ping_tag to compare against the next one:
  base::GUID first_response_uuid;
  ExpectLastPingTagValid(provider_->GetPayload(), &first_response_uuid);

  // Next ping after normal delay, since the previous one succeeded:
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kNormalPingInterval);

  task_environment_.FastForwardBy(kNormalPingInterval);

  // Successful second server response:
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Tuesday"})"));
  ExpectPayloadAndTimeConsistentAfterRestart();

  // Payload now contains all ping dates set.
  const std::string payload = provider_->GetPayload();
  ExpectPayloadContainsRequiredStaticValues(payload);
  ExpectPayloadContainsValue(payload, "last_ping", base::Value("Tuesday"));
  ExpectPayloadContainsValue(payload, "previous_last_ping",
                             base::Value("Monday"));
  ExpectPayloadContainsValue(payload, "first_ping", base::Value("Monday"));

  base::GUID second_response_uuid;
  ExpectLastPingTagValid(payload, &second_response_uuid);

  // Different tag was generated:
  EXPECT_NE(second_response_uuid, first_response_uuid);
  ExpectPayloadAndTimeConsistentAfterRestart();
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest,
       PreviousLastPingTracksCorrectly) {
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Monday"})"));
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Tuesday"})"));
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Wednesday"})"));
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Thursday"})"));

  const std::string payload = provider_->GetPayload();
  ExpectPayloadContainsRequiredStaticValues(payload);
  // "last_ping" is the one received most recently.
  ExpectPayloadContainsValue(payload, "last_ping", base::Value("Thursday"));
  // "previous_last_ping" is the one received before "last_ping".
  ExpectPayloadContainsValue(payload, "previous_last_ping",
                             base::Value("Wednesday"));
  // "first_ping" never changes, after the initial successful response.
  ExpectPayloadContainsValue(payload, "first_ping", base::Value("Monday"));
  // Note: "Wednesday" does not appear in results, it would be a
  // "previous_previous_last_ping" which we don't track.
  ExpectPayloadAndTimeConsistentAfterRestart();
}

TEST_P(AdblockActivepingTelemetryTopicProviderTest, TimeToNextPingAfterDelay) {
  // At first, require next request to happen after an initial delay.
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kInitialDelay);
  task_environment_.FastForwardBy(kInitialDelay);
  provider_->ParseResponse(
      std::make_unique<std::string>(R"({"token":"Monday"})"));
  // After a success, next ping should happen after a normal delay.
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kNormalPingInterval);

  // After half of the time passes, another half remains:
  const auto kHalfInterval = kNormalPingInterval / 2;
  task_environment_.FastForwardBy(kHalfInterval);
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kHalfInterval);

  // Too much time passed, "negative" remaining delay is clamped to initial
  // delay.
  task_environment_.FastForwardBy(kNormalPingInterval * 3);
  EXPECT_EQ(provider_->GetTimeToNextRequest(), kInitialDelay);
  ExpectPayloadAndTimeConsistentAfterRestart();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockActivepingTelemetryTopicProviderTest,
                         testing::Values(AcceptableAds::Enabled,
                                         AcceptableAds::Disabled));
}  // namespace adblock

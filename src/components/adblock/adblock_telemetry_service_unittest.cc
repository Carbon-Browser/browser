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

#include "components/adblock/adblock_telemetry_service.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "components/adblock/adblock_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace adblock;

namespace {
constexpr char kTestPingServer[] = "http://faketelemetryping";
}

class AdblockTelemetryServiceTest : public testing::Test {
 public:
  AdblockTelemetryServiceTest() {}

 protected:
  // testing::Test
  void SetUp() override {
    test_pref_service_ =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    AdblockTelemetryService::RegisterProfilePrefs(
        test_pref_service_->registry());
    test_pref_service_->registry()->RegisterBooleanPref(
        adblock::prefs::kEnableAdblock, true);
    test_pref_service_->registry()->RegisterBooleanPref(
        adblock::prefs::kEnableAcceptableAds, true);
  }

  PrefService* prefs() const { return test_pref_service_.get(); }

  network::TestURLLoaderFactory* net_factory() {
    return const_cast<network::TestURLLoaderFactory*>(
        &test_url_loader_factory_);
  }

  content::BrowserTaskEnvironment* task_environment() const {
    return const_cast<content::BrowserTaskEnvironment*>(&task_environment_);
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      test_pref_service_;
  network::TestURLLoaderFactory test_url_loader_factory_;

  AdblockTelemetryServiceTest& operator=(const AdblockTelemetryServiceTest&) =
      delete;
  AdblockTelemetryServiceTest(const AdblockTelemetryServiceTest&) = delete;
};

TEST_F(AdblockTelemetryServiceTest, RequestDataValidity) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"token\"}",
                             net::HTTP_OK);
  net_factory()->SetInterceptor(
      base::BindLambdaForTesting([](const network::ResourceRequest& request) {
        EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, request.method);
        auto body = request.request_body;
        ASSERT_NE(nullptr, body);
        auto* elements = body->elements();
        ASSERT_NE(nullptr, elements);
        EXPECT_FALSE(elements->empty());
        std::string data;
        for (auto& elem : *elements) {
          auto piece = elem.As<network::DataElementBytes>().AsStringPiece();
          data.append(piece.data(), piece.size());
        }
        auto parsed = base::JSONReader::Read(data);
        EXPECT_TRUE(parsed.has_value());
        base::DictionaryValue* main_dict{nullptr};
        ASSERT_TRUE(parsed->GetAsDictionary(&main_dict));
        std::string evt;
        EXPECT_TRUE(main_dict->GetString("event", &evt));
        EXPECT_EQ("PingV1", evt);
        base::Value* val{nullptr};
        EXPECT_TRUE(main_dict->Get("event", &val));
        base::Value* sub_value{nullptr};
        ASSERT_TRUE(main_dict->Get("data", &sub_value));
        base::DictionaryValue* sub_dict{nullptr};
        ASSERT_TRUE(sub_value->GetAsDictionary(&sub_dict));
        ASSERT_TRUE(sub_dict->Get("rti", &sub_value));
        EXPECT_TRUE(sub_value->is_string());
        ASSERT_TRUE(sub_dict->Get("first_ping_token", &sub_value));
        EXPECT_TRUE(sub_value->is_string() || sub_value->is_none());
        ASSERT_TRUE(sub_dict->Get("last_ping_token", &sub_value));
        EXPECT_TRUE(sub_value->is_string() || sub_value->is_none());
        ASSERT_TRUE(sub_dict->Get("acceptable_ads", &sub_value));
        EXPECT_TRUE(sub_value->is_bool());
        ASSERT_TRUE(sub_dict->Get("addon", &sub_value));
        EXPECT_TRUE(sub_value->is_string() || sub_value->is_none());
        ASSERT_TRUE(sub_dict->Get("addon_version", &sub_value));
        EXPECT_TRUE(sub_value->is_string() || sub_value->is_none());
        ASSERT_TRUE(sub_dict->Get("application", &sub_value));
        ASSERT_TRUE(sub_value->is_string());
        EXPECT_FALSE(sub_value->GetString().empty());
        ASSERT_TRUE(sub_dict->Get("application_version", &sub_value));
        ASSERT_TRUE(sub_value->is_string());
        EXPECT_FALSE(sub_value->GetString().empty());
        ASSERT_TRUE(sub_dict->Get("platform", &sub_value));
        ASSERT_TRUE(sub_value->is_string());
        EXPECT_FALSE(sub_value->GetString().empty());
        ASSERT_TRUE(sub_dict->Get("platform_version", &sub_value));
        ASSERT_TRUE(sub_value->is_string());
        EXPECT_FALSE(sub_value->GetString().empty());
      }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
}

TEST_F(AdblockTelemetryServiceTest, ResponseTokens) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"test1_token\"}",
                             net::HTTP_OK);
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      "test1_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryFirstPingResponseToken));
  EXPECT_EQ(
      "test1_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryLastPingResponseToken));

  net_factory()->ClearResponses();
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"test2_token\"}",
                             net::HTTP_OK);
  task_environment()->FastForwardBy(AdblockTelemetryService::kPingInterval);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      "test1_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryFirstPingResponseToken));
  EXPECT_EQ(
      "test2_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryLastPingResponseToken));

  net_factory()->ClearResponses();
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"test3_token\"}",
                             net::HTTP_OK);
  task_environment()->FastForwardBy(AdblockTelemetryService::kPingInterval);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      "test1_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryFirstPingResponseToken));
  EXPECT_EQ(
      "test3_token",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryLastPingResponseToken));
}

TEST_F(AdblockTelemetryServiceTest, UUIDLifetime) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"token\"}",
                             net::HTTP_OK);
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  auto uuid =
      prefs()->GetString(AdblockTelemetryService::kAdblockTelemetryUUID);
  EXPECT_FALSE(uuid.empty());

  task_environment()->FastForwardBy(AdblockTelemetryService::kUUIDLifetime +
                                    base::TimeDelta::FromMilliseconds(1));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(prefs()
                   ->GetString(AdblockTelemetryService::kAdblockTelemetryUUID)
                   .empty());
  EXPECT_NE(uuid,
            prefs()->GetString(AdblockTelemetryService::kAdblockTelemetryUUID));
}

TEST_F(AdblockTelemetryServiceTest, Scheduling) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"token\"}",
                             net::HTTP_OK);
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  auto last_sent_ts = prefs()->GetInt64(
      AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp);
  EXPECT_NE(0u, last_sent_ts);
  task_environment()->FastForwardBy(AdblockTelemetryService::kPingInterval / 2);
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(last_sent_ts,
            prefs()->GetInt64(
                AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp));
  task_environment()->FastForwardBy(AdblockTelemetryService::kPingInterval / 2);
  base::RunLoop().RunUntilIdle();
  EXPECT_NE(last_sent_ts,
            prefs()->GetInt64(
                AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp));
}

TEST_F(AdblockTelemetryServiceTest, Retries) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, {},
                             net::HTTP_INTERNAL_SERVER_ERROR);
  int hit_count = 0;
  net_factory()->SetInterceptor(base::BindLambdaForTesting(
      [&hit_count](const network::ResourceRequest& request) { ++hit_count; }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  for (int i = 0; i < AdblockTelemetryService::kMaxRetries; ++i) {
    task_environment()->FastForwardBy(AdblockTelemetryService::kMaximumBackoff);
    base::RunLoop().RunUntilIdle();
  }
  EXPECT_EQ(AdblockTelemetryService::kMaxRetries, hit_count);
}

TEST_F(AdblockTelemetryServiceTest, DoesNotRetryOn4xx) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, {}, net::HTTP_NOT_FOUND);
  int hit_count = 0;
  net_factory()->SetInterceptor(base::BindLambdaForTesting(
      [&hit_count](const network::ResourceRequest& request) { ++hit_count; }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  for (int i = 0; i < AdblockTelemetryService::kMaxRetries; ++i) {
    task_environment()->FastForwardBy(AdblockTelemetryService::kMaximumBackoff);
    base::RunLoop().RunUntilIdle();
  }
  EXPECT_EQ(1, hit_count);
}

TEST_F(AdblockTelemetryServiceTest, 2xxIsSuccess) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"pass\"}",
                             net::HTTP_ACCEPTED);
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      "pass",
      prefs()->GetString(
          AdblockTelemetryService::kAdblockTelemetryLastPingResponseToken));
}

TEST_F(AdblockTelemetryServiceTest, OnlyWhenEnabled) {
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  prefs()->SetBoolean(adblock::prefs::kEnableAdblock, false);
  int hit_count = 0;
  net_factory()->SetInterceptor(base::BindLambdaForTesting(
      [&hit_count](const network::ResourceRequest& request) { ++hit_count; }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, hit_count);
}

TEST_F(AdblockTelemetryServiceTest, SentInTheFuture) {
  auto future = base::Time::Now() + base::TimeDelta::FromHours(1);
  prefs()->SetInt64(AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp,
                    future.ToInternalValue());
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"token\"}",
                             net::HTTP_OK);
  int hit_count = 0;
  net_factory()->SetInterceptor(base::BindLambdaForTesting(
      [&hit_count](const network::ResourceRequest& request) { ++hit_count; }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
  auto last_sent_ts = prefs()->GetInt64(
      AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp);
  EXPECT_NE(future.ToInternalValue(), last_sent_ts);
  EXPECT_EQ(1, hit_count);
}

TEST_F(AdblockTelemetryServiceTest, AcceptableAdsValue) {
  prefs()->SetBoolean(adblock::prefs::kEnableAcceptableAds, false);
  AdblockTelemetryService service(
      prefs(), base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                   net_factory()));
  service.OverrideValuesForTesting(kTestPingServer);
  net_factory()->AddResponse(kTestPingServer, "{\"token\": \"token\"}",
                             net::HTTP_OK);
  net_factory()->SetInterceptor(
      base::BindLambdaForTesting([](const network::ResourceRequest& request) {
        EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, request.method);
        auto body = request.request_body;
        ASSERT_NE(nullptr, body);
        auto* elements = body->elements();
        ASSERT_NE(nullptr, elements);
        EXPECT_FALSE(elements->empty());
        std::string data;
        for (auto& elem : *elements) {
          auto piece = elem.As<network::DataElementBytes>().AsStringPiece();
          data.append(piece.data(), piece.size());
        }
        auto parsed = base::JSONReader::Read(data);
        EXPECT_TRUE(parsed.has_value());
        base::DictionaryValue* main_dict{nullptr};
        ASSERT_TRUE(parsed->GetAsDictionary(&main_dict));
        base::Value* sub_value{nullptr};
        ASSERT_TRUE(main_dict->Get("data", &sub_value));
        base::DictionaryValue* sub_dict{nullptr};
        ASSERT_TRUE(sub_value->GetAsDictionary(&sub_dict));
        ASSERT_TRUE(sub_dict->Get("acceptable_ads", &sub_value));
        ASSERT_TRUE(sub_value->is_bool());
        bool acceptable_ads_on = true;
        ASSERT_TRUE(sub_value->GetAsBoolean(&acceptable_ads_on));
        EXPECT_EQ(false, acceptable_ads_on);
      }));
  service.Start();
  task_environment()->FastForwardBy(AdblockTelemetryService::kInitialDelay);
  base::RunLoop().RunUntilIdle();
}

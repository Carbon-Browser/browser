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

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include "base/task/post_task.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_utils.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {

namespace {

const char kDataType[] = "application/json";

const int kErrorsToIgnore{0};
const double kMultiplyFactor{2.};
const double kJitterFactor{.5};
const int64_t kEntryLifetimeMs{-1};  // Unlimited
const bool kAlwaysUseInitialDelay{true};

}  // namespace

const int AdblockTelemetryService::kMaxRetries{5};
constexpr base::TimeDelta AdblockTelemetryService::kInitialDelay;
constexpr base::TimeDelta AdblockTelemetryService::kPingInterval;
constexpr base::TimeDelta AdblockTelemetryService::kUUIDLifetime;
constexpr base::TimeDelta AdblockTelemetryService::kMaximumBackoff;

const char AdblockTelemetryService::kAdblockTelemetryFirstPingResponseToken[] =
    "adblock.telemetry.first_token";
const char AdblockTelemetryService::kAdblockTelemetryLastPingResponseToken[] =
    "adblock.telemetry.last_token";
const char AdblockTelemetryService::kAdblockTelemetryLastSentTimestamp[] =
    "adblock.telemetry.last_sent_ts";
const char AdblockTelemetryService::kAdblockTelemetryUUID[] =
    "adblock.telemetry.uid";
const char AdblockTelemetryService::kAdblockTelemetryUUIDGenerationTimestamp[] =
    "adblock.telemetry.uid_ts";

AdblockTelemetryService::AdblockTelemetryService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      url_loader_factory_(url_loader_factory),
      backoff_policy_{
          kErrorsToIgnore,
          // this is incorrect initiall dealy in milliseconds
          // might overflow the 32 bit mark
          // TODO: pull request to chromium change policiy initial_delay_ms to
          // int64_t
          static_cast<int>(
              AdblockTelemetryService::kInitialDelay.InMilliseconds()),
          kMultiplyFactor, kJitterFactor, kMaximumBackoff.InMicroseconds(),
          kEntryLifetimeMs, kAlwaysUseInitialDelay},
      back_off_(&backoff_policy_),
      weak_ptr_factory_(this) {}

AdblockTelemetryService::~AdblockTelemetryService() = default;

// static
void AdblockTelemetryService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterInt64Pref(kAdblockTelemetryLastSentTimestamp, 0);
  registry->RegisterInt64Pref(kAdblockTelemetryUUIDGenerationTimestamp, 0);
  registry->RegisterStringPref(kAdblockTelemetryUUID, {});
  registry->RegisterStringPref(kAdblockTelemetryFirstPingResponseToken, {});
  registry->RegisterStringPref(kAdblockTelemetryLastPingResponseToken, {});
}

void AdblockTelemetryService::Start() {
  DLOG(INFO) << "[ABP] Starting telemetry service.";

  auto last_send_ts = prefs_->GetInt64(kAdblockTelemetryLastSentTimestamp);
  auto last_sent_time = base::Time::FromInternalValue(last_send_ts);
  auto now = base::Time::Now();
  base::TimeDelta send_diff;
  // We couldn't send it in the future, could we? Force sending now.
  if (last_sent_time > now) {
    last_send_ts = 0;
    prefs_->ClearPref(kAdblockTelemetryLastSentTimestamp);
  } else {
    send_diff = base::Time::Now() - last_sent_time;
  }
  auto send_interval = kPingInterval;
  auto callback = base::BindOnce(&AdblockTelemetryService::OnTimerFired,
                                 base::Unretained(this));

  // Sent longer than |kPingInterval| ago (or never sent) - send it right after
  // the initial delay.
  if (!last_send_ts || send_diff >= send_interval) {
    if (!last_send_ts) {
      DLOG(INFO) << "Never sent before so sending straight away after "
                 << "initial delay";
    } else {
      DLOG(INFO) << "Sent long time ago (" << send_diff.InMillisecondsF()
                 << " ms) so sending straight away after initial delay";
    }
    timer_.Start(FROM_HERE, kInitialDelay, std::move(callback));
  } else {
    DLOG(INFO) << "Sent not so long time ago so scheduled after "
               << (send_interval - send_diff).InMillisecondsF() << " ms";
    // Sent shorter than |kPingInterval| ago - supplement to |kPingInterval|.
    timer_.Start(FROM_HERE, send_interval - send_diff, std::move(callback));
  }
}

std::string AdblockTelemetryService::ValidateAndGetGUID() const {
  // Make sure valid GUID is there.
  auto uuid = prefs_->GetString(kAdblockTelemetryUUID);
  auto uuid_gen_ts = prefs_->GetInt64(kAdblockTelemetryUUIDGenerationTimestamp);
  auto now = base::Time::Now();
  auto gen_diff = now - base::Time::FromInternalValue(uuid_gen_ts);
  // No GUID or GUID is older than |kUUIDLifetime|. Regenerate.
  if (uuid.empty() || gen_diff >= kUUIDLifetime) {
    uuid = base::GenerateGUID();
    prefs_->SetString(kAdblockTelemetryUUID, uuid);
    prefs_->SetInt64(kAdblockTelemetryUUIDGenerationTimestamp,
                     now.ToInternalValue());
  }

  return uuid;
}

std::string AdblockTelemetryService::GetPingJSONData() const {
  base::DictionaryValue object;
  object.SetStringKey("event", "PingV1");

  base::DictionaryValue data;
  data.SetStringKey("rti", ValidateAndGetGUID());
  auto first_token = prefs_->GetString(kAdblockTelemetryFirstPingResponseToken);
  if (!first_token.empty()) {
    data.SetStringKey("first_ping_token", first_token);
  } else {
    data.SetKey("first_ping_token", base::Value());
  }
  auto last_token = prefs_->GetString(kAdblockTelemetryLastPingResponseToken);
  if (!last_token.empty()) {
    data.SetStringKey("last_ping_token", last_token);
  } else {
    data.SetKey("last_ping_token", base::Value());
  }

  data.SetBoolKey("acceptable_ads",
                  prefs_->GetBoolean(prefs::kEnableAcceptableAds));
  auto internal_info = utils::GetAppInfo();
  data.SetStringKey("application", internal_info.name);
  data.SetStringKey("application_version", internal_info.version);
  data.SetStringKey("platform", base::SysInfo::OperatingSystemName());
  data.SetStringKey("platform_version",
                    base::SysInfo::OperatingSystemVersion());
  data.SetKey("addon", base::Value());
  data.SetKey("addon_version", base::Value());

  object.SetKey("data", std::move(data));
  std::string serialized;
  JSONStringValueSerializer serializer(&serialized);
  serializer.Serialize(object);

  DLOG(INFO) << "[ABP] Sending ping with: " << serialized;
  return serialized;
}

// Time to send some ping, shall we?
void AdblockTelemetryService::OnTimerFired() {
  DLOG(INFO) << "[ABP] service trigger";
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(GeneratePingURL());
  request->method = net::HttpRequestHeaders::kPostMethod;
  request->enable_upload_progress = true;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("adblock_telemetry_ping", R"(
        semantics {
          sender: "AdblockTelemetryService"
          description:
            "ABP telemetry ping"
          trigger:
            "Interval"
          data:
            "Ping data. No user data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You enable or disable this feature via 'Adblock Enable' pref."
          policy_exception_justification: "Not implemented."
          }
        })");

  DCHECK(!url_loader_);
  url_loader_ =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);

  url_loader_->AttachStringForUpload(GetPingJSONData(), kDataType);
  url_loader_->SetTimeoutDuration(base::TimeDelta::FromMinutes(3));
  url_loader_->SetAllowHttpErrorResults(true);
  // Retries are handled manually.
  url_loader_->SetRetryOptions(0, network::SimpleURLLoader::RETRY_NEVER);
  if (prefs_->GetBoolean(prefs::kEnableAdblock)) {
    DLOG(INFO) << "[ABP] Sending ping to: " << GeneratePingURL();
    url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&AdblockTelemetryService::OnURLCompleted,
                       base::Unretained(this)),
        network::SimpleURLLoader::kMaxBoundedStringDownloadSize - 1);
  }
}

void AdblockTelemetryService::OnURLCompleted(
    std::unique_ptr<std::string> response) {
  DLOG(INFO) << "[ABP] Telemetry service got response to ping";
  // Sent something. Record that fact.
  prefs_->SetInt64(kAdblockTelemetryLastSentTimestamp,
                   base::Time::Now().ToInternalValue());

  auto status = url_loader_->NetError();
  DCHECK(url_loader_->ResponseInfo() || status != net::OK);
  // If there is a response but it has no headers still assume success.
  auto response_code =
      (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
          ? url_loader_->ResponseInfo()->headers->response_code()
          : net::HTTP_OK;

  base::DeleteSoon(FROM_HERE, {content::BrowserThread::UI},
                   url_loader_.release());
  const bool is_error = (status != net::OK) || (response_code / 100 != 2);
  bool max_failure_count_reached = false;
  if (is_error) {
    const bool worth_retry = status != net::OK || (response_code / 100 == 5);

    // Error: do some retries using exponential backoff timer.
    LOG(ERROR) << "[ABP] ping response error";
    back_off_.InformOfRequest(false);
    max_failure_count_reached =
        back_off_.failure_count() >= kMaxRetries || !worth_retry;
    if (!max_failure_count_reached) {
      LOG(ERROR) << "[ABP] ping response error. Will retry in "
                 << back_off_.GetTimeUntilRelease().InMillisecondsF() << " ms";
      timer_.Start(FROM_HERE, back_off_.GetTimeUntilRelease(),
                   base::BindOnce(&AdblockTelemetryService::OnTimerFired,
                                  base::Unretained(this)));
      return;
    }
  }
  // Either success or retries limit has been reached. Schedule next check
  // in any case.
  back_off_.Reset();
  timer_.Start(FROM_HERE, kPingInterval,
               base::BindOnce(&AdblockTelemetryService::OnTimerFired,
                              base::Unretained(this)));
  if (max_failure_count_reached) {
    LOG(ERROR) << "[ABP] ping response error. No retries, though "
               << "(does not make sense or max retries limit reached).";
    return;
  }

  DLOG(INFO) << "[ABP] ping response: " << (response ? *response : "");

  if (response) {
    auto parsed = base::JSONReader::Read(*response);
    std::string token;
    if (parsed.has_value() && parsed->is_dict()) {
      auto* token_value = parsed->FindStringKey("token");
      if (token_value)
        token = *token_value;
    }

    if (prefs_->GetString(kAdblockTelemetryFirstPingResponseToken).empty()) {
      prefs_->SetString(kAdblockTelemetryFirstPingResponseToken, token);
    }

    prefs_->SetString(kAdblockTelemetryLastPingResponseToken, token);
  }
}

std::string AdblockTelemetryService::GeneratePingURL() {
  if (ping_url_.empty()) {
    // Requirements from OPS so not a single server is hammered.
    auto random = base::RandInt(0, 9);
    ping_url_ = base::ReplaceStringPlaceholders(
        kPingURLTemplate, {ABP_TELEMETRY_CLIENT_ID, std::to_string(random)},
        nullptr);
  }

  return ping_url_;
}

void AdblockTelemetryService::Shutdown() {}

}  // namespace adblock

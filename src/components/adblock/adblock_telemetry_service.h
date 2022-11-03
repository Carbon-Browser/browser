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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_TELEMETRY_SERVICE_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_TELEMETRY_SERVICE_H_

#include <memory>
#include <string>

#include "AdblockPlus.h"

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/base/backoff_entry.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace adblock {
/**
 * @brief Sends periodic pings to eyeo in order to count active users. Executed
 * from Browser process UI main thread.
 */
class AdblockTelemetryService : public KeyedService {
 public:
  static constexpr base::TimeDelta kInitialDelay =
      base::TimeDelta::FromSeconds(30);
  static constexpr base::TimeDelta kPingInterval =
      base::TimeDelta::FromHours(8);
  static constexpr base::TimeDelta kUUIDLifetime =
      base::TimeDelta::FromHours(24);
  static constexpr base::TimeDelta kMaximumBackoff =
      base::TimeDelta::FromHours(1);
  static const int kMaxRetries;

  static const char kAdblockTelemetryFirstPingResponseToken[];
  static const char kAdblockTelemetryLastPingResponseToken[];
  static const char kAdblockTelemetryLastSentTimestamp[];
  static const char kAdblockTelemetryUUID[];
  static const char kAdblockTelemetryUUIDGenerationTimestamp[];

  AdblockTelemetryService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AdblockTelemetryService() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  AdblockTelemetryService& operator=(const AdblockTelemetryService& other) =
      delete;
  AdblockTelemetryService(const AdblockTelemetryService& other) = delete;

  void Start();

  // KeyedService:
  void Shutdown() override;

  void OverrideValuesForTesting(std::string ping_url_override) {
    ping_url_ = std::move(ping_url_override);
  }

 private:
  void OnTimerFired();
  void OnURLCompleted(std::unique_ptr<std::string> response);
  std::string ValidateAndGetGUID() const;
  std::string GetPingJSONData() const;
  std::string GeneratePingURL();

  const std::string kPingURLTemplate {
#if defined(NDEBUG)
    "https://$1-$2.telemetry.eyeo.com/"
#else
    "https://$1-$2.staging.telemetry.eyeo.com/"
#endif
  };

  PrefService* prefs_{nullptr};
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  base::OneShotTimer timer_;
  net::BackoffEntry::Policy backoff_policy_;
  net::BackoffEntry back_off_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  std::string ping_url_;
  base::WeakPtrFactory<AdblockTelemetryService> weak_ptr_factory_;
};

}  // namespace adblock
#endif  // COMPONENTS_ADBLOCK_ADBLOCK_TELEMETRY_SERVICE_H_

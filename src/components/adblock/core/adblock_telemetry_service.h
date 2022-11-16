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

#ifndef COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
}  // namespace network

class PrefService;

namespace adblock {
/**
 * @brief Sends periodic pings to eyeo in order to count active users. Executed
 * from Browser process UI main thread.
 */
class AdblockTelemetryService : public KeyedService {
 public:
  // Provides data and behavior relevant for a Telemetry "topic". A topic could
  // be "counting users" or "reporting filter list hits" for example.
  class TopicProvider {
   public:
    virtual ~TopicProvider() = default;
    // Endpoint URL on the Telemetry server onto which requests should be sent.
    virtual GURL GetEndpointURL() const = 0;
    // Authorization bearer token for the endpoint defined by GetEndpointURL().
    virtual std::string GetAuthToken() const = 0;
    // Data uploaded with the request, should be valid for the schema
    // present on the server.
    virtual std::string GetPayload() const = 0;
    // Returns the desired delay until AdblockTelemetryService makes the next
    // network request.
    virtual base::TimeDelta GetTimeToNextRequest() const = 0;
    // Parses the response returned by the Telemetry server. |response_content|
    // may be null. Implementation is free to implement a "retry" in case of
    // response errors via GetTimeToNextRequest().
    virtual void ParseResponse(
        std::unique_ptr<std::string> response_content) = 0;
  };
  AdblockTelemetryService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AdblockTelemetryService() override;

  // Add all required topic providers before calling Start().
  void AddTopicProvider(std::unique_ptr<TopicProvider> topic_provider);

  // Starts periodic Telemetry requests, provided ad-blocking is enabled.
  // If prefs::kAdblockEnabled is false, the schedule will instead start when
  // the pref becomes true.
  void Start();

  // KeyedService:
  void Shutdown() override;

 private:
  void OnEnableAdblockChanged();

  SEQUENCE_CHECKER(sequence_checker_);
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  class Conversation;
  std::vector<std::unique_ptr<Conversation>> ongoing_conversations_;
  BooleanPrefMember enable_adblock_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_ADBLOCK_TELEMETRY_SERVICE_H_

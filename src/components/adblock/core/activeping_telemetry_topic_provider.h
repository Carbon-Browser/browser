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

#ifndef COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_
#define COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_

#include "base/time/time.h"
#include "components/adblock/core/adblock_telemetry_service.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/prefs/pref_service.h"

namespace adblock {

// Telemetry topic provider that uploads user-counting data for periodic pings.
// Provides the following data in Payload:
// - Last ping time, previous-to-last ping time, first ping time
// - Unique, non-persistent tag for disambiguating pings made by clients in
//   the same day
// - Whether Acceptable Ads is enabled
// - Application name & version, platform name & version
// Note: Provides no user-identifiable information, no persistent tracking
// data (ie. no traceable UUID) and no information about user actions.
class ActivepingTelemetryTopicProvider final
    : public AdblockTelemetryService::TopicProvider {
 public:
  ActivepingTelemetryTopicProvider(utils::AppInfo app_info,
                                   PrefService* pref_service,
                                   const GURL& base_url,
                                   const std::string& auth_token);
  ~ActivepingTelemetryTopicProvider() final;

  static GURL DefaultBaseUrl();
  static std::string DefaultAuthToken();

  GURL GetEndpointURL() const final;
  std::string GetAuthToken() const final;
  std::string GetPayload() const final;

  // Normally 8 hours since last ping, 1 hour in case of retries.
  base::TimeDelta GetTimeToNextRequest() const final;

  // Attempts to parse "token" (an opaque server description of last ping time)
  // from |response_content|.
  void ParseResponse(std::unique_ptr<std::string> response_content) final;

 private:
  void ScheduleNextPing(base::TimeDelta delay);
  void UpdatePrefs(const std::string& ping_response_time);

  const utils::AppInfo app_info_;
  PrefService* pref_service_;
  const GURL base_url_;
  const std::string auth_token_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_ACTIVEPING_TELEMETRY_TOPIC_PROVIDER_H_

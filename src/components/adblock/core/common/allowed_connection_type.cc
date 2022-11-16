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

#include "components/adblock/core/common/allowed_connection_type.h"

#include "base/notreached.h"

namespace adblock {

absl::optional<AllowedConnectionType> AllowedConnectionTypeFromString(
    base::StringPiece str) {
  if (str == "wifi")
    return AllowedConnectionType::kWiFi;
  if (str == "any")
    return AllowedConnectionType::kAny;
  return absl::nullopt;
}

const char* AllowedConnectionTypeToString(AllowedConnectionType value) {
  switch (value) {
    case AllowedConnectionType::kWiFi:
      return "wifi";
    case AllowedConnectionType::kAny:
      return "any";
    default:
      NOTREACHED();
      return "";
  }
}

}  // namespace adblock

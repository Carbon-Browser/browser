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

#ifndef COMPONENTS_ADBLOCK_ALLOWED_CONNECTION_TYPE_H_
#define COMPONENTS_ADBLOCK_ALLOWED_CONNECTION_TYPE_H_

#include <string>

#include "absl/types/optional.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_piece_forward.h"

namespace adblock {

// Specifies the connection type for which subscription updates are allowed.
// Subscription files can be quite big so it makes sense to limit them on
// metered data connection.
enum class AllowedConnectionType {
  // Allow updates only when on WiFi:
  kWiFi,
  // Allow updates on any connection:
  kAny,
  // Don't allow updates at all:
  kNone,
};

absl::optional<AllowedConnectionType> AllowedConnectionTypeFromString(
    base::StringPiece str);
const char* AllowedConnectionTypeToString(AllowedConnectionType value);

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ALLOWED_CONNECTION_TYPE_H_

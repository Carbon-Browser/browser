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

#include "components/adblock/adblock_logging_impl.h"

#include "base/logging.h"

namespace adblock {

void AdblockLoggingImpl::operator()(AdblockPlus::LogSystem::LogLevel level,
                                    const std::string& message,
                                    const std::string& source) {
  switch (level) {
    default:
      LOG(INFO) << "[ABP] " << source << ":" << message;
      break;
    case LogSystem::LOG_LEVEL_ERROR:
      LOG(ERROR) << "[ABP] " << source << ":" << message;
      break;
    case LogSystem::LOG_LEVEL_WARN:
      LOG(WARNING) << "[ABP] " << source << ":" << message;
      break;
  }
}

}  // namespace adblock

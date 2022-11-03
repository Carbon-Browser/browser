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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_LOGGING_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_LOGGING_IMPL_H_

#include "third_party/libadblockplus/src/include/AdblockPlus/LogSystem.h"

#include <string>

namespace adblock {
/**
 * @brief Logs abp message attaching a [ABP] tag to it.
 *
 */
class AdblockLoggingImpl final : public AdblockPlus::LogSystem {
 public:
  AdblockLoggingImpl() = default;
  ~AdblockLoggingImpl() final = default;
  AdblockLoggingImpl& operator=(const AdblockLoggingImpl& other) = delete;
  AdblockLoggingImpl(const AdblockLoggingImpl& other) = delete;
  AdblockLoggingImpl& operator=(AdblockLoggingImpl&& other) = default;
  AdblockLoggingImpl(AdblockLoggingImpl&& other) = default;

  // AdblockPlus::LogSystem overrides:
  void operator()(AdblockPlus::LogSystem::LogLevel level,
                  const std::string& message,
                  const std::string& source) final;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_LOGGING_IMPL_H_

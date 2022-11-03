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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_TIMER_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_TIMER_IMPL_H_

#include "third_party/libadblockplus/src/include/AdblockPlus/ITimer.h"

#include <chrono>

#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"

namespace adblock {
/**
 * @brief Implements running delayed tasks for libabp via base::OneShotTimer.
 * Lives in abp dedicated thread, in the browser process.
 */
class AdblockTimerImpl final : public AdblockPlus::ITimer {
 public:
  AdblockTimerImpl(scoped_refptr<base::SingleThreadTaskRunner> abp_runner);
  ~AdblockTimerImpl() final;
  AdblockTimerImpl& operator=(const AdblockTimerImpl& other) = delete;
  AdblockTimerImpl(const AdblockTimerImpl& other) = delete;
  AdblockTimerImpl& operator=(AdblockTimerImpl&& other) = delete;
  AdblockTimerImpl(AdblockTimerImpl&& other) = delete;

  // AdblockPlus::ITimer overrides
  void SetTimer(const std::chrono::milliseconds& timeout,
                const AdblockPlus::ITimer::TimerCallback& callback) final;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> abp_runner_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_TIMER_IMPL_H_

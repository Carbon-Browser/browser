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

#include "components/adblock/adblock_timer_impl.h"

#include <memory>

#include "base/time/time.h"
#include "base/timer/timer.h"

namespace adblock {

namespace {

void OnTimeout(std::unique_ptr<base::OneShotTimer> timer,
               const AdblockPlus::ITimer::TimerCallback& callback) {
  callback();
}

void StartTimer(const base::TimeDelta& timeout,
                const AdblockPlus::ITimer::TimerCallback& callback,
                scoped_refptr<base::SingleThreadTaskRunner> callback_runner) {
  DCHECK(callback_runner->BelongsToCurrentThread());
  auto timer = std::make_unique<base::OneShotTimer>();
  timer->SetTaskRunner(callback_runner);
  timer->Start(FROM_HERE, timeout,
               base::BindOnce(&OnTimeout, std::move(timer), callback));
}

}  // namespace

AdblockTimerImpl::AdblockTimerImpl(
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner)
    : abp_runner_(abp_runner) {
  DCHECK(abp_runner_->BelongsToCurrentThread());
}

AdblockTimerImpl::~AdblockTimerImpl() {
  DCHECK(abp_runner_->BelongsToCurrentThread());
}

void AdblockTimerImpl::SetTimer(
    const std::chrono::milliseconds& timeout,
    const AdblockPlus::ITimer::TimerCallback& callback) {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
      << "the same thread. Otherwise thread safety can not be assured.";

  StartTimer(base::TimeDelta::FromMilliseconds(timeout.count()), callback,
             abp_runner_);
}

}  // namespace adblock

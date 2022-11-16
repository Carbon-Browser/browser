// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/profiler/stack_sampler.h"

#include <pthread.h>

#include <memory>

#include "base/threading/platform_thread.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_CHROMEOS) && defined(ARCH_CPU_X86_64)
#include "base/bind.h"
#include "base/check.h"
#include "base/profiler/frame_pointer_unwinder.h"
#include "base/profiler/stack_copier_signal.h"
#include "base/profiler/stack_sampler_impl.h"
#include "base/profiler/thread_delegate_posix.h"
#include "base/profiler/unwinder.h"
#endif

namespace base {

namespace {

#if BUILDFLAG(IS_CHROMEOS) && defined(ARCH_CPU_X86_64)
std::vector<std::unique_ptr<Unwinder>> CreateUnwinders() {
  std::vector<std::unique_ptr<Unwinder>> unwinders;
  unwinders.push_back(std::make_unique<FramePointerUnwinder>());
  return unwinders;
}
#endif

}  // namespace

std::unique_ptr<StackSampler> StackSampler::Create(
    SamplingProfilerThreadToken thread_token,
    ModuleCache* module_cache,
    UnwindersFactory core_unwinders_factory,
    RepeatingClosure record_sample_callback,
    StackSamplerTestDelegate* test_delegate) {
#if BUILDFLAG(IS_CHROMEOS) && defined(ARCH_CPU_X86_64)
  DCHECK(!core_unwinders_factory);
  return std::make_unique<StackSamplerImpl>(
      std::make_unique<StackCopierSignal>(
          ThreadDelegatePosix::Create(thread_token)),
      BindOnce(&CreateUnwinders), module_cache,
      std::move(record_sample_callback), test_delegate);
#else
  return nullptr;
#endif
}

size_t StackSampler::GetStackBufferSize() {
  size_t stack_size = PlatformThread::GetDefaultThreadStackSize();

  pthread_attr_t attr;
  if (stack_size == 0 && pthread_attr_init(&attr) == 0) {
    if (pthread_attr_getstacksize(&attr, &stack_size) != 0)
      stack_size = 0;
    pthread_attr_destroy(&attr);
  }

  // Maximum limits under NPTL implementation.
  constexpr size_t kDefaultStackLimit = 4 * (1 << 20);
  return stack_size > 0 ? stack_size : kDefaultStackLimit;
}

}  // namespace base

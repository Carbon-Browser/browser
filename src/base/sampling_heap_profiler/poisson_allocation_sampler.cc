// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sampling_heap_profiler/poisson_allocation_sampler.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <utility>

#include "base/allocator/buildflags.h"
#include "base/allocator/dispatcher/dispatcher.h"
#include "base/allocator/dispatcher/reentry_guard.h"
#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/no_destructor.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "build/build_config.h"

namespace base {

namespace {

using ::base::allocator::dispatcher::ReentryGuard;

const size_t kDefaultSamplingIntervalBytes = 128 * 1024;

// Notes on TLS usage:
//
// * There's no safe way to use TLS in malloc() as both C++ thread_local and
//   pthread do not pose any guarantees on whether they allocate or not.
// * We think that we can safely use thread_local w/o re-entrancy guard because
//   the compiler will use "tls static access model" for static builds of
//   Chrome [https://www.uclibc.org/docs/tls.pdf].
//   But there's no guarantee that this will stay true, and in practice
//   it seems to have problems on macOS/Android. These platforms do allocate
//   on the very first access to a thread_local on each thread.
// * Directly using/warming-up platform TLS seems to work on all platforms,
//   but is also not guaranteed to stay true. We make use of it for reentrancy
//   guards on macOS/Android.
// * We cannot use Windows Tls[GS]etValue API as it modifies the result of
//   GetLastError.
//
// Android thread_local seems to be using __emutls_get_address from libgcc:
// https://github.com/gcc-mirror/gcc/blob/master/libgcc/emutls.c
// macOS version is based on _tlv_get_addr from dyld:
// https://opensource.apple.com/source/dyld/dyld-635.2/src/threadLocalHelpers.s.auto.html

// The guard protects against reentering on platforms other the macOS and
// Android.
thread_local bool g_tls_internal_reentry_guard = false;

// Accumulated bytes towards sample thread local key.
thread_local intptr_t g_tls_accumulated_bytes = 0;

// Used as a workaround to avoid bias from muted samples. See
// ScopedMuteThreadSamples for more details.
thread_local intptr_t g_tls_accumulated_bytes_snapshot = 0;
const intptr_t kAccumulatedBytesOffset = 1 << 29;

// A boolean used to distinguish first allocation on a thread:
//   false - first allocation on the thread;
//   true  - otherwise.
// Since g_tls_accumulated_bytes is initialized with zero the very first
// allocation on a thread would always trigger the sample, thus skewing the
// profile towards such allocations. To mitigate that we use the flag to
// ensure the first allocation is properly accounted.
thread_local bool g_tls_sampling_interval_initialized = false;

// Controls if sample intervals should not be randomized. Used for testing.
bool g_deterministic = false;

// Controls if hooked samples should be ignored. Used for testing.
std::atomic_bool g_mute_hooked_samples{false};

// A positive value if profiling is running, otherwise it's zero.
std::atomic_bool g_running{false};

// Pointer to the current |LockFreeAddressHashSet|.
std::atomic<LockFreeAddressHashSet*> g_sampled_addresses_set{nullptr};

// Sampling interval parameter, the mean value for intervals between samples.
std::atomic_size_t g_sampling_interval{kDefaultSamplingIntervalBytes};

void (*g_hooks_install_callback)() = nullptr;

// This will be true if *either* InstallAllocatorHooksOnce or
// SetHooksInstallerCallback has run. `g_hooks_install_callback` should be
// invoked when *both* have run, so each of them checks the value and, if it is
// true, knows that the other function has already run so it's time to invoke
// the callback.
std::atomic_bool g_hooks_installed{false};
}  // namespace

PoissonAllocationSampler::ScopedMuteThreadSamples::ScopedMuteThreadSamples() {
  DCHECK(!g_tls_internal_reentry_guard);
  g_tls_internal_reentry_guard = true;

  // We mute thread samples immediately after taking a sample, which is when we
  // reset g_tls_accumulated_bytes. This breaks the random sampling requirement
  // of the poisson process, and causes us to systematically overcount all other
  // allocations. That's because muted allocations rarely trigger a sample
  // [which would cause them to be ignored] since they occur right after
  // g_tls_accumulated_bytes is reset.
  //
  // To counteract this, we drop g_tls_accumulated_bytes by a large, fixed
  // amount to lower the probability that a sample is taken to close to 0. Then
  // we reset it after we're done muting thread samples.
  g_tls_accumulated_bytes_snapshot = g_tls_accumulated_bytes;
  g_tls_accumulated_bytes -= kAccumulatedBytesOffset;
}

PoissonAllocationSampler::ScopedMuteThreadSamples::~ScopedMuteThreadSamples() {
  DCHECK(g_tls_internal_reentry_guard);
  g_tls_internal_reentry_guard = false;
  g_tls_accumulated_bytes = g_tls_accumulated_bytes_snapshot;
}

// static
bool PoissonAllocationSampler::ScopedMuteThreadSamples::IsMuted() {
  return g_tls_internal_reentry_guard;
}

PoissonAllocationSampler::ScopedSuppressRandomnessForTesting::
    ScopedSuppressRandomnessForTesting() {
  DCHECK(!g_deterministic);
  g_deterministic = true;
  // The g_tls_accumulated_bytes may contain a random value from previous
  // test runs, which would make the behaviour of the next call to
  // RecordAlloc unpredictable.
  g_tls_accumulated_bytes = 0;
}

PoissonAllocationSampler::ScopedSuppressRandomnessForTesting::
    ~ScopedSuppressRandomnessForTesting() {
  DCHECK(g_deterministic);
  g_deterministic = false;
}

// static
bool PoissonAllocationSampler::ScopedSuppressRandomnessForTesting::
    IsSuppressed() {
  return g_deterministic;
}

PoissonAllocationSampler::ScopedMuteHookedSamplesForTesting::
    ScopedMuteHookedSamplesForTesting() {
  DCHECK(!g_mute_hooked_samples);
  g_mute_hooked_samples = true;

  // `g_hooks_install_callback` can't be used with
  // ScopedMuteHookedSamplesForTesting because there's no way to remove it.
  DCHECK(!g_hooks_install_callback);

  // Make sure hooks have been installed, so that the only order of operations
  // that needs to be handled is Install Hooks -> Remove Hooks For Testing ->
  // Reinstall Hooks.
  PoissonAllocationSampler::Get()->InstallAllocatorHooksOnce();

  allocator::dispatcher::RemoveStandardAllocatorHooksForTesting();  // IN-TEST

  // Reset the accumulated bytes to 0 on this thread.
  accumulated_bytes_snapshot_ = g_tls_accumulated_bytes;
  g_tls_accumulated_bytes = 0;
}

PoissonAllocationSampler::ScopedMuteHookedSamplesForTesting::
    ~ScopedMuteHookedSamplesForTesting() {
  DCHECK(g_mute_hooked_samples);
  // Restore the allocator hooks and accumulated bytes.
  g_tls_accumulated_bytes = accumulated_bytes_snapshot_;

  allocator::dispatcher::InstallStandardAllocatorHooks();

  g_mute_hooked_samples = false;
}

PoissonAllocationSampler* PoissonAllocationSampler::instance_ = nullptr;

PoissonAllocationSampler::PoissonAllocationSampler() {
  CHECK_EQ(nullptr, instance_);
  instance_ = this;
  Init();
  auto* sampled_addresses = new LockFreeAddressHashSet(64);
  g_sampled_addresses_set.store(sampled_addresses, std::memory_order_release);
}

// static
void PoissonAllocationSampler::Init() {
  [[maybe_unused]] static bool init_once = []() {
    ReentryGuard::Init();
    return true;
  }();
}

void PoissonAllocationSampler::InstallAllocatorHooksOnce() {
  [[maybe_unused]] static bool hook_installed = [] {
    allocator::dispatcher::InstallStandardAllocatorHooks();

    bool expected = false;
    if (!g_hooks_installed.compare_exchange_strong(expected, true)) {
      // SetHooksInstallCallback already ran, so run the callback now.
      g_hooks_install_callback();
    }
    // The allocator hooks use `g_sampled_address_set` so it had better be
    // initialized.
    DCHECK(g_sampled_addresses_set.load(std::memory_order_acquire));
    return true;
  }();
}

// static
void PoissonAllocationSampler::SetHooksInstallCallback(
    void (*hooks_install_callback)()) {
  // `g_hooks_install_callback` can't be used with
  // ScopedMuteHookedSamplesForTesting because there's no way to remove it.
  DCHECK(!g_mute_hooked_samples);

  CHECK(!g_hooks_install_callback && hooks_install_callback);
  g_hooks_install_callback = hooks_install_callback;

  bool expected = false;
  if (!g_hooks_installed.compare_exchange_strong(expected, true)) {
    // InstallAllocatorHooksOnce already ran, so run the callback now.
    g_hooks_install_callback();
  }
}

// static
bool PoissonAllocationSampler::AreHookedSamplesMuted() {
  return g_mute_hooked_samples;
}

void PoissonAllocationSampler::SetSamplingInterval(
    size_t sampling_interval_bytes) {
  // TODO(alph): Reset the sample being collected if running.
  g_sampling_interval = sampling_interval_bytes;
}

size_t PoissonAllocationSampler::SamplingInterval() const {
  return g_sampling_interval.load(std::memory_order_relaxed);
}

// static
size_t PoissonAllocationSampler::GetNextSampleInterval(size_t interval) {
  if (UNLIKELY(g_deterministic))
    return interval;

  // We sample with a Poisson process, with constant average sampling
  // interval. This follows the exponential probability distribution with
  // parameter λ = 1/interval where |interval| is the average number of bytes
  // between samples.
  // Let u be a uniformly distributed random number between 0 and 1, then
  // next_sample = -ln(u) / λ
  double uniform = RandDouble();
  double value = -log(uniform) * interval;
  size_t min_value = sizeof(intptr_t);
  // We limit the upper bound of a sample interval to make sure we don't have
  // huge gaps in the sampling stream. Probability of the upper bound gets hit
  // is exp(-20) ~ 2e-9, so it should not skew the distribution.
  size_t max_value = interval * 20;
  if (UNLIKELY(value < min_value))
    return min_value;
  if (UNLIKELY(value > max_value))
    return max_value;
  return static_cast<size_t>(value);
}

// static
void PoissonAllocationSampler::RecordAlloc(void* address,
                                           size_t size,
                                           AllocatorType type,
                                           const char* context) {
  g_tls_accumulated_bytes += size;
  intptr_t accumulated_bytes = g_tls_accumulated_bytes;
  if (LIKELY(accumulated_bytes < 0))
    return;

  if (UNLIKELY(!g_running.load(std::memory_order_relaxed))) {
    // Sampling is in fact disabled. Reset the state of the sampler.
    // We do this check off the fast-path, because it's quite a rare state when
    // allocation hooks are installed but the sampler is not running.
    g_tls_sampling_interval_initialized = false;
    g_tls_accumulated_bytes = 0;
    return;
  }

  instance_->DoRecordAlloc(accumulated_bytes, size, address, type, context);
}

void PoissonAllocationSampler::DoRecordAlloc(intptr_t accumulated_bytes,
                                             size_t size,
                                             void* address,
                                             AllocatorType type,
                                             const char* context) {
  // Failed allocation? Skip the sample.
  if (UNLIKELY(!address))
    return;

  size_t mean_interval = g_sampling_interval.load(std::memory_order_relaxed);
  if (UNLIKELY(!g_tls_sampling_interval_initialized)) {
    g_tls_sampling_interval_initialized = true;
    // This is the very first allocation on the thread. It always makes it
    // passing the condition at |RecordAlloc|, because g_tls_accumulated_bytes
    // is initialized with zero due to TLS semantics.
    // Generate proper sampling interval instance and make sure the allocation
    // has indeed crossed the threshold before counting it as a sample.
    accumulated_bytes -= GetNextSampleInterval(mean_interval);
    if (accumulated_bytes < 0) {
      g_tls_accumulated_bytes = accumulated_bytes;
      return;
    }
  }

  // This cast is safe because this function is only called with a positive
  // value of `accumulated_bytes`.
  size_t samples = static_cast<size_t>(accumulated_bytes) / mean_interval;
  accumulated_bytes %= mean_interval;

  do {
    accumulated_bytes -= GetNextSampleInterval(mean_interval);
    ++samples;
  } while (accumulated_bytes >= 0);

  g_tls_accumulated_bytes = accumulated_bytes;

  if (UNLIKELY(ScopedMuteThreadSamples::IsMuted()))
    return;

  ScopedMuteThreadSamples no_reentrancy_scope;
  std::vector<SamplesObserver*> observers_copy;
  {
    AutoLock lock(mutex_);

    // TODO(alph): Sometimes RecordAlloc is called twice in a row without
    // a RecordFree in between. Investigate it.
    if (sampled_addresses_set().Contains(address))
      return;
    sampled_addresses_set().Insert(address);
    BalanceAddressesHashSet();
    observers_copy = observers_;
  }

  size_t total_allocated = mean_interval * samples;
  for (auto* observer : observers_copy)
    observer->SampleAdded(address, size, total_allocated, type, context);
}

void PoissonAllocationSampler::DoRecordFree(void* address) {
  if (UNLIKELY(ScopedMuteThreadSamples::IsMuted()))
    return;
  // There is a rare case on macOS and Android when the very first thread_local
  // access in ScopedMuteThreadSamples constructor may allocate and
  // thus reenter DoRecordAlloc. However the call chain won't build up further
  // as RecordAlloc accesses are guarded with pthread TLS-based ReentryGuard.
  ScopedMuteThreadSamples no_reentrancy_scope;
  std::vector<SamplesObserver*> observers_copy;
  {
    AutoLock lock(mutex_);
    observers_copy = observers_;
    sampled_addresses_set().Remove(address);
  }
  for (auto* observer : observers_copy)
    observer->SampleRemoved(address);
}

void PoissonAllocationSampler::BalanceAddressesHashSet() {
  // Check if the load_factor of the current addresses hash set becomes higher
  // than 1, allocate a new twice larger one, copy all the data,
  // and switch to using it.
  // During the copy process no other writes are made to both sets
  // as it's behind the lock.
  // All the readers continue to use the old one until the atomic switch
  // process takes place.
  LockFreeAddressHashSet& current_set = sampled_addresses_set();
  if (current_set.load_factor() < 1)
    return;
  auto new_set =
      std::make_unique<LockFreeAddressHashSet>(current_set.buckets_count() * 2);
  new_set->Copy(current_set);
  // Atomically switch all the new readers to the new set.
  g_sampled_addresses_set.store(new_set.release(), std::memory_order_release);
  // We leak the older set because we still have to keep all the old maps alive
  // as there might be reader threads that have already obtained the map,
  // but haven't yet managed to access it.
}

// static
LockFreeAddressHashSet& PoissonAllocationSampler::sampled_addresses_set() {
  return *g_sampled_addresses_set.load(std::memory_order_acquire);
}

// static
PoissonAllocationSampler* PoissonAllocationSampler::Get() {
  static NoDestructor<PoissonAllocationSampler> instance;
  return instance.get();
}

void PoissonAllocationSampler::AddSamplesObserver(SamplesObserver* observer) {
  // The following implementation (including ScopedMuteThreadSamples) will use
  // `thread_local`, which may cause a reentrancy issue.  So, temporarily
  // disable the sampling by having a ReentryGuard.
  ReentryGuard guard;

  ScopedMuteThreadSamples no_reentrancy_scope;
  AutoLock lock(mutex_);
  DCHECK(ranges::find(observers_, observer) == observers_.end());
  observers_.push_back(observer);
  InstallAllocatorHooksOnce();
  g_running = !observers_.empty();
}

void PoissonAllocationSampler::RemoveSamplesObserver(
    SamplesObserver* observer) {
  // The following implementation (including ScopedMuteThreadSamples) will use
  // `thread_local`, which may cause a reentrancy issue.  So, temporarily
  // disable the sampling by having a ReentryGuard.
  ReentryGuard guard;

  ScopedMuteThreadSamples no_reentrancy_scope;
  AutoLock lock(mutex_);
  auto it = ranges::find(observers_, observer);
  DCHECK(it != observers_.end());
  observers_.erase(it);
  g_running = !observers_.empty();
}

}  // namespace base

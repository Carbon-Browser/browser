// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/compute_pressure/pressure_sampler.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/check_op.h"
#include "base/run_loop.h"
#include "base/sequence_checker.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/thread_annotations.h"
#include "base/threading/platform_thread.h"
#include "base/threading/scoped_blocking_call.h"
#include "build/build_config.h"
#include "services/device/compute_pressure/cpu_probe.h"
#include "services/device/compute_pressure/pressure_sample.h"
#include "services/device/compute_pressure/pressure_test_support.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class PressureSamplerTest : public testing::Test {
 public:
  PressureSamplerTest()
      : sampler_(std::make_unique<PressureSampler>(
            std::make_unique<FakeCpuProbe>(),
            base::Milliseconds(1),
            base::BindRepeating(&PressureSamplerTest::SamplerCallback,
                                base::Unretained(this)))) {}

  void WaitForUpdate() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    base::RunLoop run_loop;
    SetNextUpdateCallback(run_loop.QuitClosure());
    run_loop.Run();
  }

  // Only valid if `sampler_` uses a FakeCpuProbe. This is guaranteed if
  // `sampler_` is not replaced during the test.
  FakeCpuProbe& cpu_probe() {
    auto* cpu_probe =
        static_cast<FakeCpuProbe*>(sampler_->cpu_probe_for_testing());
    DCHECK(cpu_probe);
    return *cpu_probe;
  }

  void SamplerCallback(PressureSample sample) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    samples_.push_back(sample);
    if (update_callback_) {
      std::move(update_callback_).Run();
      update_callback_.Reset();
    }
  }

 protected:
  SEQUENCE_CHECKER(sequence_checker_);

  base::test::TaskEnvironment task_environment_;

  // This member is a std::unique_ptr instead of a plain PressureSampler
  // so it can be replaced inside tests.
  std::unique_ptr<PressureSampler> sampler_;

  // The samples reported by the callback.
  std::vector<PressureSample> samples_ GUARDED_BY_CONTEXT(sequence_checker_);

 private:
  void SetNextUpdateCallback(base::OnceClosure callback) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(!update_callback_)
        << __func__ << " already called before update received";
    update_callback_ = std::move(callback);
  }

  // Used to implement WaitForUpdate().
  base::OnceClosure update_callback_ GUARDED_BY_CONTEXT(sequence_checker_);
};

TEST_F(PressureSamplerTest, EnsureStarted) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sampler_->EnsureStarted();
  WaitForUpdate();

  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.42}));
}

namespace {

// TestDouble for CpuProbe that produces a different value after every Update().
class StreamingCpuProbe : public CpuProbe {
 public:
  explicit StreamingCpuProbe(std::vector<PressureSample> samples,
                             base::OnceClosure callback)
      : samples_(std::move(samples)), callback_(std::move(callback)) {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    DCHECK_GT(samples_.size(), 0u);
  }
  ~StreamingCpuProbe() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  // CpuProbe implementation.
  void Update() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    ++sample_index_;
    base::ScopedBlockingCall scoped_blocking_call(
        FROM_HERE, base::BlockingType::MAY_BLOCK);
  }

  PressureSample LastSample() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (sample_index_ < samples_.size()) {
      return samples_.at(sample_index_);
    }

    if (!callback_.is_null()) {
      std::move(callback_).Run();
    }

    return samples_.back();
  }

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  std::vector<PressureSample> samples_ GUARDED_BY_CONTEXT(sequence_checker_);
  size_t sample_index_ GUARDED_BY_CONTEXT(sequence_checker_) = 0;

  // This closure is called on a LastSample call after expected number of
  // samples has been taken by PressureSampler.
  base::OnceClosure callback_;
};

}  // namespace

TEST_F(PressureSamplerTest, EnsureStarted_SkipsFirstSample) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::vector<PressureSample> samples = {
      // Value right after construction.
      PressureSample{0.1},
      // Value after first Update(), should be discarded.
      PressureSample{0.2},
      // Value after second Update(), should be reported.
      PressureSample{0.4},
  };

  base::RunLoop run_loop;
  sampler_ = std::make_unique<PressureSampler>(
      std::make_unique<StreamingCpuProbe>(samples, run_loop.QuitClosure()),
      base::Milliseconds(1),
      base::BindRepeating(&PressureSamplerTest::SamplerCallback,
                          base::Unretained(this)));
  sampler_->EnsureStarted();
  run_loop.Run();

  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Not(testing::Contains(PressureSample{0.2})));
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.4}));
}

// TODO(crbug.com/1271419): Flaky.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_FUCHSIA)
#define MAYBE_Stop_Delayed_EnsureStarted_Immediate \
  DISABLED_Stop_Delayed_EnsureStarted_Immediate
#else
#define MAYBE_Stop_Delayed_EnsureStarted_Immediate \
  Stop_Delayed_EnsureStarted_Immediate
#endif
TEST_F(PressureSamplerTest, MAYBE_Stop_Delayed_EnsureStarted_Immediate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sampler_->EnsureStarted();
  WaitForUpdate();
  sampler_->Stop();

  samples_.clear();
  cpu_probe().SetLastSample(PressureSample{0.25});

  sampler_->EnsureStarted();
  WaitForUpdate();
  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.25}));
}

TEST_F(PressureSamplerTest, Stop_Delayed_EnsureStarted_Delayed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sampler_->EnsureStarted();
  WaitForUpdate();
  sampler_->Stop();

  samples_.clear();
  cpu_probe().SetLastSample(PressureSample{0.25});
  // 10ms should be long enough to ensure that all the sampling tasks are done.
  base::PlatformThread::Sleep(base::Milliseconds(10));

  sampler_->EnsureStarted();
  WaitForUpdate();
  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.25}));
}

TEST_F(PressureSamplerTest, Stop_Immediate_EnsureStarted_Immediate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sampler_->EnsureStarted();
  sampler_->Stop();

  samples_.clear();
  cpu_probe().SetLastSample(PressureSample{0.25});

  sampler_->EnsureStarted();
  WaitForUpdate();
  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.25}));
}

TEST_F(PressureSamplerTest, Stop_Immediate_EnsureStarted_Delayed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sampler_->EnsureStarted();
  sampler_->Stop();

  samples_.clear();
  cpu_probe().SetLastSample(PressureSample{0.25});
  // 10ms should be long enough to ensure that all the sampling tasks are done.
  base::PlatformThread::Sleep(base::Milliseconds(10));

  sampler_->EnsureStarted();
  WaitForUpdate();
  EXPECT_GE(samples_.size(), 1u);
  EXPECT_THAT(samples_, testing::Contains(PressureSample{0.25}));
}

}  // namespace device

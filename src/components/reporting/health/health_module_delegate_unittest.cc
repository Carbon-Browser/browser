// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reporting/health/health_module_delegate.h"

#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::StrEq;

namespace reporting {
namespace {

constexpr char kBaseFileOne[] = "base";
constexpr uint32_t kDefaultCallSize = 10;
constexpr uint32_t kRepeatedPtrFieldSizeOverhead = 2;
constexpr uint32_t kMaxWriteCount = 10;
constexpr uint32_t kMaxStorage =
    kMaxWriteCount * (kRepeatedPtrFieldSizeOverhead + kDefaultCallSize);
constexpr uint32_t kTinyStorage = 2;

const char kHexCharLookup[0x10] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

std::string BytesToHexString(base::StringPiece bytes) {
  std::string result;
  for (char byte : bytes) {
    result.push_back(kHexCharLookup[(byte >> 4) & 0xf]);
    result.push_back(kHexCharLookup[byte & 0xf]);
  }
  return result;
}

void CompareHealthData(base::StringPiece expected, ERPHealthData got) {
  EXPECT_THAT(expected, testing::StrEq(got.SerializeAsString()));
}

class HealthModuleDelegateTest : public ::testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

  HealthDataHistory AddEnqueueRecordCall() {
    HealthDataHistory history;
    EnqueueRecordCall call;
    call.set_priority(static_cast<Priority>(1));
    *history.mutable_enqueue_record_call() = call;
    history.set_timestamp_seconds(base::Time::Now().ToTimeT());
    return history;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(HealthModuleDelegateTest, TestInit) {
  ERPHealthData ref_data;
  const std::string file_name = base::StrCat({kBaseFileOne, "0"});
  auto call = AddEnqueueRecordCall();
  *ref_data.add_history() = call;
  ASSERT_TRUE(
      ::reporting::AppendLine(temp_dir_.GetPath().AppendASCII(file_name),
                              BytesToHexString(call.SerializeAsString()))
          .ok());

  HealthModuleDelegate delegate(temp_dir_.GetPath(), kBaseFileOne, kMaxStorage);
  ASSERT_FALSE(delegate.IsInitialized());

  delegate.Init();
  ASSERT_TRUE(delegate.IsInitialized());
  delegate.GetERPHealthData(
      base::BindOnce(CompareHealthData, ref_data.SerializeAsString()));
}

TEST_F(HealthModuleDelegateTest, TestWrite) {
  ERPHealthData ref_data;
  HealthModuleDelegate delegate(temp_dir_.GetPath(), kBaseFileOne, kMaxStorage);
  ASSERT_FALSE(delegate.IsInitialized());

  // Can not post before initiating.
  delegate.PostHealthRecord(AddEnqueueRecordCall());
  delegate.GetERPHealthData(
      base::BindOnce(CompareHealthData, ref_data.SerializeAsString()));

  delegate.Init();
  ASSERT_TRUE(delegate.IsInitialized());

  // Fill the local storage.
  for (uint32_t i = 0; i < kMaxWriteCount; i++) {
    auto call = AddEnqueueRecordCall();
    *ref_data.add_history() = call;
    delegate.PostHealthRecord(call);
  }
  delegate.GetERPHealthData(
      base::BindOnce(CompareHealthData, ref_data.SerializeAsString()));

  // Overwrite half of the local storage.
  for (uint32_t i = 0; i < kMaxWriteCount / 2; i++) {
    auto call = AddEnqueueRecordCall();
    *ref_data.add_history() = call;
    delegate.PostHealthRecord(call);
  }
  ref_data.mutable_history()->DeleteSubrange(0, kMaxWriteCount / 2);
  delegate.GetERPHealthData(
      base::BindOnce(CompareHealthData, ref_data.SerializeAsString()));
}

TEST_F(HealthModuleDelegateTest, TestOversizedWrite) {
  ERPHealthData ref_data;
  HealthModuleDelegate delegate(temp_dir_.GetPath(), kBaseFileOne,
                                kTinyStorage);

  delegate.PostHealthRecord(AddEnqueueRecordCall());
  delegate.GetERPHealthData(
      base::BindOnce(CompareHealthData, ref_data.SerializeAsString()));
}
}  // namespace
}  // namespace reporting
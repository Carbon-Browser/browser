// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "third_party/blink/renderer/platform/peerconnection/rtc_stats.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/peerconnection/rtc_stats.h"
#include "third_party/webrtc/api/stats/rtc_stats_report.h"
#include "third_party/webrtc/api/stats/rtcstats_objects.h"
#include "third_party/webrtc/stats/test/rtc_test_stats.h"

namespace blink {

TEST(RTCStatsTest, OnlyIncludeAllowlistedStats_GetStats) {
  const char* not_allowlisted_id = "NotAllowlistedId";
  const char* allowlisted_id = "AllowlistedId";

  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(42);
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCTestStats>(not_allowlisted_id, 42));
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCPeerConnectionStats>(allowlisted_id, 42));

  RTCStatsReportPlatform report(webrtc_report.get(), {});
  EXPECT_FALSE(report.GetStats(not_allowlisted_id));
  EXPECT_TRUE(report.GetStats(allowlisted_id));
}

TEST(RTCStatsTest, OnlyIncludeAllowlistedStats_Iteration) {
  const char* not_allowlisted_id = "NotAllowlistedId";
  const char* allowlisted_id = "AllowlistedId";

  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(42);
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCTestStats>(not_allowlisted_id, 42));
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCPeerConnectionStats>(allowlisted_id, 42));

  RTCStatsReportPlatform report(webrtc_report.get(), {});
  // Only allowlisted stats are counted.
  EXPECT_EQ(report.Size(), 1u);

  std::unique_ptr<RTCStats> stats = report.Next();
  EXPECT_TRUE(stats);
  EXPECT_EQ(stats->Id(), allowlisted_id);
  EXPECT_FALSE(report.Next());
}

// Stats object with both a standard and non-standard member, used for the test
// below.
namespace {
class TestStats : public webrtc::RTCStats {
 public:
  WEBRTC_RTCSTATS_DECL();

  TestStats(const std::string& id, int64_t timestamp_us);
  ~TestStats() override = default;

  webrtc::RTCStatsMember<int32_t> standardized;
  webrtc::RTCNonStandardStatsMember<int32_t> non_standardized;
};

WEBRTC_RTCSTATS_IMPL(TestStats,
                     webrtc::RTCStats,
                     "teststats",
                     &standardized,
                     &non_standardized)

TestStats::TestStats(const std::string& id, int64_t timestamp_us)
    : RTCStats(id, timestamp_us),
      standardized("standardized"),
      non_standardized("non_standardized",
                       {webrtc::NonStandardGroupId::kGroupIdForTesting}) {}
}  // namespace

// Similar to how only allowlisted stats objects should be surfaced, only
// standardized members of the allowlisted objects should be surfaced.
TEST(RTCStatsTest, OnlyIncludeStandarizedMembers) {
  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(42);
  AllowStatsForTesting(TestStats::kType);
  webrtc_report->AddStats(std::make_unique<TestStats>("id", 0));

  // TestStats has two members, but the non-standard member should be filtered
  // out.
  RTCStatsReportPlatform report(webrtc_report.get(), {});
  std::unique_ptr<RTCStats> stats = report.Next();
  ASSERT_NE(nullptr, stats);
  ASSERT_EQ(1u, stats->MembersCount());
  EXPECT_EQ("standardized", stats->GetMember(0)->GetName());
}

TEST(RTCStatsTest, IncludeAllMembers) {
  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(7);
  AllowStatsForTesting(TestStats::kType);
  webrtc_report->AddStats(std::make_unique<TestStats>("id", 0));

  // Include both standard and non-standard member.
  RTCStatsReportPlatform report(
      webrtc_report.get(), Vector<webrtc::NonStandardGroupId>{
                               webrtc::NonStandardGroupId::kGroupIdForTesting});
  std::unique_ptr<RTCStats> stats = report.GetStats("id");
  ASSERT_NE(nullptr, stats);
  ASSERT_EQ(2u, stats->MembersCount());
  EXPECT_EQ("standardized", stats->GetMember(0)->GetName());
  EXPECT_EQ("non_standardized", stats->GetMember(1)->GetName());
}

TEST(RTCStatsTest, IncludeAllMembersFeatureFlag) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kWebRtcExposeNonStandardStats);

  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(7);
  AllowStatsForTesting(TestStats::kType);
  webrtc_report->AddStats(std::make_unique<TestStats>("id", 0));

  // Include both standard and non-standard member.
  RTCStatsReportPlatform report(
      webrtc_report.get(), Vector<webrtc::NonStandardGroupId>{
                               webrtc::NonStandardGroupId::kGroupIdForTesting});
  std::unique_ptr<RTCStats> stats = report.GetStats("id");
  ASSERT_NE(nullptr, stats);
  ASSERT_EQ(2u, stats->MembersCount());
  EXPECT_EQ("standardized", stats->GetMember(0)->GetName());
  EXPECT_EQ("non_standardized", stats->GetMember(1)->GetName());
}

TEST(RTCStatsTest, CopyHandle) {
  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(17);
  AllowStatsForTesting(TestStats::kType);
  webrtc_report->AddStats(std::make_unique<TestStats>("id", 0));

  // Check that filtering options are preserved during copy.
  RTCStatsReportPlatform standard_members_report(webrtc_report.get(), {});
  std::unique_ptr<RTCStatsReportPlatform> standard_members_copy =
      standard_members_report.CopyHandle();

  ASSERT_EQ(1u, standard_members_report.GetStats("id")->MembersCount());
  ASSERT_EQ(1u, standard_members_copy->GetStats("id")->MembersCount());

  RTCStatsReportPlatform all_members_report(
      webrtc_report.get(), Vector<webrtc::NonStandardGroupId>{
                               webrtc::NonStandardGroupId::kGroupIdForTesting});
  std::unique_ptr<RTCStatsReportPlatform> all_members_copy =
      all_members_report.CopyHandle();
  ASSERT_EQ(2u, all_members_report.GetStats("id")->MembersCount());
  ASSERT_EQ(2u, all_members_copy->GetStats("id")->MembersCount());
}

}  // namespace blink

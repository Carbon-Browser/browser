/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/content/browser/session_stats_impl.h"

#include "base/test/mock_callback.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

namespace {

constexpr char kAllowedTestSub[] = "http://allowed.sub.com/";
constexpr char kBlockedTestSub[] = "http://blocked.sub.com/";

class FakeResourceClassificationRunner : public ResourceClassificationRunner {
 public:
  MOCK_METHOD((mojom::FilterMatchResult),
              ShouldBlockPopup,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL& opener,
               const GURL& popup_url,
               content::RenderFrameHost* render_frame_host),
              (override));

  MOCK_METHOD(void,
              CheckRequestFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL& request_url,
               int32_t resource_type,
               int32_t process_id,
               int32_t render_frame_id,
               mojom::AdblockInterface::CheckFilterMatchCallback callback),
              (override));

  MOCK_METHOD(void,
              CheckRequestFilterMatchForWebSocket,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL& request_url,
               content::RenderFrameHost* render_frame_host,
               mojom::AdblockInterface::CheckFilterMatchCallback callback),
              (override));

  MOCK_METHOD(void,
              CheckResponseFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL& request_url,
               int32_t process_id,
               int32_t render_frame_id,
               const scoped_refptr<net::HttpResponseHeaders>& headers,
               mojom::AdblockInterface::CheckFilterMatchCallback callback),
              (override));

  MOCK_METHOD(void,
              CheckRewriteFilterMatch,
              (std::unique_ptr<SubscriptionCollection>,
               const GURL& url,
               int32_t process_id,
               int32_t render_frame_id,
               base::OnceCallback<void(const absl::optional<GURL>&)> result),
              (override));

  void AddObserver(Observer* observer) override {
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) override {
    observers_.RemoveObserver(observer);
  }

  void NotifyAdMatched(const GURL& url,
                       mojom::FilterMatchResult result,
                       const std::vector<GURL>& parent_frame_urls,
                       ContentType content_type,
                       content::RenderFrameHost* render_frame_host,
                       const GURL& subscription) {
    for (auto& observer : observers_) {
      observer.OnAdMatched(url, result, parent_frame_urls,
                           static_cast<ContentType>(content_type),
                           render_frame_host, subscription);
    }
  }

 private:
  base::ObserverList<Observer> observers_;
};

}  // namespace

class AdblockSessionStatsTest : public testing::Test {
 public:
  AdblockSessionStatsTest() = default;

  void SetUp() override {
    session_stats_ = std::make_unique<SessionStatsImpl>(&classfier_);
  }

  FakeResourceClassificationRunner classfier_;
  std::unique_ptr<SessionStatsImpl> session_stats_;
};

TEST_F(AdblockSessionStatsTest, StatsDataCollected) {
  EXPECT_TRUE(session_stats_->GetSessionAllowedAdsCount().empty());
  EXPECT_TRUE(session_stats_->GetSessionBlockedAdsCount().empty());

  classfier_.NotifyAdMatched(GURL(), mojom::FilterMatchResult::kAllowRule,
                             std::vector<GURL>(), ContentType::Subdocument,
                             nullptr, GURL());

  classfier_.NotifyAdMatched(GURL(), mojom::FilterMatchResult::kBlockRule,
                             std::vector<GURL>(), ContentType::Subdocument,
                             nullptr, GURL());

  // Before call to StartCollectingStats()
  EXPECT_TRUE(session_stats_->GetSessionAllowedAdsCount().empty());
  EXPECT_TRUE(session_stats_->GetSessionBlockedAdsCount().empty());

  session_stats_->StartCollectingStats();

  classfier_.NotifyAdMatched(GURL(), mojom::FilterMatchResult::kAllowRule,
                             std::vector<GURL>(), ContentType::Subdocument,
                             nullptr, GURL{kAllowedTestSub});

  classfier_.NotifyAdMatched(GURL(), mojom::FilterMatchResult::kBlockRule,
                             std::vector<GURL>(), ContentType::Subdocument,
                             nullptr, GURL{kBlockedTestSub});

  auto allowed_result = session_stats_->GetSessionAllowedAdsCount();
  auto blocked_result = session_stats_->GetSessionBlockedAdsCount();

  EXPECT_EQ((std::map<GURL, long>{{GURL(kAllowedTestSub), 1}}), allowed_result);
  EXPECT_EQ((std::map<GURL, long>{{GURL(kBlockedTestSub), 1}}), blocked_result);

  classfier_.NotifyAdMatched(GURL(), mojom::FilterMatchResult::kBlockRule,
                             std::vector<GURL>(), ContentType::Subdocument,
                             nullptr, GURL{kBlockedTestSub});

  allowed_result = session_stats_->GetSessionAllowedAdsCount();
  blocked_result = session_stats_->GetSessionBlockedAdsCount();

  EXPECT_EQ((std::map<GURL, long>{{GURL(kAllowedTestSub), 1}}), allowed_result);
  EXPECT_EQ((std::map<GURL, long>{{GURL(kBlockedTestSub), 2}}), blocked_result);
}

}  // namespace adblock

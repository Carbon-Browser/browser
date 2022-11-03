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

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/adblock_element_hider.h"
#include "components/adblock/adblock_prefs.h"
#include "components/adblock/adblock_request_classifier_libabp_impl.h"
#include "components/adblock/fake_subscription_impl.h"
#include "components/adblock/mock_adblock_controller.h"
#include "components/adblock/mock_adblock_element_hider.h"
#include "components/adblock/mock_adblock_frame_hierarchy_builder.h"
#include "components/adblock/mock_adblock_platform_embedder.h"
#include "components/adblock/mock_adblock_sitekey_storage.h"
#include "components/adblock/mock_filter_engine.h"
#include "components/adblock/mock_filter_impl.h"
#include "components/adblock/sitekey.h"
#include "components/prefs/pref_member.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/Filter.h"

using namespace AdblockPlus;
using namespace adblock;
using namespace testing;

namespace adblock {

constexpr char kTestPopupURL[] = "http://popup.example.com";
constexpr char kTestURL[] = "http://example.com";
constexpr char kTestReferrer[] = "https://example.referrer.com";

class MockObserver : public AdblockRequestClassifier::Observer {
 public:
  MOCK_METHOD(void,
              OnAdMatched,
              (const GURL& url,
               mojom::FilterMatchResult match_result,
               const std::vector<GURL>& parent_frame_urls,
               ContentType content_type,
               content::RenderFrameHost* render_frame_host,
               const std::vector<GURL>& subscriptions),
              (override));
};

class AdblockRequestClassifierLibabpTest
    : public content::RenderViewHostTestHarness {
 public:
  AdblockRequestClassifierLibabpTest() = default;
  ~AdblockRequestClassifierLibabpTest() override = default;

  void SetUp() override {
    RenderViewHostTestHarness::SetUp();
    std::unique_ptr<MockFilterEngine> mock_engine =
        std::make_unique<MockFilterEngine>();
    mock_engine_ = mock_engine.get();
    embedder_ = new MockAdblockPlatformEmbedder(
        std::move(mock_engine), task_environment()->GetMainThreadTaskRunner());

    std::unique_ptr<MockFrameHierarchyBuilder> mock_frame_hierarchy_builder =
        std::make_unique<MockFrameHierarchyBuilder>();
    mock_frame_hierarchy_builder_ = mock_frame_hierarchy_builder.get();

    classifier_ = std::make_unique<adblock::AdblockRequestClassifierLibabpImpl>(
        embedder_, std::move(mock_frame_hierarchy_builder), &mock_hider_,
        &mock_controller_, &mock_sitekey_storage_);
    classifier_->AddObserver(&mock_observer_);

    EXPECT_CALL(mock_controller_, IsAdblockEnabled())
        .WillRepeatedly(Return(true));
  }

  void ExpectSiteKeyFoundForFrameHierarchy(
      const std::vector<GURL>& frame_hierarchy,
      SiteKey sitekey) {
    EXPECT_CALL(mock_sitekey_storage_, FindSiteKeyForAnyUrl(frame_hierarchy))
        .WillRepeatedly(
            Return(std::make_pair(frame_hierarchy.front(), sitekey)));
  }

  void DoPopupBlockingTest(const Filter& filter,
                           mojom::FilterMatchResult result) {
    GURL popup = GURL(kTestPopupURL);
    GURL opener = GURL(kTestURL);
    GURL test_subscription_url = GURL{kTestURL};
    std::vector<GURL> subscriptions_urls{test_subscription_url};
    std::unique_ptr<FakeSubscriptionImpl> mock_subscription =
        std::make_unique<FakeSubscriptionImpl>(test_subscription_url, "title",
                                               std::vector<std::string>{"pl"},
                                               kSynchronizationOkStatus, false);
    AdblockPlus::Subscription subscription(std::move(mock_subscription));
    std::vector<AdblockPlus::Subscription> subscriptions{subscription};
    EXPECT_CALL(
        *mock_engine_,
        Matches(popup.spec(),
                AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_POPUP,
                opener.spec(), _, _))
        .WillOnce(Return(filter));
    if (result == mojom::FilterMatchResult::kBlockRule ||
        result == mojom::FilterMatchResult::kAllowRule) {
      EXPECT_CALL(*mock_engine_, GetSubscriptionsFromFilter(filter))
          .WillOnce(Return(subscriptions));
      EXPECT_CALL(mock_observer_,
                  OnAdMatched(popup, result, _, CONTENT_TYPE_POPUP, main_rfh(),
                              subscriptions_urls));
    }
    auto test_result = classifier_->ShouldBlockPopup(opener, popup, main_rfh());
    // Needed due to test env DCHECK()ing othewise because of ShouldBlockPopup()
    // posting a task to UI with notification about blocked/allowed.
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(test_result, result);
  }

  void DoFilterMatchTest(Filter filter,
                         bool allowlisting_result,
                         bool genericblock_match_result,
                         mojom::FilterMatchResult expected) {
    base::RunLoop loop;
    GURL referrer = GURL{kTestReferrer};
    GURL url = GURL{kTestURL};

    GURL test_subscription_url = GURL{kTestURL};
    std::vector<GURL> subscriptions_urls{test_subscription_url};
    std::vector<GURL> frame_hierarchy{referrer};
    std::unique_ptr<FakeSubscriptionImpl> mock_subscription =
        std::make_unique<FakeSubscriptionImpl>(test_subscription_url, "title",
                                               std::vector<std::string>{"pl"},
                                               kSynchronizationOkStatus, false);
    AdblockPlus::Subscription subscription(std::move(mock_subscription));
    std::vector<AdblockPlus::Subscription> subscriptions{subscription};
    if (expected == mojom::FilterMatchResult::kBlockRule) {
      EXPECT_CALL(mock_observer_,
                  OnAdMatched(url, mojom::FilterMatchResult::kBlockRule,
                              frame_hierarchy, CONTENT_TYPE_STYLESHEET,
                              main_rfh(), subscriptions_urls));
      // After stopping the resource from loading, the testee will collapse the
      // blank space.
      EXPECT_CALL(mock_hider_, IsElementTypeHideable(CONTENT_TYPE_STYLESHEET))
          .WillOnce(Return(true));
      EXPECT_CALL(mock_hider_, HideBlockedElement(url, main_rfh()));
    } else {
      EXPECT_CALL(
          mock_observer_,
          OnAdMatched(url, mojom::FilterMatchResult::kAllowRule,
                      frame_hierarchy, CONTENT_TYPE_STYLESHEET, main_rfh(),
                      ((!allowlisting_result && !genericblock_match_result)
                           ? subscriptions_urls
                           : std::vector<GURL>{})));
      // If the resource isn't blocked, the testee will not collapse the blank
      // space.
      EXPECT_CALL(mock_hider_, IsElementTypeHideable(_)).Times(0);
      EXPECT_CALL(mock_hider_, HideBlockedElement(_, _)).Times(0);
    }
    EXPECT_CALL(*mock_frame_hierarchy_builder_,
                FindRenderFrameHost(main_rfh()->GetProcess()->GetID(),
                                    main_rfh()->GetRoutingID()))
        .WillOnce(Return(main_rfh()));
    EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
        .WillOnce(Return(frame_hierarchy));
    ExpectSiteKeyFoundForFrameHierarchy(frame_hierarchy, SiteKey("key"));
    if (filter.GetType() == AdblockPlus::Filter::Type::TYPE_EXCEPTION) {
      EXPECT_CALL(
          *mock_engine_,
          IsContentAllowlisted(
              url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT,
              std::vector<std::string>{referrer.spec()}, "key"))
          .Times(0);
    } else {
      EXPECT_CALL(
          *mock_engine_,
          IsContentAllowlisted(
              url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT,
              std::vector<std::string>{referrer.spec()}, "key"))
          .WillOnce(Return(allowlisting_result));
    }
    EXPECT_CALL(
        *mock_engine_,
        IsContentAllowlisted(
            url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
            std::vector<std::string>{referrer.spec()}, "key"))
        .WillOnce(Return(genericblock_match_result));
    EXPECT_CALL(
        *mock_engine_,
        Matches(
            url.spec(),
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_STYLESHEET,
            referrer.spec(), "key", genericblock_match_result))
        .WillOnce(Return(filter));
    if (!allowlisting_result && !genericblock_match_result) {
      EXPECT_CALL(*mock_engine_, GetSubscriptionsFromFilter(filter))
          .WillOnce(Return(subscriptions));
    }
    classifier_->CheckFilterMatch(
        url, static_cast<int>(blink::mojom::ResourceType::kStylesheet),
        main_rfh()->GetProcess()->GetID(), main_rfh()->GetRoutingID(),
        base::BindLambdaForTesting(
            [&loop, &expected](mojom::FilterMatchResult result) {
              EXPECT_EQ(expected, result);
              loop.QuitWhenIdle();
            }));
    loop.Run();
  }

  AdblockRequestClassifierLibabpTest(
      const AdblockRequestClassifierLibabpTest&) = delete;
  AdblockRequestClassifierLibabpTest& operator=(
      const AdblockRequestClassifierLibabpTest&) = delete;

  MockFilterEngine* mock_engine_;
  MockAdblockElementHider mock_hider_;
  MockObserver mock_observer_;
  MockAdblockController mock_controller_;
  MockAdblockSitekeyStorage mock_sitekey_storage_;
  MockFrameHierarchyBuilder* mock_frame_hierarchy_builder_;
  std::unique_ptr<AdblockRequestClassifierLibabpImpl> classifier_;
  scoped_refptr<MockAdblockPlatformEmbedder> embedder_;
};

TEST_F(AdblockRequestClassifierLibabpTest, PopupBlockingWhenABPDisabled) {
  EXPECT_CALL(mock_controller_, IsAdblockEnabled())
      .WillRepeatedly(Return(false));
  GURL popup = GURL(kTestPopupURL);
  GURL opener = GURL(kTestURL);
  EXPECT_CALL(
      *mock_engine_,
      Matches(popup.spec(),
              AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_POPUP,
              opener.spec(), _, _))
      .Times(0);
  auto result = classifier_->ShouldBlockPopup(popup, opener, main_rfh());
  EXPECT_EQ(result, adblock::mojom::FilterMatchResult::kDisabled);
}

TEST_F(AdblockRequestClassifierLibabpTest, PopupBlockingEnabledNoRule) {
  DoPopupBlockingTest(Filter(), adblock::mojom::FilterMatchResult::kNoRule);
}

TEST_F(AdblockRequestClassifierLibabpTest,
       PopupBlockingWithoutFilterEngineYieldsNoRule) {
  GURL popup = GURL(kTestPopupURL);
  GURL opener = GURL(kTestURL);
  EXPECT_CALL(
      *mock_engine_,
      Matches(popup.spec(),
              AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_POPUP,
              opener.spec(), _, _))
      .Times(0);

  embedder_->SetFilterEngine(nullptr);

  auto result = classifier_->ShouldBlockPopup(popup, opener, main_rfh());
  EXPECT_EQ(result, adblock::mojom::FilterMatchResult::kNoRule);
}

TEST_F(AdblockRequestClassifierLibabpTest, PopupBlockingEnabledAllowRule) {
  DoPopupBlockingTest(Filter(std::make_unique<MockFilterImpl>(
                          "", Filter::Type::TYPE_EXCEPTION)),
                      adblock::mojom::FilterMatchResult::kAllowRule);
}

TEST_F(AdblockRequestClassifierLibabpTest, PopupBlockingEnabledBlockRule) {
  DoPopupBlockingTest(
      Filter(std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_BLOCKING)),
      mojom::FilterMatchResult::kBlockRule);
}

TEST_F(AdblockRequestClassifierLibabpTest,
       HasMatchingFilterWithoutAndWithFilterEngine) {
  // without filter engine
  std::unique_ptr<MockFilterEngine> engine = embedder_->TakeFilterEngine();
  embedder_->SetFilterEngine(nullptr);

  GURL url = GURL{kTestURL};
  base::MockOnceCallback<void(bool)> result_callback;

  {
    base::RunLoop loop;
    EXPECT_CALL(*mock_engine_, Matches(_, _, _, _, _)).Times(0);
    EXPECT_CALL(result_callback, Run(_)).Times(0);

    classifier_->HasMatchingFilter(url, CONTENT_TYPE_DOCUMENT, url,
                                   SiteKey{"key_sig"}, false,
                                   result_callback.Get());

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    embedder_->SetFilterEngine(std::move(engine));
    Filter filter(
        std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_BLOCKING));

    EXPECT_CALL(
        *mock_engine_,
        Matches(url.spec(),
                AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_DOCUMENT,
                url.spec(), "key_sig", false))
        .Times(1)
        .WillOnce(Return(filter));
    EXPECT_CALL(result_callback, Run(true)).Times(1);

    embedder_->NotifyOnEngineCreated();

    loop.RunUntilIdle();
  }
}

TEST_F(AdblockRequestClassifierLibabpTest, CheckFilterMatchDocumentAllowed) {
  base::RunLoop loop;
  GURL referrer = GURL{kTestReferrer};
  GURL url = GURL{kTestURL};
  std::vector<GURL> frame_hierarchy{referrer};

  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetProcess()->GetID(),
                                  main_rfh()->GetRoutingID()))
      .WillOnce(Return(main_rfh()));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(frame_hierarchy));
  EXPECT_CALL(
      mock_observer_,
      OnAdMatched(url, mojom::FilterMatchResult::kAllowRule, frame_hierarchy,
                  CONTENT_TYPE_STYLESHEET, main_rfh(), std::vector<GURL>{}));
  ExpectSiteKeyFoundForFrameHierarchy(frame_hierarchy, SiteKey("key"));
  EXPECT_CALL(
      *mock_engine_,
      IsContentAllowlisted(
          url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
          std::vector<std::string>{referrer.spec()}, "key"))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_hider_, IsElementTypeHideable(_)).Times(0);
  EXPECT_CALL(mock_hider_, HideBlockedElement(_, _)).Times(0);
  Filter filter(
      std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_EXCEPTION));
  EXPECT_CALL(
      *mock_engine_,
      Matches(url.spec(),
              AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_STYLESHEET,
              referrer.spec(), "key", true))
      .WillOnce(Return(filter));
  classifier_->CheckFilterMatch(
      url, static_cast<int>(blink::mojom::ResourceType::kStylesheet),
      main_rfh()->GetProcess()->GetID(), main_rfh()->GetRoutingID(),
      base::BindLambdaForTesting([&loop](mojom::FilterMatchResult result) {
        EXPECT_EQ(mojom::FilterMatchResult::kAllowRule, result);
        loop.QuitWhenIdle();
      }));
  loop.Run();
}

TEST_F(AdblockRequestClassifierLibabpTest, CheckFilterMatchResourceAllowed) {
  DoFilterMatchTest(Filter(std::make_unique<MockFilterImpl>(
                        "", Filter::Type::TYPE_EXCEPTION)),
                    false, true, mojom::FilterMatchResult::kAllowRule);
}

TEST_F(AdblockRequestClassifierLibabpTest, CheckFilterMatchResourceBlocked) {
  DoFilterMatchTest(
      Filter(std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_BLOCKING)),
      false, false, mojom::FilterMatchResult::kBlockRule);
}

TEST_F(AdblockRequestClassifierLibabpTest,
       LoadAllowedWhenNoRenderFrameHostFound) {
  base::RunLoop loop;
  GURL referrer = GURL{kTestReferrer};
  GURL url = GURL{kTestURL};
  EXPECT_CALL(mock_hider_, IsElementTypeHideable(_)).Times(0);
  EXPECT_CALL(mock_hider_, HideBlockedElement(_, _)).Times(0);
  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(network::mojom::kBrowserProcessId, 1))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(_)).Times(0);
  classifier_->CheckFilterMatch(
      url, static_cast<int>(blink::mojom::ResourceType::kStylesheet),
      network::mojom::kBrowserProcessId, 1,
      base::BindLambdaForTesting([&loop](mojom::FilterMatchResult result) {
        // A false result indicates "do not block".
        EXPECT_EQ(mojom::FilterMatchResult::kNoRule, result);
        loop.QuitWhenIdle();
      }));
  loop.Run();
}

TEST_F(AdblockRequestClassifierLibabpTest, CheckFilterForWebSocket) {
  base::RunLoop loop;
  GURL url = GURL{"wss://echo.websocket.org"};
  GURL referrer = GURL{kTestReferrer};
  std::vector<GURL> frame_hierarchy{referrer};

  GURL test_subscription_url = GURL{kTestURL};
  std::vector<GURL> subscriptions_urls{test_subscription_url};
  std::unique_ptr<FakeSubscriptionImpl> mock_subscription =
      std::make_unique<FakeSubscriptionImpl>(test_subscription_url, "title",
                                             std::vector<std::string>{"pl"},
                                             kSynchronizationOkStatus, false);
  AdblockPlus::Subscription subscription(std::move(mock_subscription));
  std::vector<AdblockPlus::Subscription> subscriptions{subscription};
  EXPECT_CALL(
      mock_observer_,
      OnAdMatched(url, mojom::FilterMatchResult::kBlockRule, frame_hierarchy,
                  CONTENT_TYPE_WEBSOCKET, main_rfh(), subscriptions_urls));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(frame_hierarchy));
  ExpectSiteKeyFoundForFrameHierarchy(frame_hierarchy, SiteKey("key"));
  EXPECT_CALL(*mock_engine_,
              IsContentAllowlisted(
                  url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT,
                  std::vector<std::string>{referrer.spec()}, "key"))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *mock_engine_,
      IsContentAllowlisted(
          url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
          std::vector<std::string>{referrer.spec()}, "key"))
      .WillOnce(Return(false));
  Filter filter(
      std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_BLOCKING));
  EXPECT_CALL(*mock_engine_, GetSubscriptionsFromFilter(filter))
      .WillOnce(Return(subscriptions));
  EXPECT_CALL(
      *mock_engine_,
      Matches(url.spec(),
              AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_WEBSOCKET,
              referrer.spec(), "key", false))
      .WillOnce(Return(filter));

  EXPECT_CALL(mock_hider_, IsElementTypeHideable(CONTENT_TYPE_WEBSOCKET))
      .WillOnce(Return(false));
  EXPECT_CALL(mock_hider_, HideBlockedElement(_, _)).Times(0);
  classifier_->CheckFilterMatchForWebSocket(
      url, main_rfh(),
      base::BindLambdaForTesting([&loop](mojom::FilterMatchResult result) {
        EXPECT_EQ(mojom::FilterMatchResult::kBlockRule, result);
        loop.QuitWhenIdle();
      }));
  loop.Run();
}

TEST_F(AdblockRequestClassifierLibabpTest,
       NotificationAfterRenderFrameHostDestroyed) {
  base::RunLoop loop;
  GURL referrer = GURL{kTestReferrer};
  GURL url = GURL{kTestURL};
  std::vector<GURL> frame_hierarchy{referrer};

  // Simulate an allowing match:
  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetProcess()->GetID(),
                                  main_rfh()->GetRoutingID()))
      .WillOnce(Return(main_rfh()));
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
      .WillOnce(Return(frame_hierarchy));
  EXPECT_CALL(
      *mock_engine_,
      IsContentAllowlisted(
          url.spec(), AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
          std::vector<std::string>{referrer.spec()}, ""))
      .WillOnce(Return(true));
  Filter filter(
      std::make_unique<MockFilterImpl>("", Filter::Type::TYPE_EXCEPTION));
  EXPECT_CALL(
      *mock_engine_,
      Matches(url.spec(),
              AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_STYLESHEET,
              referrer.spec(), "", true))
      .WillOnce(Return(filter));

  // CheckFilterMatch called while main_rfh() still alive.
  classifier_->CheckFilterMatch(
      url, static_cast<int>(blink::mojom::ResourceType::kStylesheet),
      main_rfh()->GetProcess()->GetID(), main_rfh()->GetRoutingID(),
      base::BindLambdaForTesting([&loop](mojom::FilterMatchResult result) {
        EXPECT_EQ(mojom::FilterMatchResult::kAllowRule, result);
        loop.QuitWhenIdle();
      }));

  // "Tab is closed" before |bridge_| has time to process the check.
  DeleteContents();

  // The observer gets called with a null RenderFrameHost.
  EXPECT_CALL(
      mock_observer_,
      OnAdMatched(url, mojom::FilterMatchResult::kAllowRule, frame_hierarchy,
                  CONTENT_TYPE_STYLESHEET, nullptr, _));

  // Let |bridge_| process the check for a dead RenderFrameHost.
  loop.Run();
}

TEST_F(AdblockRequestClassifierLibabpTest,
       FilterEngineCreatedAfterRenderFrameHostDestroyed) {
  base::RunLoop loop;
  GURL referrer = GURL{kTestReferrer};
  GURL url = GURL{kTestURL};

  // Simulate a not-yet-initialized state by removing FilterEngine.
  auto filter_engine = embedder_->TakeFilterEngine();

  // Bridge will find the RenderFrameHost :
  EXPECT_CALL(*mock_frame_hierarchy_builder_,
              FindRenderFrameHost(main_rfh()->GetProcess()->GetID(),
                                  main_rfh()->GetRoutingID()))
      .WillOnce(Return(main_rfh()));

  // But won't progress, because the embedder is not initialized.

  // The RenderFrameHost is destroyed before FilterEngine becomes available,
  // so we don't expect any calls that would require a RFH.
  EXPECT_CALL(*mock_frame_hierarchy_builder_, BuildFrameHierarchy(_)).Times(0);
  EXPECT_CALL(*mock_engine_, IsContentAllowlisted(_, _, _, _)).Times(0);
  EXPECT_CALL(*mock_engine_, Matches(_, _, _, _, _)).Times(0);

  // CheckFilterMatch called while main_rfh() still alive. Task gets deferred
  // because FilterEngine unavailable.
  classifier_->CheckFilterMatch(
      url, static_cast<int>(blink::mojom::ResourceType::kStylesheet),
      main_rfh()->GetProcess()->GetID(), main_rfh()->GetRoutingID(),
      base::BindLambdaForTesting([&loop](mojom::FilterMatchResult result) {
        // Bridge will bail out with kNoRule result.
        EXPECT_EQ(mojom::FilterMatchResult::kNoRule, result);
        loop.QuitWhenIdle();
      }));

  // "Tab is closed" before FilterEngine becomes available.
  DeleteContents();

  embedder_->SetFilterEngine(std::move(filter_engine));
  // Bridge will now execute deferred CheckFilterMatch:
  embedder_->NotifyOnEngineCreated();
  loop.Run();
}

}  // namespace adblock

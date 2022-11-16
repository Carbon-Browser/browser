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

#include "components/adblock/content/browser/adblock_webcontents_observer.h"

#include "components/adblock/content/browser/test/mock_element_hider.h"
#include "components/adblock/content/browser/test/mock_frame_hierarchy_builder.h"
#include "components/adblock/core/subscription/test/mock_subscription_updater.h"
#include "components/adblock/core/test/mock_adblock_controller.h"
#include "components/adblock/core/test/mock_sitekey_storage.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"

namespace adblock {

class AdblockWebContentObserverTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    auto* web_contents = this->web_contents();
    AdblockWebContentObserver::CreateForWebContents(
        web_contents, &controller_, &hider_, &storage_,
        std::make_unique<MockFrameHierarchyBuilder>());
    observer_ = AdblockWebContentObserver::FromWebContents(web_contents);
  }

  MockAdblockController controller_;
  MockElementHider hider_;
  MockSitekeyStorage storage_;
  AdblockWebContentObserver* observer_;
};

TEST_F(AdblockWebContentObserverTest, DidFinishNavigationNotCommited) {
  content::MockNavigationHandle mock_navigation_handle;
  mock_navigation_handle.set_has_committed(false);

  EXPECT_CALL(hider_, HideBlockedElement(testing::_, testing::_)).Times(0);
  EXPECT_CALL(hider_,
              ApplyElementHidingEmulationOnPage(
                  testing::_, testing::_, testing::_, testing::_, testing::_))
      .Times(0);

  observer_->DidFinishNavigation(&mock_navigation_handle);
}

TEST_F(AdblockWebContentObserverTest, DidFinishNavigationWithNiceUrl) {
  content::RenderFrameHost* frame_host = main_rfh();
  content::MockNavigationHandle mock_navigation_handle;
  GURL url("https://test.com");
  mock_navigation_handle.set_has_committed(true);
  mock_navigation_handle.set_url(url);
  mock_navigation_handle.set_render_frame_host(frame_host);

  EXPECT_CALL(controller_, IsAdblockEnabled()).WillOnce(testing::Return(true));
  EXPECT_CALL(storage_, FindSiteKeyForAnyUrl(std::vector<GURL>{}))
      .WillOnce(testing::Return(absl::nullopt));
  EXPECT_CALL(hider_, HideBlockedElement(testing::_, testing::_)).Times(0);
  EXPECT_CALL(hider_, ApplyElementHidingEmulationOnPage(
                          GURL{}, std::vector<GURL>{}, frame_host, testing::_,
                          testing::_))
      .Times(1);

  observer_->DidFinishNavigation(&mock_navigation_handle);
}

TEST_F(AdblockWebContentObserverTest, DidFinishNavigationWithErrorPage) {
  content::MockNavigationHandle mock_navigation_handle;
  mock_navigation_handle.set_has_committed(true);
  mock_navigation_handle.set_is_error_page(true);

  EXPECT_CALL(hider_, HideBlockedElement(testing::_, testing::_)).Times(0);
  EXPECT_CALL(hider_,
              ApplyElementHidingEmulationOnPage(
                  testing::_, testing::_, testing::_, testing::_, testing::_))
      .Times(0);

  observer_->DidFinishNavigation(&mock_navigation_handle);
}

TEST_F(AdblockWebContentObserverTest, DidFinishNavigationWithAboutBlank) {
  content::MockNavigationHandle mock_navigation_handle;
  mock_navigation_handle.set_has_committed(true);
  mock_navigation_handle.set_url(GURL("about:blank"));

  EXPECT_CALL(hider_, HideBlockedElement(testing::_, testing::_)).Times(0);
  EXPECT_CALL(hider_,
              ApplyElementHidingEmulationOnPage(
                  testing::_, testing::_, testing::_, testing::_, testing::_))
      .Times(0);

  observer_->DidFinishNavigation(&mock_navigation_handle);
}

TEST_F(AdblockWebContentObserverTest, DidFinishNavigationWithBlockedFrame) {
  GURL url("https://test.com/frame");
  GURL parent("https://test.com");
  content::RenderFrameHost* frame_host = main_rfh();
  NavigateAndCommit(parent);

  content::RenderFrameHost* child_frame =
      content::RenderFrameHostTester::For(frame_host)->AppendChild("");
  ASSERT_TRUE(child_frame != nullptr);
  content::MockNavigationHandle mock_navigation_handle;
  mock_navigation_handle.set_has_committed(true);
  mock_navigation_handle.set_is_error_page(true);
  mock_navigation_handle.set_net_error_code(net::ERR_BLOCKED_BY_ADMINISTRATOR);
  mock_navigation_handle.set_render_frame_host(child_frame);
  mock_navigation_handle.set_url(url);

  EXPECT_CALL(hider_, HideBlockedElement(url, frame_host)).Times(1);
  EXPECT_CALL(hider_,
              ApplyElementHidingEmulationOnPage(
                  testing::_, testing::_, testing::_, testing::_, testing::_))
      .Times(0);

  observer_->DidFinishNavigation(&mock_navigation_handle);
}

}  // namespace adblock

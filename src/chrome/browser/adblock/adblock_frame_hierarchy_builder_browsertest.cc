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

#include "components/adblock/content/browser/frame_hierarchy_builder.h"

#include "chrome/browser/adblock/subscription_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

// Simulated setup:
// http://outer.com/outermost_frame.html
//   has an iframe: http://middle.com/middle_frame.html
//     has an iframe: http://inner.com/innermost_frame.html
//       has a subresource http://inner.com/resource.png
//
// All of these files are in chrome/test/data/adblock. Cross-domain distribution
// is simulated via SetupCrossSiteRedirector.
// innermost_frame.html reports whether resource.png is visible via
// window.top.postMessage to outermost_frame.html, which stores a global
// subresource_visible JS variable.
class AdblockFrameHierarchyBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/adblock");
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetFilters(std::vector<std::string> filters) {
    auto* subscription_service =
        SubscriptionServiceFactory::GetForBrowserContext(browser()->profile());
    subscription_service->SetCustomFilters(std::move(filters));
  }

  void NavigateToOutermostFrame() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(
                       "/cross-site/outer.com/outermost_frame.html")));
  }

  void VerifyTargetResourceShown(bool expected_shown) {
    EXPECT_EQ(
        expected_shown,
        content::EvalJs(browser()->tab_strip_model()->GetActiveWebContents(),
                        "subresource_visible === true"));
  }
};

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest,
                       SubresourceShownWithNoFilters) {
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(true);
}

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest, SubresourceBlocked) {
  SetFilters({"/resource.png"});
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(false);
}

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaInnerFrame) {
  SetFilters({"/resource.png", "@@||inner.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(true);
}

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaMiddleFrame) {
  SetFilters({"/resource.png", "@@||middle.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(true);
}

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest,
                       SubresourceAllowedViaOutermostFrame) {
  SetFilters({"/resource.png", "@@||outer.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(true);
}

IN_PROC_BROWSER_TEST_F(AdblockFrameHierarchyBrowserTest,
                       SubresourceBlockedWhenInvalidAllowRule) {
  SetFilters({"/resource.png", "@@||bogus.com^$document"});
  NavigateToOutermostFrame();
  VerifyTargetResourceShown(false);
}

// More tests can be added / parametrized, e.g.:
// - elemhide blocking filters (in conjunction with $elemhide allow rules)
// - $subdocument-based allow rules

}  // namespace adblock

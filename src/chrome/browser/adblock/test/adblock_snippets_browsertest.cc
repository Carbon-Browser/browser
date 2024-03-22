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

#include <vector>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockSnippetsBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/adblock");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetFilters(std::vector<std::string> filters) {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser()->profile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    adblock_configuration->RemoveCustomFilter(kAllowlistEverythingFilter);
    for (auto& filter : filters) {
      adblock_configuration->AddCustomFilter(filter);
    }
  }

  GURL GetUrl(const std::string& path) {
    return embedded_test_server()->GetURL("example.org", path);
  }

  void VerifyTargetVisibility(bool is_hidden, const std::string& id) {
    std::string is_invisible_js =
        "getComputedStyle(document.getElementById('{{node id}}')).display == "
        "'none'";
    base::ReplaceSubstringsAfterOffset(&is_invisible_js, 0, "{{node id}}", id);
    EXPECT_EQ(
        is_hidden,
        content::EvalJs(browser()->tab_strip_model()->GetActiveWebContents(),
                        is_invisible_js));
  }
};

IN_PROC_BROWSER_TEST_F(AdblockSnippetsBrowserTest, VerifyXpath3) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GetUrl("/xpath3.html")));
  VerifyTargetVisibility(false, "xpath3-target");
  SetFilters(
      {"example.org#$#hide-if-matches-xpath3 //*[@id=\"xpath3-target\"]"});
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GetUrl("/xpath3.html")));
  VerifyTargetVisibility(true, "xpath3-target");
}

}  // namespace adblock

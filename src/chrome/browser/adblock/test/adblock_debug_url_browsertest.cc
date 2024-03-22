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

#include <string>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/adblock_url_loader_factory_for_test.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::HasSubstr;
using testing::Mock;
using testing::Return;
using testing::StartsWith;

namespace adblock {

class AdblockDebugUrlTest : public InProcessBrowserTest {
 public:
  AdblockDebugUrlTest() {}
  ~AdblockDebugUrlTest() override = default;
  AdblockDebugUrlTest(const AdblockDebugUrlTest&) = delete;
  AdblockDebugUrlTest& operator=(const AdblockDebugUrlTest&) = delete;

 protected:
  std::string ExecuteScriptAndExtractString(const std::string& js_code) const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    return content::EvalJs(web_contents->GetPrimaryMainFrame(), js_code)
        .ExtractString();
  }

  bool IsAdblockEnabled() {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser()->profile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    DCHECK(adblock_configuration) << "Test expects \"adblock\" configuration";
    return adblock_configuration->IsEnabled();
  }

  bool IsAAEnabled() {
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(browser()->profile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    DCHECK(adblock_configuration) << "Test expects \"adblock\" configuration";
    return base::ranges::any_of(
        adblock_configuration->GetFilterLists(),
        [&](const auto& url) { return url == AcceptableAdsUrl(); });
  }

  const std::string kReadPageBodyScript =
      "document.getElementsByTagName('body')[0].firstChild.innerHTML";

  const std::string kAdblockDebugUrl =
      "http://" +
      adblock::AdblockURLLoaderFactoryForTest::kAdblockDebugDataHostName;
};

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestInvalidUrls) {
  GURL no_command1(kAdblockDebugUrl);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), no_command1));
  ASSERT_EQ("INVALID_COMMAND",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL no_command2(kAdblockDebugUrl + "/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), no_command2));
  ASSERT_EQ("INVALID_COMMAND",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL invalid_command_url(kAdblockDebugUrl + "/some_invalid_command");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), invalid_command_url));
  ASSERT_EQ("INVALID_COMMAND",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL invalid_topic(kAdblockDebugUrl + "/filter/add/%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), invalid_topic));
  ASSERT_EQ("INVALID_COMMAND",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL invalid_command(kAdblockDebugUrl + "/filters/ad/%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), invalid_command));
  ASSERT_EQ("INVALID_COMMAND",
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestFilterCommands) {
  GURL clear_filters_url(kAdblockDebugUrl + "/filters/clear");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_filters_url(kAdblockDebugUrl + "/filters/list");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  std::string expected_no_filters = "OK";
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_filters_url(kAdblockDebugUrl +
                       "/filters/add/%2FadsPlugin%2F%2A%0A%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), add_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("adsPlugin/*"));
  ASSERT_THAT(response, HasSubstr("adsponsor."));

  GURL remove_filter_url(kAdblockDebugUrl + "/filters/remove/%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), remove_filter_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  std::string expected_one_filter = "OK\n\n/adsPlugin/*\n";
  ASSERT_EQ(expected_one_filter,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestDomainCommands) {
  GURL clear_domains_url(kAdblockDebugUrl + "/domains/clear");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_domains_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_domains_url(kAdblockDebugUrl + "/domains/list");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_domains_url));
  std::string expected_no_domains = "OK";
  ASSERT_EQ(expected_no_domains,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_domain_url(kAdblockDebugUrl +
                      "/domains/add/example.com%0Adomain.org");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), add_domain_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_domains_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("example.com"));
  ASSERT_THAT(response, HasSubstr("domain.org"));

  GURL remove_domain_url(kAdblockDebugUrl + "/domains/remove/example.com");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), remove_domain_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_domains_url));
  std::string expected_one_domain = "OK\n\ndomain.org\n";
  ASSERT_EQ(expected_one_domain,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_domains_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_domains_url));
  ASSERT_EQ(expected_no_domains,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestSubscriptionCommands) {
  GURL clear_subscriptions_url(kAdblockDebugUrl + "/subscriptions/clear");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_subscriptions_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_subscriptions_url(kAdblockDebugUrl + "/subscriptions/list");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_subscriptions_url));
  std::string expected_no_subscriptions = "OK";
  ASSERT_EQ(expected_no_subscriptions,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_subscription_url(kAdblockDebugUrl +
                            "/subscriptions/add/"
                            "https%3A%2F%2Fexample.com%2Flist1.txt%0Ahttps%3A%"
                            "2F%2Fwww.domain.org%2Flist2.txt");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), add_subscription_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_subscriptions_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("https://example.com/list1.txt"));
  ASSERT_THAT(response, HasSubstr("https://www.domain.org/list2.txt"));

  GURL remove_subscription_url(
      kAdblockDebugUrl +
      "/subscriptions/remove/https%3A%2F%2Fwww.domain.org%2Flist2.txt");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), remove_subscription_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_subscriptions_url));
  std::string expected_one_subscription =
      "OK\n\nhttps://example.com/list1.txt\n";
  ASSERT_EQ(expected_one_subscription,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_subscriptions_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_subscriptions_url));
  ASSERT_EQ(expected_no_subscriptions,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestEnableAdblockCommands) {
  GURL enable_adblock__url(kAdblockDebugUrl + "/adblock/enable");
  GURL disable_adblock_url(kAdblockDebugUrl + "/adblock/disable");
  GURL adblock_state_url(kAdblockDebugUrl + "/adblock/state");

  ASSERT_TRUE(IsAdblockEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), adblock_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), disable_adblock_url));
  ASSERT_FALSE(IsAdblockEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), adblock_state_url));
  ASSERT_EQ("OK\n\ndisabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), enable_adblock__url));
  ASSERT_TRUE(IsAdblockEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), adblock_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestEnableAACommands) {
  GURL enable_aa_url(kAdblockDebugUrl + "/aa/enable");
  GURL disable_aa_url(kAdblockDebugUrl + "/aa/disable");
  GURL aa_state_url(kAdblockDebugUrl + "/aa/state");

  ASSERT_TRUE(IsAAEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), aa_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), disable_aa_url));
  ASSERT_FALSE(IsAAEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), aa_state_url));
  ASSERT_EQ("OK\n\ndisabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), enable_aa_url));
  ASSERT_TRUE(IsAAEnabled());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), aa_state_url));
  ASSERT_EQ("OK\n\nenabled",
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

IN_PROC_BROWSER_TEST_F(AdblockDebugUrlTest, TestFilterCommandsInOldFormat) {
  GURL clear_filters_url(kAdblockDebugUrl + "/filters/clear");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL list_filters_url(kAdblockDebugUrl + "/filters/list");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  std::string expected_no_filters = "OK";
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  GURL add_filters_url(kAdblockDebugUrl +
                       "/add?payload=%2FadsPlugin%2F%2A%0A%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), add_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  auto response = ExecuteScriptAndExtractString(kReadPageBodyScript);
  ASSERT_THAT(response, StartsWith("OK\n\n"));
  ASSERT_THAT(response, HasSubstr("adsPlugin/*"));
  ASSERT_THAT(response, HasSubstr("adsponsor."));

  GURL remove_filter_url(kAdblockDebugUrl + "/remove?payload=%2Fadsponsor.");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), remove_filter_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  std::string expected_one_filter = "OK\n\n/adsPlugin/*\n";
  ASSERT_EQ(expected_one_filter,
            ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), clear_filters_url));
  ASSERT_EQ("OK", ExecuteScriptAndExtractString(kReadPageBodyScript));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), list_filters_url));
  ASSERT_EQ(expected_no_filters,
            ExecuteScriptAndExtractString(kReadPageBodyScript));
}

}  // namespace adblock

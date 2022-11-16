// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

#include <map>
#include <memory>
#include <string>

#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/extensions/api/adblock_private.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/core/adblock_controller.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/background_script_executor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Mock;
using testing::Return;

namespace extensions {

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleSubscriptionUpdateRequest(
    const std::string subcription_path,
    const std::string subcription_filters,
    const net::test_server::HttpRequest& request) {
  static const char kSubscriptionHeader[] =
      "[Adblock Plus 2.0]\n"
      "! Checksum: X5A8vtJDBW2a9EgS9glqbg\n"
      "! Version: 202202061935\n"
      "! Last modified: 06 Feb 2022 19:35 UTC\n"
      "! Expires: 1 days (update frequency)\n\n";

  if (base::StartsWith(request.relative_url, subcription_path,
                       base::CompareCase::SENSITIVE)) {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content(kSubscriptionHeader + subcription_filters);
    http_response->set_content_type("text/plain");
    return std::move(http_response);
  }

  // Unhandled requests result in the Embedded test server sending a 404.
  return nullptr;
}

class AdblockPrivateApiTest : public ExtensionApiTest {
 public:
  AdblockPrivateApiTest() {}
  ~AdblockPrivateApiTest() override = default;
  AdblockPrivateApiTest(const AdblockPrivateApiTest&) = delete;
  AdblockPrivateApiTest& operator=(const AdblockPrivateApiTest&) = delete;

 protected:
  void SetUp() override {
    ExtensionApiTest::SetUp();
    // When any of that fails we need to update comment in adblock_private.idl
    ASSERT_EQ(api::tabs::TAB_ID_NONE, -1);
    ASSERT_EQ(SessionID::InvalidValue().id(), -1);
  }

  bool RunTest(const std::string& subtest) {
    const std::string page_url = "main.html?" + subtest;
    return RunExtensionTest("adblock_private", {.page_url = page_url.c_str()},
                            {.load_as_component = true});
  }

  bool RunTestWithParams(const std::string& subtest,
                         const std::map<std::string, std::string>& params) {
    if (params.empty()) {
      return RunTest(subtest);
    }
    std::string subtest_with_params = subtest;
    for (const auto& [key, value] : params) {
      subtest_with_params += "&" + key + "=" + value;
    }
    return RunTest(subtest_with_params);
  }

  bool RunTestWithServer(const std::string& subtest,
                         const std::string& subcription_path,
                         const std::string& subcription_filters) {
    net::EmbeddedTestServer https_server{
        net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS)};
    https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server.RegisterRequestHandler(
        base::BindRepeating(&HandleSubscriptionUpdateRequest, subcription_path,
                            subcription_filters));
    if (!https_server.Start()) {
      return false;
    }
    const std::string subtest_with_params =
        subtest +
        "&subscription_url=" + https_server.GetURL(subcription_path).spec();
    return RunTest(subtest_with_params);
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, UpdateConsent) {
  EXPECT_TRUE(RunTest("updateConsent")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, SetAndCheckEnabled) {
  EXPECT_TRUE(RunTest("setEnabled_isEnabled")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, SetAndCheckAAEnabled) {
  EXPECT_TRUE(RunTest("setAAEnabled_isAAEnabled")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, GetBuiltInSubscriptions) {
  EXPECT_TRUE(RunTest("getBuiltInSubscriptions")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, SelectedSubscriptionsDataSchema) {
  EXPECT_TRUE(RunTest("selectedSubscriptionsDataSchema")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest,
                       SelectBuiltInSubscriptionsInvalidURL) {
  EXPECT_TRUE(RunTest("selectBuiltInSubscriptionsInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest,
                       SelectBuiltInSubscriptionsNotBuiltIn) {
  EXPECT_TRUE(RunTest("selectBuiltInSubscriptionsNotBuiltIn")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest,
                       UnselectBuiltInSubscriptionsInvalidURL) {
  EXPECT_TRUE(RunTest("unselectBuiltInSubscriptionsInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest,
                       UnselectBuiltInSubscriptionsNotBuiltIn) {
  EXPECT_TRUE(RunTest("unselectBuiltInSubscriptionsNotBuiltIn")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, BuiltInSubscriptionsManagement) {
  DCHECK(browser()->profile());
  auto* controller = adblock::AdblockControllerFactory::GetForBrowserContext(
      browser()->profile());
  auto selected = controller->GetSelectedBuiltInSubscriptions();
  const auto easylist = std::find_if(
      selected.begin(), selected.end(),
      [&](scoped_refptr<adblock::Subscription> subscription) {
        return base::EndsWith(subscription->GetSourceUrl().path_piece(),
                              "easylist.txt");
      });
  const auto exceptions = std::find_if(
      selected.begin(), selected.end(),
      [&](scoped_refptr<adblock::Subscription> subscription) {
        return base::EndsWith(subscription->GetSourceUrl().path_piece(),
                              "exceptionrules.txt");
      });
  const auto snippets = std::find_if(
      selected.begin(), selected.end(),
      [&](scoped_refptr<adblock::Subscription> subscription) {
        return base::EndsWith(subscription->GetSourceUrl().path_piece(),
                              "abp-filters-anti-cv.txt");
      });
  if (easylist == selected.end() || exceptions == selected.end() ||
      snippets == selected.end()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  const std::map<std::string, std::string> params = {
      {"easylist", (*easylist)->GetSourceUrl().spec()},
      {"exceptions", (*exceptions)->GetSourceUrl().spec()},
      {"snippets", (*snippets)->GetSourceUrl().spec()}};
  EXPECT_TRUE(RunTestWithParams("builtInSubscriptionsManagement", params))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, CustomSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("addCustomSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, RemoveSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("removeCustomSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, CustomSubscriptionsManagement) {
  EXPECT_TRUE(RunTest("customSubscriptionsManagement")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, AllowedDomainsManagement) {
  EXPECT_TRUE(RunTest("allowedDomainsManagement")) << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, AdBlockedEvents) {
  std::string subcription_path = "/testeventssub.txt";
  std::string subcription_filters = "test1.png";

  EXPECT_TRUE(RunTestWithServer("adBlockedEvents", subcription_path,
                                subcription_filters))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, AdAllowedEvents) {
  std::string subcription_path = "/testeventssub.txt";
  std::string subcription_filters = "test2.png\n@@test2.png";

  EXPECT_TRUE(RunTestWithServer("adAllowedEvents", subcription_path,
                                subcription_filters))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, SessionStats) {
  std::string subcription_path = "/teststatssub.txt";
  std::string subcription_filters = "test3.png\ntest4.png\n@@test4.png";

  EXPECT_TRUE(
      RunTestWithServer("sessionStats", subcription_path, subcription_filters))
      << message_;
}

class AdblockPrivateApiBackgroundPageTest : public ExtensionApiTest {
 public:
  AdblockPrivateApiBackgroundPageTest() {}
  ~AdblockPrivateApiBackgroundPageTest() override = default;
  AdblockPrivateApiBackgroundPageTest(
      const AdblockPrivateApiBackgroundPageTest&) = delete;
  AdblockPrivateApiBackgroundPageTest& operator=(
      const AdblockPrivateApiBackgroundPageTest&) = delete;

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    // Map example.com to localhost.
    host_resolver()->AddRule("example.com", "127.0.0.1");
    ASSERT_TRUE(StartEmbeddedTestServer());
  }

  void ExecuteJavascript(const std::string& js_code) const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(
        content::ExecuteScript(web_contents->GetPrimaryMainFrame(), js_code));
  }
};

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiBackgroundPageTest, PageAllowedEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"));
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.pageAllowedCount = 0;
    chrome.adblockPrivate.onPageAllowed.addListener(function(e) {
      if (!e.url.endsWith('test.html')) {
        return;
      }
      testData.pageAllowedCount = testData.pageAllowedCount + 1;
    });
    window.domAutomationController.send('');
  )";

  constexpr char kReadCountersScript[] = R"(
    window.domAutomationController.send(testData.pageAllowedCount.toString());
  )";

  constexpr char kAllowDomainScript[] = R"(
    chrome.adblockPrivate.addAllowedDomain('example.com');
    window.domAutomationController.send('');
  )";

  GURL test_url = embedded_test_server()->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(
      "0", ExecuteScriptInBackgroundPage(extension->id(), kReadCountersScript));

  ExecuteScriptInBackgroundPage(extension->id(), kAllowDomainScript);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(
      "1", ExecuteScriptInBackgroundPage(extension->id(), kReadCountersScript));
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiBackgroundPageTest, PageAllowedStats) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"));
  ASSERT_TRUE(extension);

  constexpr char kReadAllowedStatsScript[] = R"(
    chrome.adblockPrivate.getSessionAllowedAdsCount(function(sessionStats) {
      let count = 0;
      for (const entry of sessionStats) {
        if (entry.url === 'adblock:custom') {
          count = entry.count;
        }
      }
      window.domAutomationController.send(count.toString());
    });
  )";

  constexpr char kAllowDomainScript[] = R"(
    chrome.adblockPrivate.addAllowedDomain('example.com');
    window.domAutomationController.send('');
  )";

  GURL test_url = embedded_test_server()->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");
  int initial_value = std::stoi(
      ExecuteScriptInBackgroundPage(extension->id(), kReadAllowedStatsScript));

  ExecuteScriptInBackgroundPage(extension->id(), kAllowDomainScript);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  ASSERT_EQ(initial_value + 1, std::stoi(ExecuteScriptInBackgroundPage(
                                   extension->id(), kReadAllowedStatsScript)));
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiBackgroundPageTest, PopupEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"));
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.popupBlockedCount = 0;
    testData.popupAllowedCount = 0;
    chrome.adblockPrivate.onPopupAllowed.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupAllowedCount = testData.popupAllowedCount + 1;
    });
    chrome.adblockPrivate.onPopupBlocked.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupBlockedCount = testData.popupBlockedCount + 1;
    });
    window.domAutomationController.send('');
  )";

  constexpr char kReadCountersScript[] = R"(
    window.domAutomationController.send(testData.popupBlockedCount +
      '-' + testData.popupAllowedCount);
  )";

  constexpr char kBlockPopupScript[] = R"(
    chrome.adblockPrivate.addCustomFilter('some-popup.html^$popup');
    window.domAutomationController.send('');
  )";

  constexpr char kAllowPopupScript[] = R"(
    chrome.adblockPrivate.addCustomFilter('@@some-popup.html^$popup');
    window.domAutomationController.send('');
  )";

  GURL test_url = embedded_test_server()->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);
  ASSERT_EQ("0-0", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  ExecuteScriptInBackgroundPage(extension->id(), kBlockPopupScript);
  ExecuteJavascript("window.open('some-popup.html');");
  ASSERT_EQ("1-0", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  ExecuteScriptInBackgroundPage(extension->id(), kAllowPopupScript);
  ExecuteJavascript("window.open('some-popup.html');");
  ASSERT_EQ("1-1", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));
}

IN_PROC_BROWSER_TEST_F(AdblockPrivateApiBackgroundPageTest, PopupStats) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"));
  ASSERT_TRUE(extension);

  constexpr char kReadAllowedStatsScript[] = R"(
    chrome.adblockPrivate.getSessionAllowedAdsCount(function(sessionStats) {
      let count = 0;
      for (const entry of sessionStats) {
        if (entry.url === 'adblock:custom') {
          count = entry.count;
        }
      }
      window.domAutomationController.send(count.toString());
    });
  )";

  constexpr char kReadBlockedStatsScript[] = R"(
    chrome.adblockPrivate.getSessionBlockedAdsCount(function(sessionStats) {
      let count = 0;
      for (const entry of sessionStats) {
        if (entry.url === 'adblock:custom') {
          count = entry.count;
        }
      }
      window.domAutomationController.send(count.toString());
    });
  )";

  constexpr char kBlockPopupScript[] = R"(
    chrome.adblockPrivate.addCustomFilter('some-popup.html^$popup');
    window.domAutomationController.send('');
  )";

  constexpr char kAllowPopupScript[] = R"(
    chrome.adblockPrivate.addCustomFilter('@@some-popup.html^$popup');
    window.domAutomationController.send('');
  )";

  GURL test_url = embedded_test_server()->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  int initial_allowed_value = std::stoi(
      ExecuteScriptInBackgroundPage(extension->id(), kReadAllowedStatsScript));
  int initial_blocked_value = std::stoi(
      ExecuteScriptInBackgroundPage(extension->id(), kReadBlockedStatsScript));

  ExecuteScriptInBackgroundPage(extension->id(), kBlockPopupScript);
  ExecuteJavascript("window.open('some-popup.html');");
  ASSERT_EQ(initial_blocked_value + 1,
            std::stoi(ExecuteScriptInBackgroundPage(extension->id(),
                                                    kReadBlockedStatsScript)));

  ExecuteScriptInBackgroundPage(extension->id(), kAllowPopupScript);
  ExecuteJavascript("window.open('some-popup.html');");
  ASSERT_EQ(initial_allowed_value + 1,
            std::stoi(ExecuteScriptInBackgroundPage(extension->id(),
                                                    kReadAllowedStatsScript)));
}

}  // namespace extensions

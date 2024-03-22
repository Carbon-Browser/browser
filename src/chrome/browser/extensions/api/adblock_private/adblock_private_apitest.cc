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

#include "chrome/browser/adblock/adblock_content_browser_client.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/ssl/https_upgrades_interceptor.h"
#include "chrome/browser/ssl/https_upgrades_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/api/adblock_private.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/background_script_executor.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {
enum class Mode { Normal, Incognito };
enum class EyeoExtensionApi { Old, New };
}  // namespace

class AdblockPrivateApiTest : public ExtensionApiTest,
                              public testing::WithParamInterface<Mode> {
 public:
  AdblockPrivateApiTest() {}
  ~AdblockPrivateApiTest() override = default;
  AdblockPrivateApiTest(const AdblockPrivateApiTest&) = delete;
  AdblockPrivateApiTest& operator=(const AdblockPrivateApiTest&) = delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
    if (IsIncognito()) {
      command_line->AppendSwitch(switches::kIncognito);
    }
  }

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    // When any of that fails we need to update comment in adblock_private.idl
    ASSERT_EQ(api::tabs::TAB_ID_NONE, -1);
    ASSERT_EQ(SessionID::InvalidValue().id(), -1);

    adblock::SubscriptionServiceFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->GetFilteringConfiguration(adblock::kAdblockFilteringConfigurationName)
        ->RemoveCustomFilter(adblock::kAllowlistEverythingFilter);

    AdblockContentBrowserClient::ForceAdblockProxyForTesting();
  }

  bool IsIncognito() { return GetParam() == Mode::Incognito; }

  bool RunTest(const std::string& subtest) {
    const std::string page_url = "main.html?subtest=" + subtest;
    return RunExtensionTest("adblock_private",
                            {.extension_url = page_url.c_str()},
                            {.allow_in_incognito = IsIncognito(),
                             .load_as_component = !IsIncognito()});
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
                         const std::string& subscription_path,
                         const std::string& subscription_filters,
                         std::map<std::string, std::string> params = {{}}) {
    net::EmbeddedTestServer https_server{
        net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS)};
    https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server.RegisterRequestHandler(base::BindRepeating(
        &AdblockPrivateApiTest::HandleSubscriptionUpdateRequest,
        base::Unretained(this), subscription_path, subscription_filters));
    if (!https_server.Start()) {
      return false;
    }
    params.insert(
        {"subscription_url", https_server.GetURL(subscription_path).spec()});
    return RunTestWithParams(subtest, params);
  }

  std::unique_ptr<net::test_server::HttpResponse>
  HandleSubscriptionUpdateRequest(
      const std::string subscription_path,
      const std::string subscription_filters,
      const net::test_server::HttpRequest& request) {
    static const char kSubscriptionHeader[] =
        "[Adblock Plus 2.0]\n"
        "! Checksum: X5A8vtJDBW2a9EgS9glqbg\n"
        "! Version: 202202061935\n"
        "! Last modified: 06 Feb 2022 19:35 UTC\n"
        "! Expires: 1 days (update frequency)\n\n";

    if (base::StartsWith(request.relative_url, subscription_path,
                         base::CompareCase::SENSITIVE)) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(kSubscriptionHeader + subscription_filters);
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

  std::map<std::string, std::string> SubscriptionsManagementSetup() {
    DCHECK(browser()->profile()->GetOriginalProfile());
    auto selected = adblock::SubscriptionServiceFactory::GetForBrowserContext(
                        browser()->profile()->GetOriginalProfile())
                        ->GetFilteringConfiguration(
                            adblock::kAdblockFilteringConfigurationName)
                        ->GetFilterLists();
    const auto easylist = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(), "easylist.txt");
        });
    const auto exceptions = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(),
                                "exceptionrules.txt");
        });
    const auto snippets = std::find_if(
        selected.begin(), selected.end(), [&](const GURL& subscription) {
          return base::EndsWith(subscription.path_piece(),
                                "abp-filters-anti-cv.txt");
        });
    if (easylist == selected.end() || exceptions == selected.end() ||
        snippets == selected.end()) {
      return std::map<std::string, std::string>{};
    }
    return std::map<std::string, std::string>{
        {"easylist", easylist->spec()},
        {"exceptions", exceptions->spec()},
        {"snippets", snippets->spec()}};
  }
};

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckEnabled) {
  EXPECT_TRUE(RunTest("setEnabled_isEnabled")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckEnabledNewAPI) {
  EXPECT_TRUE(RunTestWithParams("setEnabled_isEnabled",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckAAEnabled) {
  EXPECT_TRUE(RunTest("setAAEnabled_isAAEnabled")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SetAndCheckAAEnabledNewAPI) {
  EXPECT_TRUE(RunTest("setAAEnabled_isAAEnabled_newAPI")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, GetBuiltInSubscriptions) {
  EXPECT_TRUE(RunTest("getBuiltInSubscriptions")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       InstalledSubscriptionsDataSchema) {
  EXPECT_TRUE(RunTest("installedSubscriptionsDataSchema")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       InstalledSubscriptionsDataSchemaNewAPI) {
  EXPECT_TRUE(RunTestWithParams("installedSubscriptionsDataSchema",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       InstalledSubscriptionsDataSchemaConfigDisabled) {
  EXPECT_TRUE(RunTestWithParams("installedSubscriptionsDataSchema",
                                {{"disabled", "true"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       InstalledSubscriptionsDataSchemaConfigDisabledNewAPI) {
  EXPECT_TRUE(RunTestWithParams(
      "installedSubscriptionsDataSchema",
      {{"api", "eyeoFilteringPrivate"}, {"disabled", "true"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, InstallSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("installSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       InstallSubscriptionInvalidURLNewAPI) {
  EXPECT_TRUE(RunTestWithParams("installSubscriptionInvalidURL",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, UninstallSubscriptionInvalidURL) {
  EXPECT_TRUE(RunTest("uninstallSubscriptionInvalidURL")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       UninstallSubscriptionInvalidURLNewAPI) {
  EXPECT_TRUE(RunTestWithParams("uninstallSubscriptionInvalidURL",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SubscriptionsManagement) {
  auto params = SubscriptionsManagementSetup();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SubscriptionsManagementNewAPI) {
  auto params = SubscriptionsManagementSetup();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  params.insert({"api", "eyeoFilteringPrivate"});
  SubscriptionsManagementSetup();
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       SubscriptionsManagementConfigDisabled) {
  auto params = SubscriptionsManagementSetup();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  params.insert({"disabled", "true"});
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest,
                       SubscriptionsManagementConfigDisabledNewAPI) {
  auto params = SubscriptionsManagementSetup();
  if (params.empty()) {
    // Since default configuration has been changed let's skip this test
    return;
  }
  params.insert({"api", "eyeoFilteringPrivate"});
  params.insert({"disabled", "true"});
  SubscriptionsManagementSetup();
  EXPECT_TRUE(RunTestWithParams("subscriptionsManagement", params)) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AllowedDomainsManagement) {
  EXPECT_TRUE(RunTest("allowedDomainsManagement")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AllowedDomainsManagementNewAPI) {
  EXPECT_TRUE(RunTestWithParams("allowedDomainsManagement",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, CustomFiltersManagement) {
  EXPECT_TRUE(RunTest("customFiltersManagement")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, CustomFiltersManagementNewAPI) {
  EXPECT_TRUE(RunTestWithParams("customFiltersManagement",
                                {{"api", "eyeoFilteringPrivate"}}))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdBlockedEvents) {
  std::string subscription_path = "/testeventssub.txt";
  std::string subscription_filters = "test1.png";
  EXPECT_TRUE(RunTestWithServer("adBlockedEvents", subscription_path,
                                subscription_filters))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdBlockedEventsNewAPI) {
  std::string subscription_path = "/testeventssub.txt";
  std::string subscription_filters = "test1.png";
  std::map<std::string, std::string> params = {{"api", "eyeoFilteringPrivate"}};
  EXPECT_TRUE(RunTestWithServer("adBlockedEvents", subscription_path,
                                subscription_filters, std::move(params)))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdAllowedEvents) {
  std::string subscription_path = "/testeventssub.txt";
  std::string subscription_filters = "test2.png\n@@test2.png";
  EXPECT_TRUE(RunTestWithServer("adAllowedEvents", subscription_path,
                                subscription_filters))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AdAllowedEventsNewAPI) {
  std::string subscription_path = "/testeventssub.txt";
  std::string subscription_filters = "test2.png\n@@test2.png";
  std::map<std::string, std::string> params = {{"api", "eyeoFilteringPrivate"}};
  EXPECT_TRUE(RunTestWithServer("adAllowedEvents", subscription_path,
                                subscription_filters, std::move(params)))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SessionStats) {
  std::string subscription_path = "/teststatssub.txt";
  std::string subscription_filters = "test3.png\ntest4.png\n@@test4.png";
  EXPECT_TRUE(RunTestWithServer("sessionStats", subscription_path,
                                subscription_filters))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, SessionStatsNewAPI) {
  std::string subscription_path = "/teststatssub.txt";
  std::string subscription_filters = "test3.png\ntest4.png\n@@test4.png";
  std::map<std::string, std::string> params = {{"api", "eyeoFilteringPrivate"}};
  EXPECT_TRUE(RunTestWithServer("sessionStats", subscription_path,
                                subscription_filters, std::move(params)))
      << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, AllowedDomainsEvent) {
  EXPECT_TRUE(RunTest("allowedDomainsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, EnabledStateEvent) {
  EXPECT_TRUE(RunTest("enabledStateEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, FilterListsEvent) {
  EXPECT_TRUE(RunTest("filterListsEvent")) << message_;
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiTest, CustomFiltersEvent) {
  EXPECT_TRUE(RunTest("customFiltersEvent")) << message_;
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockPrivateApiTest,
                         testing::Values(Mode::Normal, Mode::Incognito));

class AdblockPrivateApiBackgroundPageTest
    : public ExtensionApiTest,
      public testing::WithParamInterface<std::tuple<EyeoExtensionApi, Mode>> {
 public:
  AdblockPrivateApiBackgroundPageTest() {}
  ~AdblockPrivateApiBackgroundPageTest() override = default;
  AdblockPrivateApiBackgroundPageTest(
      const AdblockPrivateApiBackgroundPageTest&) = delete;
  AdblockPrivateApiBackgroundPageTest& operator=(
      const AdblockPrivateApiBackgroundPageTest&) = delete;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
    if (IsIncognito()) {
      command_line->AppendSwitch(switches::kIncognito);
    }
  }

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    https_test_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_test_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_test_server_->ServeFilesFromSourceDirectory(
        "chrome/test/data/extensions/api_test/adblock_private");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "chrome/test/data/extensions/api_test/adblock_private");
    HttpsUpgradesInterceptor::SetHttpsPortForTesting(
        https_test_server_->port());
    HttpsUpgradesInterceptor::SetHttpPortForTesting(
        embedded_test_server()->port());
    ASSERT_TRUE(https_test_server_->Start());
    AllowHttpForHostnamesForTesting({"example.com"},
                                    browser()->profile()->GetPrefs());

    // Map example.com to localhost.
    host_resolver()->AddRule("example.com", "127.0.0.1");
    ASSERT_TRUE(StartEmbeddedTestServer());
    adblock::SubscriptionServiceFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->GetFilteringConfiguration(adblock::kAdblockFilteringConfigurationName)
        ->RemoveCustomFilter(adblock::kAllowlistEverythingFilter);
  }

  bool IsOldApi() { return std::get<0>(GetParam()) == EyeoExtensionApi::Old; }

  bool IsIncognito() { return std::get<1>(GetParam()) == Mode::Incognito; }

  void ExecuteScript(const std::string& js_code) const {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetWebContentsAt(0);
    ASSERT_TRUE(content::ExecJs(web_contents->GetPrimaryMainFrame(), js_code));
  }

  std::string ExecuteScriptInBackgroundPage(const std::string& extension_id,
                                            const std::string& script) {
    return ExtensionApiTest::ExecuteScriptInBackgroundPage(extension_id, script)
        .GetString();
  }

  void SetupApiObjectAndMethods(const std::string& extension_id) {
    std::string setup_script;
    if (IsOldApi()) {
      setup_script =
          "var apiObject = chrome.adblockPrivate;"
          "var sessionAllowedCount = 'getSessionAllowedAdsCount';"
          "var sessionBlockedCount = 'getSessionBlockedAdsCount';"
          "var onAllowedEvent = 'onAdAllowed';"
          "var onBlockedEvent = 'onAdBlocked';"
          "chrome.test.sendScriptResult('');";
    } else {
      setup_script =
          "var apiObject = chrome.eyeoFilteringPrivate;"
          "var sessionAllowedCount = 'getSessionAllowedRequestsCount';"
          "var sessionBlockedCount = 'getSessionBlockedRequestsCount';"
          "var onAllowedEvent = 'onRequestAllowed';"
          "var onBlockedEvent = 'onRequestBlocked';"
          "chrome.test.sendScriptResult('');";
    }
    ExecuteScriptInBackgroundPage(extension_id, setup_script);
  }

  std::unique_ptr<net::EmbeddedTestServer> https_test_server_;
};

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PageAllowedEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.pageAllowedCount = 0;
    apiObject.onPageAllowed.addListener(function(e) {
      if (!e.url.endsWith('test.html')) {
        return;
      }
      testData.pageAllowedCount = testData.pageAllowedCount + 1;
    });
    chrome.test.sendScriptResult('');
  )";

  constexpr char kReadCountersScript[] = R"(
    chrome.test.sendScriptResult(testData.pageAllowedCount.toString());
  )";

  constexpr char kAllowDomainScript[] = R"(
    apiObject.addAllowedDomain(%s'example.com');
    chrome.test.sendScriptResult('');
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");

  SetupApiObjectAndMethods(extension->id());

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(
      "0", ExecuteScriptInBackgroundPage(extension->id(), kReadCountersScript));

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kAllowDomainScript, IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ(
      "1", ExecuteScriptInBackgroundPage(extension->id(), kReadCountersScript));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PageAllowedStats) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  constexpr char kReadAllowedStatsScript[] = R"(
    apiObject[sessionAllowedCount](function(sessionStats) {
      let count = 0;
      for (const entry of sessionStats) {
        if (entry.url === 'adblock:custom') {
          count = entry.count;
        }
      }
      chrome.test.sendScriptResult(count.toString());
    });
  )";

  constexpr char kAllowDomainScript[] = R"(
    apiObject.addAllowedDomain(%s'example.com');
    chrome.test.sendScriptResult('');
  )";

  const GURL test_url = https_test_server_->GetURL(
      "example.com", "/extensions/api_test/adblock_private/test.html");

  SetupApiObjectAndMethods(extension->id());

  int initial_value = std::stoi(
      ExecuteScriptInBackgroundPage(extension->id(), kReadAllowedStatsScript));

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kAllowDomainScript, IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  ASSERT_EQ(initial_value + 1, std::stoi(ExecuteScriptInBackgroundPage(
                                   extension->id(), kReadAllowedStatsScript)));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PopupEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.popupBlockedCount = 0;
    testData.popupAllowedCount = 0;
    apiObject.onPopupAllowed.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupAllowedCount = testData.popupAllowedCount + 1;
    });
    apiObject.onPopupBlocked.addListener(function(e, blocked) {
      if (!e.url.endsWith('some-popup.html')) {
        return;
      }
      testData.popupBlockedCount = testData.popupBlockedCount + 1;
    });
    chrome.test.sendScriptResult('');
  )";

  auto read_allowed_stats_script = [](int expected) {
    std::string script = base::StringPrintf(
        R"(
        var intervalAllowedId = setInterval(function() {
          if (testData.popupAllowedCount == %d) {
            if (intervalAllowedId) {
              clearInterval(intervalAllowedId);
              intervalAllowedId = null;
            }
            chrome.test.sendScriptResult(
                testData.popupAllowedCount.toString());
          }
        }, 100))",
        expected);
    return script;
  };

  auto read_blocked_stats_script = [](int expected) {
    std::string script = base::StringPrintf(
        R"(
        var intervalBlockedId = setInterval(function() {
          if (testData.popupBlockedCount == %d) {
            if (intervalBlockedId) {
              clearInterval(intervalBlockedId);
              intervalBlockedId = null;
            }
            chrome.test.sendScriptResult(
               testData.popupBlockedCount.toString());
          }
        }, 100))",
        expected);
    return script;
  };

  constexpr char kBlockPopupScript[] = R"(
    apiObject.addCustomFilter(%s'some-popup.html^$popup');
    chrome.test.sendScriptResult('');
  )";

  constexpr char kAllowPopupScript[] = R"(
    apiObject.addCustomFilter(%s'@@some-popup.html^$popup');
    chrome.test.sendScriptResult('');
  )";

  const GURL test_url =
      embedded_test_server()->GetURL("example.com", "/test.html");
  constexpr char kOpenPopupScript[] =
      "document.getElementById('popup_id').click()";
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  SetupApiObjectAndMethods(extension->id());

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kBlockPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(1, std::stoi(ExecuteScriptInBackgroundPage(
                   extension->id(), read_blocked_stats_script(1))));

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kAllowPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(1, std::stoi(ExecuteScriptInBackgroundPage(
                   extension->id(), read_allowed_stats_script(1))));
  ASSERT_EQ(1, std::stoi(ExecuteScriptInBackgroundPage(
                   extension->id(), read_blocked_stats_script(1))));
}

IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTest, PopupStats) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  auto read_allowed_stats_script = [](int expected) {
    std::string script = base::StringPrintf(
        R"(
        var intervalAllowedId = setInterval(function() {
          apiObject[sessionAllowedCount](function(sessionStats) {
            let count = 0;
            for (const entry of sessionStats) {
              if (entry.url === 'adblock:custom') {
                count = entry.count;
              }
            }
            if (%d == 0 || count == %d) {
              if (intervalAllowedId) {
                clearInterval(intervalAllowedId);
                intervalAllowedId = null;
              }
              chrome.test.sendScriptResult(count.toString());
            }
          }
        )}, 100))",
        expected, expected);
    return script;
  };

  auto read_blocked_stats_script = [](int expected) {
    std::string script = base::StringPrintf(
        R"(
        var intervalBlockedId = setInterval(function() {
            apiObject[sessionBlockedCount](function(sessionStats) {
            let count = 0;
            for (const entry of sessionStats) {
              if (entry.url === 'adblock:custom') {
                count = entry.count;
              }
            }
            if (%d == 0 || count == %d) {
              if (intervalBlockedId) {
                clearInterval(intervalBlockedId);
                intervalBlockedId = null;
              }
              chrome.test.sendScriptResult(count.toString());
            }
          }
        )}, 100))",
        expected, expected);
    return script;
  };

  constexpr char kBlockPopupScript[] = R"(
    apiObject.addCustomFilter(%s'some-popup.html^$popup');
    chrome.test.sendScriptResult('');
  )";

  constexpr char kAllowPopupScript[] = R"(
    apiObject.addCustomFilter(%s'@@some-popup.html^$popup');
    chrome.test.sendScriptResult('');
  )";

  const GURL test_url =
      embedded_test_server()->GetURL("example.com", "/test.html");
  constexpr char kOpenPopupScript[] =
      "document.getElementById('popup_id').click()";
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  SetupApiObjectAndMethods(extension->id());

  int initial_allowed_value = std::stoi(ExecuteScriptInBackgroundPage(
      extension->id(), read_allowed_stats_script(0)));
  int initial_blocked_value = std::stoi(ExecuteScriptInBackgroundPage(
      extension->id(), read_blocked_stats_script(0)));

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kBlockPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(initial_blocked_value + 1,
            std::stoi(ExecuteScriptInBackgroundPage(
                extension->id(),
                read_blocked_stats_script(initial_blocked_value + 1))));

  ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kAllowPopupScript, IsOldApi() ? "" : "'adblock', "));
  ExecuteScript(kOpenPopupScript);
  ASSERT_EQ(initial_allowed_value + 1,
            std::stoi(ExecuteScriptInBackgroundPage(
                extension->id(),
                read_allowed_stats_script(initial_allowed_value + 1))));
  ASSERT_EQ(initial_blocked_value + 1,
            std::stoi(ExecuteScriptInBackgroundPage(
                extension->id(),
                read_blocked_stats_script(initial_blocked_value + 1))));
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockPrivateApiBackgroundPageTest,
    testing::Combine(testing::Values(EyeoExtensionApi::Old,
                                     EyeoExtensionApi::New),
                     testing::Values(Mode::Normal, Mode::Incognito)));

class AdblockPrivateApiBackgroundPageTestWithRedirect
    : public AdblockPrivateApiBackgroundPageTest {
 public:
  AdblockPrivateApiBackgroundPageTestWithRedirect() {}
  ~AdblockPrivateApiBackgroundPageTestWithRedirect() override = default;
  AdblockPrivateApiBackgroundPageTestWithRedirect(
      const AdblockPrivateApiBackgroundPageTestWithRedirect&) = delete;
  AdblockPrivateApiBackgroundPageTestWithRedirect& operator=(
      const AdblockPrivateApiBackgroundPageTestWithRedirect&) = delete;

 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    // Map domains to localhost. Add redirect handler.
    host_resolver()->AddRule(before_redirect_domain, "127.0.0.1");
    host_resolver()->AddRule(after_redirect_domain, "127.0.0.1");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &AdblockPrivateApiBackgroundPageTestWithRedirect::RequestHandler,
        base::Unretained(this)));
    ASSERT_TRUE(StartEmbeddedTestServer());
    adblock::SubscriptionServiceFactory::GetForBrowserContext(
        browser()->profile()->GetOriginalProfile())
        ->GetFilteringConfiguration(adblock::kAdblockFilteringConfigurationName)
        ->RemoveCustomFilter(adblock::kAllowlistEverythingFilter);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (base::EndsWith(request.relative_url, before_redirect_domain,
                       base::CompareCase::SENSITIVE)) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse());
      http_response->set_code(net::HTTP_FOUND);
      http_response->set_content("Redirecting...");
      http_response->set_content_type("text/plain");
      http_response->AddCustomHeader(
          "Location", "http://" + after_redirect_domain + ":" +
                          base::NumberToString(embedded_test_server()->port()) +
                          request.relative_url.substr(
                              0, request.relative_url.size() -
                                     before_redirect_domain.size() - 1));
      return std::move(http_response);
    }
    if (base::EndsWith(request.relative_url, subresource_with_redirect,
                       base::CompareCase::SENSITIVE)) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_OK);
      // Create a sub resource url which causes redirection
      const GURL url = embedded_test_server()->GetURL(
          before_redirect_domain, "/image.png?" + before_redirect_domain);
      std::string body = "<img src=\"" + url.spec() + "\">";
      http_response->set_content(body);
      http_response->set_content_type("text/html");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

  const std::string before_redirect_domain = "before-redirect.com";
  const std::string after_redirect_domain = "after-redirect.com";
  const std::string subresource_with_redirect = "subresource_with_redirect";
};

// Test for DPD-1519
// This test verifies redirection of a main page
IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTestWithRedirect,
                       PageAllowedEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.pageAllowedBeforeRedirectCount = 0;
    testData.pageAllowedAfterRedirectCount = 0;
    apiObject.onPageAllowed.addListener(function(e) {
      if (e.url.includes('before-redirect.com')) {
        ++testData.pageAllowedBeforeRedirectCount;
      } else if (e.url.includes('after-redirect.com')) {
        ++testData.pageAllowedAfterRedirectCount;
      }
    });
    chrome.test.sendScriptResult('');
  )";

  constexpr char kReadCountersScript[] = R"(
    chrome.test.sendScriptResult(
      testData.pageAllowedBeforeRedirectCount + '-' +
          testData.pageAllowedAfterRedirectCount);
  )";

  constexpr char kAddAllowDomainScript[] = R"(
    apiObject.addAllowedDomain(%s'after-redirect.com');
    chrome.test.sendScriptResult('');
  )";

  constexpr char kRemoveAllowDomainScript[] = R"(
    apiObject.removeAllowedDomain(%s'after-redirect.com');
    chrome.test.sendScriptResult('');
  )";

  constexpr char kAddBlockDomainFilterScript[] = R"(
    apiObject.addCustomFilter(%s'after-redirect.com');
    chrome.test.sendScriptResult('');
  )";

  // Because RequestHandler handler sees just a 127.0.0.1 instead of
  // a domain we are passing here source domain as a path in url.
  const GURL test_url = embedded_test_server()->GetURL(
      before_redirect_domain, "/" + before_redirect_domain);

  SetupApiObjectAndMethods(extension->id());

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);

  // No filter
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("0-0", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  // Just allow filter
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kAddAllowDomainScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("0-1", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));
  // Just block filter
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kAddBlockDomainFilterScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kRemoveAllowDomainScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("0-1", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  // Allow and block filter
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kAddAllowDomainScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("0-2", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));
}

// This test verifies redirection of a sub resource
IN_PROC_BROWSER_TEST_P(AdblockPrivateApiBackgroundPageTestWithRedirect,
                       AdMatchedEvents) {
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("adblock_private"),
                    {.allow_in_incognito = IsIncognito()});
  ASSERT_TRUE(extension);

  constexpr char kSetListenersScript[] = R"(
    var testData = {};
    testData.adBlockedCount = 0;
    testData.adAllowedCount = 0;
    apiObject[onBlockedEvent].addListener(function(e) {
      if (e.url.includes('http://after-redirect.com')) {
        ++testData.adBlockedCount;
      }
    });
    apiObject[onAllowedEvent].addListener(function(e) {
      if (e.url.includes('http://after-redirect.com')) {
        ++testData.adAllowedCount;
      }
    });
    chrome.test.sendScriptResult('');
  )";

  constexpr char kReadCountersScript[] = R"(
    chrome.test.sendScriptResult(
      testData.adBlockedCount + '-' + testData.adAllowedCount);
  )";

  constexpr char kAddBlockingFilterScript[] = R"(
    apiObject.addCustomFilter(%s'||after-redirect.com*/image.png');
    chrome.test.sendScriptResult('');
  )";

  constexpr char kAddAllowingFilterScript[] = R"(
    apiObject.addCustomFilter(%s'@@||after-redirect.com*/image.png');
    chrome.test.sendScriptResult('');
  )";

  SetupApiObjectAndMethods(extension->id());

  ExecuteScriptInBackgroundPage(extension->id(), kSetListenersScript);

  const GURL test_url = embedded_test_server()->GetURL(
      after_redirect_domain, "/" + subresource_with_redirect);

  // No filter
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("0-0", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  // Just block filter
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kAddBlockingFilterScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("1-0", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));

  // Allow and block filter
  ExecuteScriptInBackgroundPage(
      extension->id(), base::StringPrintf(kAddAllowingFilterScript,
                                          IsOldApi() ? "" : "'adblock', "));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_EQ("1-1", ExecuteScriptInBackgroundPage(extension->id(),
                                                 kReadCountersScript));
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockPrivateApiBackgroundPageTestWithRedirect,
    testing::Combine(testing::Values(EyeoExtensionApi::Old,
                                     EyeoExtensionApi::New),
                     testing::Values(Mode::Normal, Mode::Incognito)));

}  // namespace extensions

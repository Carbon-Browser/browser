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

#include "base/check.h"
#include "base/environment.h"
#include "base/functional/callback_forward.h"
#include "base/run_loop.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/adblock_switches.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/version_info/version_info.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {

class AdblockFilterListDownloadTestBase : public InProcessBrowserTest {
 public:
  AdblockFilterListDownloadTestBase()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  // We need to set server and request handler asap
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    host_resolver()->AddRule("easylist-downloads.adblockplus.org", "127.0.0.1");
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&AdblockFilterListDownloadTestBase::RequestHandler,
                            base::Unretained(this)));
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org"};
    https_server_.SetSSLConfig(cert_config);
    ASSERT_TRUE(https_server_.Start());
    SetFilterListServerPortForTesting(https_server_.port());
  }

  void CheckRequestParams(const net::test_server::HttpRequest& request,
                          std::string expected_disabled_value) {
    std::string os;
    base::ReplaceChars(version_info::GetOSType(), base::kWhitespaceASCII, "",
                       &os);
    EXPECT_TRUE(request.relative_url.find("addonName=eyeo-chromium-sdk") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("addonVersion=2.0.0") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("platformVersion=1.0") !=
                std::string::npos);
    EXPECT_TRUE(request.relative_url.find("platform=" + os) !=
                std::string::npos);
    if (RunsOnEyeoCI()) {
      // Those two checks below require "eyeo_application_name" and
      // "eyeo_application_version" to be set as gn gen args.
      EXPECT_TRUE(
          request.relative_url.find("application=app_name_from_ci_config") !=
          std::string::npos)
          << "Did you set \"eyeo_application_name\" gn gen arg?";
      EXPECT_TRUE(request.relative_url.find(
                      "applicationVersion=app_version_from_ci_config") !=
                  std::string::npos)
          << "Did you set \"eyeo_application_version\" gn gen arg?";
    }
    EXPECT_TRUE(
        request.relative_url.find("disabled=" + expected_disabled_value) !=
        std::string::npos);
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(request.relative_url, "/easylist.txt") ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt"))) {
      CheckRequestParams(request, "false");
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }

    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  void NotifyTestFinished() {
    finish_condition_met_ = true;
    // If the test is currently waiting for the finish condition to be met, we
    // need to quit the run loop.
    if (quit_closure_) {
      quit_closure_.Run();
    }
  }

  void RunUntilTestFinished() {
    // If the finish condition is already met, we don't need to run the run
    // loop.
    if (finish_condition_met_) {
      return;
    }
    // Wait until NotifyTestFinished() gets called.
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    std::move(run_loop).Run();
  }

  bool RunsOnEyeoCI() {
    auto env = base::Environment::Create();
    std::string value;
    env->GetVar("CI_PROJECT_NAME", &value);
    return value == "chromium-sdk";
  }

 protected:
  net::EmbeddedTestServer https_server_;
  std::set<std::string> default_lists_;
  bool finish_condition_met_ = false;
  base::RepeatingClosure quit_closure_;
};

class AdblockEnabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase {
 public:
  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    auto result = AdblockFilterListDownloadTestBase::RequestHandler(request);
    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (default_lists_.size() == 3) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return result;
  }
};

IN_PROC_BROWSER_TEST_F(AdblockEnabledFilterListDownloadTest,
                       TestInitialDownloads) {
  RunUntilTestFinished();
}

class AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase {
 public:
  AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest() {
    const auto testing_interval = base::Seconds(1);
    SubscriptionServiceFactory::SetUpdateCheckAndDelayIntervalsForTesting(
        testing_interval, testing_interval);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    // If we get expected HEAD request we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (request.method == net::test_server::HttpMethod::METHOD_HEAD &&
        base::StartsWith(request.relative_url, "/exceptionrules.txt")) {
      CheckRequestParams(request, "true");
      NotifyTestFinished();
    }

    return nullptr;
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(adblock::switches::kDisableAcceptableAds);
  }
};

IN_PROC_BROWSER_TEST_F(
    AdblockEnabledAcceptableAdsDisabledFilterListDownloadTest,
    TestInitialDownloads) {
  RunUntilTestFinished();
}

enum class DisableSwitch { Adblock, Eyeo };

class AdblockDisabledFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase,
      public testing::WithParamInterface<DisableSwitch> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(GetParam() == DisableSwitch::Adblock
                                   ? adblock::switches::kDisableAdblock
                                   : adblock::switches::kDisableEyeoFiltering);
  }

  void VerifyNoDownloads() {
    ASSERT_EQ(0u, default_lists_.size());
    NotifyTestFinished();
  }
};

IN_PROC_BROWSER_TEST_P(AdblockDisabledFilterListDownloadTest,
                       TestInitialDownloads) {
  // This test assumes that inital downloads (for adblock enabled) will happen
  // within 10 seconds. When tested locally it always happens within 3 seconds.
  base::OneShotTimer timer;
  timer.Start(
      FROM_HERE, base::Seconds(10),
      base::BindOnce(&AdblockDisabledFilterListDownloadTest::VerifyNoDownloads,
                     base::Unretained(this)));
  RunUntilTestFinished();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockDisabledFilterListDownloadTest,
                         testing::Values(DisableSwitch::Adblock,
                                         DisableSwitch::Eyeo));

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

enum class Country {
  Arabic,
  Bulgaria,
  China,
  Czech,
  France,
  Germany,
  Hungary,
  India,
  Indonesia,
  Israel,
  Italy,
  Japan,
  Korea,
  Latvia,
  Lithuania,
  Netherlands,
  Norway,
  Poland,
  Portugal,
  Romania,
  Russia,
  Spain,
  Thailand,
  Turkey,
  Vietnam
};

class AdblockLocaleFilterListDownloadTest
    : public AdblockFilterListDownloadTestBase,
      public testing::WithParamInterface<Country> {
 public:
  using LocaleToEasylistPath = std::pair<std::string, std::string>;
  AdblockLocaleFilterListDownloadTest() : locale_data_(GetLocale(GetParam())) {
    setenv("LANGUAGE", locale_data_.first.c_str(), 1);
  }

  std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) override {
    EXPECT_FALSE(base::StartsWith(request.relative_url, "/easylist.txt"));
    if (request.method == net::test_server::HttpMethod::METHOD_GET &&
        (base::StartsWith(request.relative_url, "/abp-filters-anti-cv.txt") ||
         base::StartsWith(
             request.relative_url,
             GetCountrySpecificEasylistPath(locale_data_.first.substr(0, 2),
                                            locale_data_.second)) ||
         base::StartsWith(request.relative_url, "/exceptionrules.txt"))) {
      CheckRequestParams(request, "false");
      default_lists_.insert(request.relative_url.substr(
          1, request.relative_url.find_first_of("?") - 1));
    }

    // If we get all expected requests we simply finish the test by closing
    // the browser, otherwise test will fail with a timeout.
    if (default_lists_.size() == 3) {
      NotifyTestFinished();
    }

    // Unhandled requests result in the Embedded test server sending a 404.
    return nullptr;
  }

 private:
  LocaleToEasylistPath locale_data_;
  LocaleToEasylistPath GetLocale(Country country) {
    switch (country) {
      case Country::Arabic:
        return std::make_pair("ar_SA", "/liste_ar+liste_fr+easylist.txt");
      case Country::Bulgaria:
        return std::make_pair("bg_BG", "/bulgarian_list+easylist.txt");
      case Country::China:
        return std::make_pair("zh_CN", "/easylistchina+easylist.txt");
      case Country::Czech:
        return std::make_pair("cs_CZ", "/easylistczechslovak+easylist.txt");
      case Country::France:
        return std::make_pair("fr_FR", "/liste_fr+easylist.txt");
      case Country::Germany:
        return std::make_pair("de_DE", "/easylistgermany+easylist.txt");
      case Country::Hungary:
        return std::make_pair("hu_HU", "/hufilter+easylist.txt");
      case Country::India:
        return std::make_pair("ml_IN", "/indianlist+easylist.txt");
      case Country::Indonesia:
        return std::make_pair("id_ID", "/abpindo+easylist.txt");
      case Country::Israel:
        return std::make_pair("he_IL", "/israellist+easylist.txt");
      case Country::Italy:
        return std::make_pair("it_IT", "/easylistitaly+easylist.txt");
      case Country::Japan:
        return std::make_pair("ja_JP", "/japanese-filters+easylist.txt");
      case Country::Korea:
        return std::make_pair("ko_KR", "/koreanlist+easylist.txt");
      case Country::Latvia:
        return std::make_pair("lv_LV", "/latvianlist+easylist.txt");
      case Country::Lithuania:
        return std::make_pair("lt_LT", "/easylistlithuania+easylist.txt");
      case Country::Netherlands:
        return std::make_pair("nl_NL", "/easylistdutch+easylist.txt");
      case Country::Norway:
        return std::make_pair("no_NO",
                              "/dandelion_sprouts_nordic_filters+easylist.txt");
      case Country::Poland:
        return std::make_pair("pl_PL", "/easylistpolish+easylist.txt");
      case Country::Portugal:
        return std::make_pair("pt_PT", "/easylistportuguese+easylist.txt");
      case Country::Romania:
        return std::make_pair("ro_RO", "/rolist+easylist.txt");
      case Country::Russia:
        return std::make_pair("ru_RU", "/ruadlist+easylist.txt");
      case Country::Spain:
        return std::make_pair("es_ES", "/easylistspanish+easylist.txt");
      case Country::Thailand:
        return std::make_pair("th_TH", "/global-filters+easylist.txt");
      case Country::Turkey:
        return std::make_pair("tr_TR", "/turkish-filters+easylist.txt");
      case Country::Vietnam:
        return std::make_pair("vi_VN", "/abpvn+easylist.txt");
      default:
        NOTREACHED();
        return std::make_pair("en_US", "/easylist.txt");
    }
  }

  std::string GetCountrySpecificEasylistPath(std::string_view language,
                                             std::string_view expected_path) {
    auto it = base::ranges::find_if(
        config::GetKnownSubscriptions(), [&](const auto& config_entry) {
          return std::find(config_entry.languages.begin(),
                           config_entry.languages.end(),
                           language) != config_entry.languages.end();
        });
    EXPECT_TRUE(it != config::GetKnownSubscriptions().end());
    if (it != config::GetKnownSubscriptions().end()) {
      EXPECT_EQ(expected_path, it->url.path());
      return it->url.path();
    }
    return DefaultSubscriptionUrl().path();
  }
};

IN_PROC_BROWSER_TEST_P(AdblockLocaleFilterListDownloadTest,
                       TestInitialDownloads) {
  RunUntilTestFinished();
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdblockLocaleFilterListDownloadTest,
                         testing::Values(Country::Arabic,
                                         Country::Bulgaria,
                                         Country::China,
                                         Country::Czech,
                                         Country::France,
                                         Country::Germany,
                                         Country::Hungary,
                                         Country::India,
                                         Country::Indonesia,
                                         Country::Israel,
                                         Country::Italy,
                                         Country::Japan,
                                         Country::Korea,
                                         Country::Latvia,
                                         Country::Lithuania,
                                         Country::Netherlands,
                                         Country::Norway,
                                         Country::Poland,
                                         Country::Portugal,
                                         Country::Romania,
                                         Country::Russia,
                                         Country::Spain,
                                         Country::Thailand,
                                         Country::Turkey,
                                         Country::Vietnam));

#endif

}  // namespace adblock

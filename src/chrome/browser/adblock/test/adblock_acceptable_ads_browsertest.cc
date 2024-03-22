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

#include "base/environment.h"
#include "base/run_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/content/browser/adblock_filter_match.h"
#include "components/adblock/content/browser/factories/resource_classification_runner_factory.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/adblock_switches.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace adblock {
namespace {
class SubscriptionInstalledWaiter
    : public SubscriptionService::SubscriptionObserver {
 public:
  explicit SubscriptionInstalledWaiter(
      SubscriptionService* subscription_service)
      : subscription_service_(subscription_service) {
    subscription_service_->AddObserver(this);
  }

  ~SubscriptionInstalledWaiter() override {
    subscription_service_->RemoveObserver(this);
  }

  void WaitUntilSubscriptionsInstalled(std::vector<std::string> subscriptions) {
    awaited_subscriptions_ = std::move(subscriptions);
    run_loop_.Run();
  }

  void OnSubscriptionInstalled(const GURL& subscription_url) override {
    awaited_subscriptions_.erase(
        base::ranges::remove(awaited_subscriptions_, subscription_url.path()),
        awaited_subscriptions_.end());
    if (awaited_subscriptions_.empty()) {
      run_loop_.Quit();
    }
  }

 protected:
  raw_ptr<SubscriptionService> subscription_service_;
  base::RunLoop run_loop_;
  std::vector<std::string> awaited_subscriptions_;
};

class ResourceClassificationRunnerObserver
    : public ResourceClassificationRunner::Observer {
 public:
  // ResourceClassificationRunner::Observer:
  void OnRequestMatched(const GURL& url,
                        FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override {
    if (match_result == FilterMatchResult::kAllowRule) {
      allowed_ads_notifications.push_back(url);
    } else {
      blocked_ads_notifications.push_back(url);
    }
  }

  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override {
    allowed_pages_notifications.push_back(url);
  }

  void OnPopupMatched(const GURL& url,
                      FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override {}

  std::vector<GURL> blocked_ads_notifications;
  std::vector<GURL> allowed_ads_notifications;
  std::vector<GURL> allowed_pages_notifications;
};
}  // namespace

class AdblockAcceptableAdsTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<std::tuple<bool, bool, bool>> {
 public:
  AdblockAcceptableAdsTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
    setenv("LANGUAGE", "en_US", 1);
#endif
  }

  // We need to set server and request handler asap
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    host_resolver()->AddRule("easylist-downloads.adblockplus.org", "127.0.0.1");
    host_resolver()->AddRule(kTestDomain, "127.0.0.1");
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data/adblock");
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdblockAcceptableAdsTest::RequestHandler, base::Unretained(this)));
    net::EmbeddedTestServer::ServerCertificateConfig cert_config;
    cert_config.dns_names = {"easylist-downloads.adblockplus.org", kTestDomain};
    https_server_.SetSSLConfig(cert_config);
    ASSERT_TRUE(https_server_.Start());
    SetFilterListServerPortForTesting(https_server_.port());
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    auto* classification_runner =
        ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile());
    DCHECK(classification_runner);
    classification_runner->AddObserver(&observer);
    auto* adblock_configuration =
        SubscriptionServiceFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile())
            ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
    DCHECK(adblock_configuration);
    adblock_configuration->RemoveCustomFilter(kAllowlistEverythingFilter);
    if (DomainAllowlisted()) {
      adblock_configuration->AddAllowedDomain(kTestDomain);
    }
  }

  void TearDownOnMainThread() override {
    auto* classification_runner =
        ResourceClassificationRunnerFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile());
    classification_runner->RemoveObserver(&observer);
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (!AcceptableAdsEnabled()) {
      command_line->AppendSwitch(switches::kDisableAcceptableAds);
    }
    if (IncognitoMode()) {
      command_line->AppendSwitch(::switches::kIncognito);
    }
  }

  void WaitUntilSubscriptionsInstalled() {
    std::vector<std::string> subscriptions = {DefaultSubscriptionUrl().path()};
    if (AcceptableAdsEnabled()) {
      subscriptions.emplace_back(AcceptableAdsUrl().path());
    }
    SubscriptionInstalledWaiter waiter(
        SubscriptionServiceFactory::GetForBrowserContext(
            browser()->profile()->GetOriginalProfile()));
    waiter.WaitUntilSubscriptionsInstalled(std::move(subscriptions));
  }

  virtual std::unique_ptr<net::test_server::HttpResponse> RequestHandler(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() == AcceptableAdsUrl().path()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(
          "[Adblock Plus 2.0]\n\n"
          "@@*resource.png\n");
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    } else if (request.GetURL().path() == DefaultSubscriptionUrl().path()) {
      std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
          new net::test_server::BasicHttpResponse);
      http_response->set_code(net::HTTP_OK);
      http_response->set_content(
          "[Adblock Plus 2.0]\n\n"
          "*resource.png\n");
      http_response->set_content_type("text/plain");
      return std::move(http_response);
    }

    // Unhandled requests result in the Embedded test server sending a 404. This
    // is fine for the purpose of this test.
    return nullptr;
  }

  GURL GetPageUrl(const std::string& path = "/innermost_frame.html") {
    return https_server_.GetURL(kTestDomain, path);
  }

  void NavigateToPage() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GetPageUrl()));
  }

  void VerifyExpectedNotifications() {
    if (AcceptableAdsEnabled() || DomainAllowlisted()) {
      ASSERT_EQ(observer.allowed_ads_notifications.size(), 1u);
      EXPECT_TRUE(observer.allowed_ads_notifications.front() ==
                  GetPageUrl("/resource.png"))
          << "Request not allowed!";
      if (DomainAllowlisted()) {
        ASSERT_EQ(observer.allowed_pages_notifications.size(), 1u);
        EXPECT_TRUE(observer.allowed_pages_notifications.front() ==
                    GetPageUrl())
            << "Page not allowed!";
      }
    } else {
      ASSERT_EQ(observer.blocked_ads_notifications.size(), 1u);
      EXPECT_TRUE(observer.blocked_ads_notifications.front() ==
                  GetPageUrl("/resource.png"))
          << "Request not blocked!";
      EXPECT_TRUE(observer.allowed_ads_notifications.empty());
      EXPECT_TRUE(observer.allowed_pages_notifications.empty());
    }
  }

  bool AcceptableAdsEnabled() { return std::get<0>(GetParam()); }

  bool DomainAllowlisted() { return std::get<1>(GetParam()); }

  bool IncognitoMode() { return std::get<2>(GetParam()); }

 private:
  net::EmbeddedTestServer https_server_;
  ResourceClassificationRunnerObserver observer;
  static constexpr char kTestDomain[] = "test.org";
};

IN_PROC_BROWSER_TEST_P(AdblockAcceptableAdsTest, VerifyAcceptableAds) {
  auto* adblock_configuration =
      SubscriptionServiceFactory::GetForBrowserContext(
          browser()->profile()->GetOriginalProfile())
          ->GetFilteringConfiguration(kAdblockFilteringConfigurationName);
  DCHECK(adblock_configuration);
  auto subscriptions = adblock_configuration->GetFilterLists();
  // This remove/add dance is required to avoid race when we are not sure
  // if subscriptions were already installed or not. It's difficult to set
  // SubscriptionObserver for built-in subscriptions in right time during
  // test setup so we do it here and by Remove() then Add() we trigger
  // filter lists installations which we then observe and block until done.
  for (const auto& subscription : subscriptions) {
    adblock_configuration->RemoveFilterList(subscription);
    adblock_configuration->AddFilterList(subscription);
  }
  WaitUntilSubscriptionsInstalled();
  NavigateToPage();
  VerifyExpectedNotifications();
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AdblockAcceptableAdsTest,
    testing::Combine(/* AA on/off */ testing::Bool(),
                     /* Allowlist domain */ testing::Bool(),
                     /* Incognito on/off */ testing::Bool()));

}  // namespace adblock

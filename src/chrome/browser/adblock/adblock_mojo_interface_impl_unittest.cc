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

#include "chrome/browser/adblock/adblock_mojo_interface_impl.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/adblock/content_security_policy_injector_factory.h"
#include "chrome/browser/adblock/element_hider_factory.h"
#include "chrome/browser/adblock/resource_classification_runner_factory.h"
#include "chrome/browser/adblock/sitekey_storage_factory.h"
#include "chrome/browser/adblock/subscription_service_factory.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/adblock/content/browser/adblock_content_utils.h"
#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/content/browser/test/mock_adblock_content_security_policy_injector.h"
#include "components/adblock/content/browser/test/mock_element_hider.h"
#include "components/adblock/content/browser/test/mock_resource_classification_runner.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "components/adblock/core/test/mock_sitekey_storage.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/parsed_headers.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

using testing::_;
using testing::Return;
using MockCheckFilterMatchCallback = base::MockCallback<
    adblock::mojom::AdblockInterface::CheckFilterMatchCallback>;
using MockProcessResponseHeadersCallback = base::MockCallback<
    adblock::mojom::AdblockInterface::ProcessResponseHeadersCallback>;

namespace {

static BrowserContextKeyedServiceFactory::TestingFactory
AdblockResourceClassificationRunnerTestingFactory() {
  return base::BindRepeating(
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<adblock::MockResourceClassificationRunner>();
      });
}

static BrowserContextKeyedServiceFactory::TestingFactory
SitekeyStorageTestingFactory() {
  return base::BindRepeating(
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<adblock::MockSitekeyStorage>();
      });
}

static BrowserContextKeyedServiceFactory::TestingFactory
AdblockContentSecurityPolicyInjectorTestingFactory() {
  return base::BindRepeating(
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<
            adblock::MockAdblockContentSecurityPolicyInjector>();
      });
}

static BrowserContextKeyedServiceFactory::TestingFactory
ElementHiderFactoryTestingFactory() {
  return base::BindRepeating(
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<adblock::MockElementHider>();
      });
}

static BrowserContextKeyedServiceFactory::TestingFactory
SubscriptionServiceTestingFactory() {
  return base::BindRepeating(
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<adblock::MockSubscriptionService>();
      });
}

enum class SubscriptionServiceInitialState { Initialized, NotInitialized };

}  // namespace

namespace adblock {

class AdblockMojoInterfaceImplTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    mock_resource_classification_runner_ =
        static_cast<MockResourceClassificationRunner*>(
            ResourceClassificationRunnerFactory::GetInstance()
                ->SetTestingFactoryAndUse(
                    browser_context(),
                    AdblockResourceClassificationRunnerTestingFactory()));

    mock_sitekey_storage_ = static_cast<MockSitekeyStorage*>(
        SitekeyStorageFactory::GetInstance()->SetTestingFactoryAndUse(
            browser_context(), SitekeyStorageTestingFactory()));

    mock_csp_injector_ = static_cast<MockAdblockContentSecurityPolicyInjector*>(
        ContentSecurityPolicyInjectorFactory::GetInstance()
            ->SetTestingFactoryAndUse(
                browser_context(),
                AdblockContentSecurityPolicyInjectorTestingFactory()));

    mock_element_hider_ = static_cast<MockElementHider*>(
        ElementHiderFactory::GetInstance()->SetTestingFactoryAndUse(
            browser_context(), ElementHiderFactoryTestingFactory()));

    mock_subscription_service_ = static_cast<MockSubscriptionService*>(
        SubscriptionServiceFactory::GetInstance()->SetTestingFactoryAndUse(
            browser_context(), SubscriptionServiceTestingFactory()));
  }

  void InitializeSubscriptionService() {
    EXPECT_CALL(*mock_subscription_service_, IsInitialized())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_subscription_service_, GetCurrentSnapshot())
        .WillRepeatedly(
            []() { return std::make_unique<MockSubscriptionCollection>(); });
    for (auto& task : deferred_tasks_) {
      std::move(task).Run();
    }
  }

  void SetSubscriptionServiceInitialState(
      SubscriptionServiceInitialState state) {
    EXPECT_CALL(*mock_subscription_service_, IsInitialized())
        .WillRepeatedly(testing::Return(
            state == SubscriptionServiceInitialState::Initialized));
    if (state == SubscriptionServiceInitialState::NotInitialized) {
      // When the SubscriptionService is not initialized, testee will defer
      // tasks to run after it becomes initialized via RunWhenInitialized().
      // Remember those tasks, we will run them in
      // EnsureSubscriptionServiceInitialized().
      EXPECT_CALL(*mock_subscription_service_, RunWhenInitialized(_))
          .WillRepeatedly([&](base::OnceClosure task) {
            deferred_tasks_.push_back(std::move(task));
          });
    }
  }

  void TearDown() override { ChromeRenderViewHostTestHarness::TearDown(); }

  TestingProfile::TestingFactories GetTestingFactories() const override {
    return {{ResourceClassificationRunnerFactory::GetInstance(),
             AdblockResourceClassificationRunnerTestingFactory()}};
  }

  static constexpr int INVALID_RENDER_PROCESS_ID = 42;
  static constexpr int DUMMY_RENDER_FRAME_ID = 42;
  static constexpr int DUMMY_RESOURCE_TYPE = 42;

  raw_ptr<MockResourceClassificationRunner>
      mock_resource_classification_runner_;
  raw_ptr<MockSitekeyStorage> mock_sitekey_storage_;
  raw_ptr<MockAdblockContentSecurityPolicyInjector> mock_csp_injector_;
  raw_ptr<MockElementHider> mock_element_hider_;
  raw_ptr<MockSubscriptionService> mock_subscription_service_;
  std::vector<base::OnceClosure> deferred_tasks_;
};

TEST_F(AdblockMojoInterfaceImplTest, CheckFilterMatch) {
  MockCheckFilterMatchCallback callback;
  mojom::AdblockInterface::CheckFilterMatchCallback callback_captured_by_rq;
  const GURL request_url("https://request_url.com");

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, request_url, DUMMY_RESOURCE_TYPE,
                                      main_rfh()->GetProcess()->GetID(),
                                      main_rfh()->GetRoutingID(), _))
      .Times(1)
      .WillOnce([&](auto, const GURL&, int32_t, int32_t, int32_t,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        callback_captured_by_rq = std::move(cb);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());
  adblock_mojo_interface->CheckFilterMatch(request_url, DUMMY_RESOURCE_TYPE,
                                           main_rfh()->GetRoutingID(),
                                           callback.Get());

  // |callback| ran in response to ResourceClassificationRunner running
  // |callback_captured_by_rq|.
  EXPECT_CALL(callback, Run(mojom::FilterMatchResult::kAllowRule)).Times(1);
  std::move(callback_captured_by_rq).Run(mojom::FilterMatchResult::kAllowRule);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ChecktFilterMatchDeferred) {
  MockCheckFilterMatchCallback callback;
  mojom::AdblockInterface::CheckFilterMatchCallback callback_captured_by_rq;
  const GURL request_url("https://request_url.com");

  SetSubscriptionServiceInitialState(
      SubscriptionServiceInitialState::NotInitialized);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, request_url, DUMMY_RESOURCE_TYPE,
                                      main_rfh()->GetProcess()->GetID(),
                                      main_rfh()->GetRoutingID(), _))
      .Times(1)
      .WillOnce([&](auto, const GURL&, int32_t, int32_t, int32_t,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        callback_captured_by_rq = std::move(cb);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());
  adblock_mojo_interface->CheckFilterMatch(request_url, DUMMY_RESOURCE_TYPE,
                                           main_rfh()->GetRoutingID(),
                                           callback.Get());

  InitializeSubscriptionService();
  // |callback| ran in response to ResourceClassificationRunner running
  // |callback_captured_by_rq|.
  EXPECT_CALL(callback, Run(mojom::FilterMatchResult::kAllowRule)).Times(1);
  std::move(callback_captured_by_rq).Run(mojom::FilterMatchResult::kAllowRule);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, CheckFilterMatchNoProcessHost) {
  MockCheckFilterMatchCallback callback;
  const GURL request_url("https://request_url.com");

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(callback, Run(mojom::FilterMatchResult::kNoRule)).Times(1);

  auto adblock_mojo_interface =
      std::make_unique<AdblockMojoInterfaceImpl>(INVALID_RENDER_PROCESS_ID);
  adblock_mojo_interface->CheckFilterMatch(
      request_url, DUMMY_RESOURCE_TYPE, DUMMY_RENDER_FRAME_ID, callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest,
       ChecktFilterMatch_RenderFrameHostDiesWhileTaskDeferred) {
  MockCheckFilterMatchCallback callback;
  const GURL request_url("https://request_url.com");

  SetSubscriptionServiceInitialState(
      SubscriptionServiceInitialState::NotInitialized);

  EXPECT_CALL(callback, Run(mojom::FilterMatchResult::kNoRule)).Times(1);

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());
  adblock_mojo_interface->CheckFilterMatch(request_url, DUMMY_RESOURCE_TYPE,
                                           main_rfh()->GetRoutingID(),
                                           callback.Get());

  DeleteContents();

  InitializeSubscriptionService();

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest,
       ProcessResponseHeaders_BlockedWithHeaderFilter) {
  MockProcessResponseHeadersCallback callback;
  const GURL response_url("https://request_url.com");
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  auto headers_filter_result = mojom::FilterMatchResult::kBlockRule;
  auto render_frame_host_id = main_rfh()->GetRoutingID();

  mojom::AdblockInterface::ProcessResponseHeadersCallback
      callback_captured_by_csp_injector;
  mojom::AdblockInterface::CheckFilterMatchCallback
      continue_process_response_headers_callback;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckResponseFilterMatch(_, response_url, _,
                                       main_rfh()->GetRoutingID(), headers, _))
      .WillOnce([&](auto, const GURL&, int32_t process_id,
                    int32_t render_frame_id,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        continue_process_response_headers_callback = std::move(cb);
      });
  // Sitekey and CSP Injector will not be called because the response was
  // blocked entirely.
  EXPECT_CALL(*mock_sitekey_storage_, ProcessResponseHeaders(_, _, _)).Times(0);
  EXPECT_CALL(*mock_csp_injector_,
              InsertContentSecurityPolicyHeadersIfApplicable(_, _, _, _))
      .Times(0);

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());
  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, render_frame_host_id, headers, "", callback.Get());

  EXPECT_CALL(callback, Run(headers_filter_result, testing::IsFalse()))
      .Times(1);
  std::move(continue_process_response_headers_callback)
      .Run(headers_filter_result);

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ProcessResponseHeaders_CspInjection) {
  MockProcessResponseHeadersCallback callback;
  const GURL response_url("https://request_url.com");
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  auto headers_filter_result = mojom::FilterMatchResult::kNoRule;

  mojom::AdblockInterface::CheckFilterMatchCallback
      continue_process_response_headers_callback;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckResponseFilterMatch(_, response_url, _,
                                       main_rfh()->GetRoutingID(), _, _))
      .WillOnce([&](auto, const GURL&, int32_t process_id,
                    int32_t render_frame_id,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        continue_process_response_headers_callback = std::move(cb);
      });
  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());
  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, main_rfh()->GetRoutingID(), headers, "", callback.Get());

  InsertContentSecurityPolicyHeadersCallback callback_captured_by_csp_injector;

  EXPECT_CALL(*mock_sitekey_storage_,
              ProcessResponseHeaders(response_url, headers, ""))
      .Times(1);
  EXPECT_CALL(
      *mock_csp_injector_,
      InsertContentSecurityPolicyHeadersIfApplicable(
          response_url,
          content::GlobalRenderFrameHostId(main_rfh()->GetProcess()->GetID(),
                                           main_rfh()->GetRoutingID()),
          headers, _))
      .Times(1)
      .WillOnce([&](const GURL&, content::GlobalRenderFrameHostId,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    InsertContentSecurityPolicyHeadersCallback cb) {
        callback_captured_by_csp_injector = std::move(cb);
      });
  std::move(continue_process_response_headers_callback)
      .Run(headers_filter_result);

  // |callback| ran in response to ContentPolicySecurityInjector running
  // |callback_captured_by_csp_injector|.
  EXPECT_CALL(callback, Run(headers_filter_result, testing::IsFalse()))
      .Times(1);
  std::move(callback_captured_by_csp_injector)
      .Run(mojo::StructPtr<network::mojom::ParsedHeaders>());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ProcessResponseHeadersNoProcessHost) {
  MockProcessResponseHeadersCallback callback;
  const GURL response_url("https://request_url.com");
  auto headers = net::HttpResponseHeaders::TryToCreate("");

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_sitekey_storage_, ProcessResponseHeaders(_, _, _)).Times(0);
  EXPECT_CALL(*mock_csp_injector_,
              InsertContentSecurityPolicyHeadersIfApplicable(_, _, _, _))
      .Times(0);
  EXPECT_CALL(callback, Run(_, _)).Times(1);

  auto adblock_mojo_interface =
      std::make_unique<AdblockMojoInterfaceImpl>(INVALID_RENDER_PROCESS_ID);
  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, DUMMY_RENDER_FRAME_ID, headers, "", callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_BlockedRequestHidden) {
  MockCheckFilterMatchCallback mock_check_filter_match_callback;
  const GURL request_url("https://request_url.com");
  auto resource_type = static_cast<int32_t>(blink::mojom::ResourceType::kImage);
  auto classification_result = mojom::FilterMatchResult::kBlockRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Image))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(request_url, _))
      .Times(1);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, request_url, resource_type,
                                      main_rfh()->GetProcess()->GetID(),
                                      main_rfh()->GetRoutingID(), _))
      .WillOnce([&](auto, const GURL&, int32_t resource_type,
                    int32_t process_id, int32_t render_frame_id,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->CheckFilterMatch(
      request_url, resource_type, main_rfh()->GetRoutingID(),
      mock_check_filter_match_callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_RequestNotHideable) {
  MockCheckFilterMatchCallback mock_check_filter_match_callback;
  const GURL request_url("https://request_url.com");
  auto resource_type = static_cast<int32_t>(blink::mojom::ResourceType::kPing);
  auto classification_result = mojom::FilterMatchResult::kBlockRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Ping))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(request_url, _))
      .Times(0);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, request_url, resource_type,
                                      main_rfh()->GetProcess()->GetID(),
                                      main_rfh()->GetRoutingID(), _))
      .WillOnce([&](auto, const GURL&, int32_t resource_type,
                    int32_t process_id, int32_t render_frame_id,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->CheckFilterMatch(
      request_url, resource_type, main_rfh()->GetRoutingID(),
      mock_check_filter_match_callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_AllowedRequestNotHidden) {
  MockCheckFilterMatchCallback mock_check_filter_match_callback;
  const GURL request_url("https://request_url.com");
  auto resource_type = static_cast<int32_t>(blink::mojom::ResourceType::kImage);
  auto classification_result = mojom::FilterMatchResult::kAllowRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Image))
      .Times(0);
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(request_url, _))
      .Times(0);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckRequestFilterMatch(_, request_url, resource_type,
                                      main_rfh()->GetProcess()->GetID(),
                                      main_rfh()->GetRoutingID(), _))
      .WillOnce([&](auto, const GURL&, int32_t resource_type,
                    int32_t process_id, int32_t render_frame_id,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->CheckFilterMatch(
      request_url, resource_type, main_rfh()->GetRoutingID(),
      mock_check_filter_match_callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_BlockedResponseHidden) {
  MockProcessResponseHeadersCallback mock_process_response_headers_callback;
  const GURL response_url("https://request_url.com/image.png");
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  auto classification_result = mojom::FilterMatchResult::kBlockRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Image))
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(response_url, _))
      .Times(1);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckResponseFilterMatch(_, response_url,
                                       main_rfh()->GetProcess()->GetID(),
                                       main_rfh()->GetRoutingID(), _, _))
      .WillOnce([&](auto, const GURL&, int32_t process_id,
                    int32_t render_frame_id,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, main_rfh()->GetRoutingID(), headers, "",
      mock_process_response_headers_callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_ResponseNotHideable) {
  MockProcessResponseHeadersCallback mock_process_response_headers_callback;
  const GURL response_url("https://request_url.com/script.js");
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  auto classification_result = mojom::FilterMatchResult::kBlockRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Script))
      .Times(1)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(response_url, _))
      .Times(0);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckResponseFilterMatch(_, response_url,
                                       main_rfh()->GetProcess()->GetID(),
                                       main_rfh()->GetRoutingID(), _, _))
      .WillOnce([&](auto, const GURL&, int32_t process_id,
                    int32_t render_frame_id,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, main_rfh()->GetRoutingID(), headers, "",
      mock_process_response_headers_callback.Get());

  task_environment()->RunUntilIdle();
}

TEST_F(AdblockMojoInterfaceImplTest, ElementHiding_AllowedResponseNotHidden) {
  MockProcessResponseHeadersCallback mock_process_response_headers_callback;
  const GURL response_url("https://request_url.com/image.png");
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  auto classification_result = mojom::FilterMatchResult::kAllowRule;

  InitializeSubscriptionService();
  EXPECT_CALL(*mock_element_hider_, IsElementTypeHideable(ContentType::Image))
      .Times(0);
  EXPECT_CALL(*mock_element_hider_, HideBlockedElement(response_url, _))
      .Times(0);

  EXPECT_CALL(*mock_resource_classification_runner_,
              CheckResponseFilterMatch(_, response_url,
                                       main_rfh()->GetProcess()->GetID(),
                                       main_rfh()->GetRoutingID(), _, _))
      .WillOnce([&](auto, const GURL&, int32_t process_id,
                    int32_t render_frame_id,
                    const scoped_refptr<net::HttpResponseHeaders>&,
                    mojom::AdblockInterface::CheckFilterMatchCallback cb) {
        std::move(cb).Run(classification_result);
      });

  auto adblock_mojo_interface = std::make_unique<AdblockMojoInterfaceImpl>(
      main_rfh()->GetProcess()->GetID());

  adblock_mojo_interface->ProcessResponseHeaders(
      response_url, main_rfh()->GetRoutingID(), headers, "",
      mock_process_response_headers_callback.Get());

  task_environment()->RunUntilIdle();
}

}  // namespace adblock

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

#include "components/adblock/content/browser/content_security_policy_injector_impl.h"

#include <memory>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/test/mock_callback.h"
#include "components/adblock/content/browser/test/mock_frame_hierarchy_builder.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "components/adblock/core/subscription/test/mock_subscription_collection.h"
#include "components/adblock/core/subscription/test/mock_subscription_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/content_security_policy.mojom-forward.h"
#include "services/network/public/mojom/parsed_headers.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

using testing::_;
using testing::Return;
using MockResponseCallback =
    base::MockCallback<InsertContentSecurityPolicyHeadersCallback>;

class AdblockContentSecurityPolicyInjectorImplTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    auto frame_hierarchy_builder =
        std::make_unique<MockFrameHierarchyBuilder>();
    frame_hierarchy_builder_ = frame_hierarchy_builder.get();
    csp_injector_ = std::make_unique<ContentSecurityPolicyInjectorImpl>(
        &subscription_service_, std::move(frame_hierarchy_builder));
  }

  void TearDown() override {
    // Avoid dangling pointers during destruction.
    frame_hierarchy_builder_ = nullptr;
    content::RenderViewHostTestHarness::TearDown();
  }

  void FrameHierarchyWillBeBuilt() {
    EXPECT_CALL(*frame_hierarchy_builder_,
                FindRenderFrameHost(main_rfh()->GetGlobalId()))
        .WillOnce(testing::Return(main_rfh()));
    EXPECT_CALL(*frame_hierarchy_builder_, BuildFrameHierarchy(main_rfh()))
        .WillOnce(Return(kFrameHierarchy));
  }

  void SnapshotWillContainInjections(std::set<std::string_view> injections) {
    FrameHierarchyWillBeBuilt();
    // SubscriptionService will be asked for the current snapshot...
    EXPECT_CALL(subscription_service_, GetCurrentSnapshot())
        .WillOnce([this, injections]() {
          // ... and testee will query the snapshot for CSP injection:
          auto subscription_collection =
              std::make_unique<MockSubscriptionCollection>();
          EXPECT_CALL(*subscription_collection,
                      GetCspInjections(kRequestUrl, kFrameHierarchy))
              .WillOnce(Return(injections));
          SubscriptionService::Snapshot snapshot;
          snapshot.push_back(std::move(subscription_collection));
          return snapshot;
        });
  }

  void AssertParsedHeadersContainScriptSrcNone(
      const network::mojom::ParsedHeadersPtr& parsed_headers) {
    ASSERT_EQ(parsed_headers->content_security_policy.size(), 1u);
    EXPECT_EQ(parsed_headers->content_security_policy[0]
                  ->raw_directives[network::mojom::CSPDirectiveName::ScriptSrc],
              "'none'");
  }

  const GURL kRequestUrl{"https://request.com/resource.txt"};
  const std::vector<GURL> kFrameHierarchy{GURL{"https://test.com/"}};
  MockSubscriptionService subscription_service_;
  raw_ptr<MockFrameHierarchyBuilder> frame_hierarchy_builder_;
  std::unique_ptr<ContentSecurityPolicyInjectorImpl> csp_injector_;
};

TEST_F(AdblockContentSecurityPolicyInjectorImplTest,
       NoHeaderAddedWhenNoCspInjectionFound) {
  // An empty injection set means no CSP filter found.
  SnapshotWillContainInjections({});

  MockResponseCallback callback;

  // Call testee:
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  CHECK(headers);
  csp_injector_->InsertContentSecurityPolicyHeadersIfApplicable(
      kRequestUrl, main_rfh()->GetGlobalId(), headers, callback.Get());

  // Callback is ran via posted task, with no parsed headers because headers
  // were not modified.
  EXPECT_CALL(callback, Run(testing::IsFalse())).Times(1);
  task_environment()->RunUntilIdle();

  // No header was injected.
  EXPECT_FALSE(headers->HasHeader("Content-Security-Policy"));
}

TEST_F(AdblockContentSecurityPolicyInjectorImplTest,
       HeaderAddedWhenCspInjectionFound) {
  SnapshotWillContainInjections({"script-src 'none'"});

  MockResponseCallback callback;

  // Call testee:
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  CHECK(headers);
  csp_injector_->InsertContentSecurityPolicyHeadersIfApplicable(
      kRequestUrl, main_rfh()->GetGlobalId(), headers, callback.Get());

  // Callback is ran via posted task with correctly parsed headers.
  EXPECT_CALL(callback, Run(_))
      .WillOnce([&](network::mojom::ParsedHeadersPtr parsed_headers) {
        AssertParsedHeadersContainScriptSrcNone(parsed_headers);
      });
  task_environment()->RunUntilIdle();

  // The header was injected.
  EXPECT_TRUE(
      headers->HasHeaderValue("Content-Security-Policy", "script-src 'none'"));
}

TEST_F(AdblockContentSecurityPolicyInjectorImplTest,
       HeadersAddedWhenMultipleCspInjectionFound) {
  SnapshotWillContainInjections({"script-src 'first'", "script-src 'second'"});

  MockResponseCallback callback;

  // Call testee:
  auto headers = net::HttpResponseHeaders::TryToCreate("");
  CHECK(headers);
  csp_injector_->InsertContentSecurityPolicyHeadersIfApplicable(
      kRequestUrl, main_rfh()->GetGlobalId(), headers, callback.Get());

  // Callback is ran via posted task with correctly parsed headers.
  EXPECT_CALL(callback, Run(_))
      .WillOnce([&](network::mojom::ParsedHeadersPtr parsed_headers) {
        ASSERT_EQ(parsed_headers->content_security_policy.size(), 2u);
        EXPECT_EQ(
            parsed_headers->content_security_policy[0]
                ->raw_directives[network::mojom::CSPDirectiveName::ScriptSrc],
            "'first'");
        EXPECT_EQ(
            parsed_headers->content_security_policy[1]
                ->raw_directives[network::mojom::CSPDirectiveName::ScriptSrc],
            "'second'");
      });
  task_environment()->RunUntilIdle();

  // The header was injected.
  EXPECT_TRUE(
      headers->HasHeaderValue("Content-Security-Policy", "script-src 'first'"));
  EXPECT_TRUE(headers->HasHeaderValue("Content-Security-Policy",
                                      "script-src 'second'"));
}

}  // namespace adblock

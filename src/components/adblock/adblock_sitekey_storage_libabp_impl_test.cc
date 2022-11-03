/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_sitekey_storage_libabp_impl.h"

#include "base/test/mock_callback.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/mock_adblock_platform_embedder.h"
#include "components/adblock/mock_filter_engine.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

constexpr char kTestURL[] = "http://example.com";
constexpr char kTestUA[] = "test UA";

class AdblockSitekeyStorageTest : public content::RenderViewHostTestHarness {
 public:
  AdblockSitekeyStorageTest() = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    std::unique_ptr<MockFilterEngine> mock_engine =
        std::make_unique<MockFilterEngine>();
    mock_engine_ = mock_engine.get();
    embedder_ = new MockAdblockPlatformEmbedder(
        std::move(mock_engine), task_environment()->GetMainThreadTaskRunner());
    storage_ = std::make_unique<AdblockSitekeyStorageLibabpImpl>(embedder_);
  }

  MockFilterEngine* mock_engine_;
  scoped_refptr<MockAdblockPlatformEmbedder> embedder_;
  std::unique_ptr<AdblockSitekeyStorageLibabpImpl> storage_;
};

TEST_F(AdblockSitekeyStorageTest, SiteKeyInvalid) {
  base::RunLoop loop;
  EXPECT_CALL(*mock_engine_, VerifySignature(testing::_, testing::_, testing::_,
                                             testing::_, testing::_))
      .Times(0);
  auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  headers->SetHeader(adblock::kSiteKeyHeaderKey, "invalid");
  storage_->ProcessResponseHeaders(GURL(kTestURL), headers, "test UA",
                                   base::BindOnce(loop.QuitClosure()));
  loop.Run();
}

TEST_F(AdblockSitekeyStorageTest, SiteKeyWithoutAndWithFilterEngine) {
  // without filter engine
  std::unique_ptr<MockFilterEngine> engine = embedder_->TakeFilterEngine();
  embedder_->SetFilterEngine(nullptr);

  GURL url = GURL{kTestURL};
  base::MockOnceClosure process_site_key_callback;

  {
    base::RunLoop loop;
    EXPECT_CALL(*mock_engine_,
                VerifySignature(testing::_, testing::_, testing::_, testing::_,
                                testing::_))
        .Times(0);
    EXPECT_CALL(process_site_key_callback, Run()).Times(0);

    auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
    headers->SetHeader(adblock::kSiteKeyHeaderKey, "key_sig");
    storage_->ProcessResponseHeaders(url, headers, kTestUA,
                                     process_site_key_callback.Get());

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    embedder_->SetFilterEngine(std::move(engine));

    EXPECT_CALL(*mock_engine_,
                VerifySignature("key", "sig", url.path(), url.host(), kTestUA))
        .Times(1)
        .WillOnce(testing::Return(false));
    EXPECT_CALL(process_site_key_callback, Run()).Times(1);

    embedder_->NotifyOnEngineCreated();

    loop.RunUntilIdle();
  }
}

}  // namespace adblock

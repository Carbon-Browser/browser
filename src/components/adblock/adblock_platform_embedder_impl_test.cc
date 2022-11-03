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

#include "components/adblock/adblock_platform_embedder_impl.h"

#include "base/memory/scoped_refptr.h"
#include "base/test/mock_callback.h"
#include "components/adblock/mock_filter_engine.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class MockElement : public AdblockPlus::IElement {
 public:
  std::string GetLocalName() const final { return "mock"; }
  std::string GetAttribute(const std::string& name) const final {
    return "mock";
  }
  std::string GetDocumentLocation() const final { return "mock"; }
  std::vector<const IElement*> GetChildren() const final { return {}; }
};

class AdblockPlatformEmbedderTest : public content::RenderViewHostTestHarness {
 public:
  AdblockPlatformEmbedderTest() = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    embedder_ = new AdblockPlatformEmbedderImpl(
        base::MakeRefCounted<utils::TaskRunnerWrapper>(
            task_environment()->GetMainThreadTaskRunner()),
        task_environment()->GetMainThreadTaskRunner(), {}, "", {}, nullptr);
  }

  scoped_refptr<AdblockPlatformEmbedderImpl> embedder_;
};

TEST_F(AdblockPlatformEmbedderTest,
       ComposeFilterSuggestionsWithoutAndWithFilterEngine) {
  // without filter engine
  MockFilterEngine mock_engine;
  embedder_->SetFilterEngine(nullptr);

  base::MockOnceCallback<void(const std::vector<std::string>&)> result_callback;

  {
    base::RunLoop loop;
    EXPECT_CALL(mock_engine, ComposeFilterSuggestions(testing::_)).Times(0);
    EXPECT_CALL(result_callback, Run(testing::_)).Times(0);
    auto element = std::make_unique<MockElement>();
    embedder_->ComposeFilterSuggestions(std::move(element),
                                        result_callback.Get());

    loop.RunUntilIdle();
  }

  // with filter engine
  {
    base::RunLoop loop;

    std::vector<std::string> suggestions{"test"};
    embedder_->SetFilterEngine(&mock_engine);
    auto element = std::make_unique<MockElement>();
    EXPECT_CALL(mock_engine, ComposeFilterSuggestions(element.get()))
        .Times(1)
        .WillOnce(testing::Return(suggestions));
    EXPECT_CALL(result_callback, Run(suggestions)).Times(1);

    embedder_->ComposeFilterSuggestions(std::move(element),
                                        result_callback.Get());

    loop.RunUntilIdle();
  }
}

}  // namespace adblock

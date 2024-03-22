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

#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/adblock/core/features.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class AdblockMlApiTest : public InProcessBrowserTest {
 protected:
  base::Value Execute(std::string js) {
    base::RunLoop loop;
    base::Value result;
    browser()
        ->tab_strip_model()
        ->GetActiveWebContents()
        ->GetPrimaryMainFrame()
        ->ExecuteJavaScriptInIsolatedWorld(
            base::UTF8ToUTF16(js),
            base::BindOnce(
                [](base::RunLoop* loop, base::Value* result, base::Value arg) {
                  *result = std::move(arg);
                  loop->Quit();
                },
                &loop, &result),
            content::ISOLATED_WORLD_ID_ADBLOCK);
    loop.Run();
    return result;
  }

  void SetUpInProcessBrowserTestFixture() override {
    scoped_feature_list_.InitAndEnableFeature(adblock::kEyeoMlServiceFeature);
  }

  void SetUp() override {
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
    InProcessBrowserTest::SetUp();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdblockMlApiTest, BasicMessaging) {
  Execute(
      R"(
      chrome.runtime.sendMessage(
        "phhdcbipnceblbigdhhoahagpfdblied", {type: "ML:test"}, result => {
          if (result.type === "ML:test" && result.error === 2) { document.title = 'success'; }
          else { console.log(JSON.stringify(result)); document.title = 'failure'; }
        });
      )");
  content::TitleWatcher title_watcher(
      browser()->tab_strip_model()->GetActiveWebContents(), u"success");
  title_watcher.AlsoWaitForTitle(u"failure");
  std::u16string result = title_watcher.WaitAndGetTitle();
  EXPECT_EQ(u"success", result);
}

// This file is part of Adblock Plus <https://adblockplus.org/>,
// Copyright (C) 2006-present eyeo GmbH
//
// Adblock Plus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// Adblock Plus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.

#include <memory>

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/extensions/api/adblock_private.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Mock;
using testing::Return;

namespace extensions {

namespace {

class AdblockPrivateApiTest : public ExtensionApiTest {
 public:
  AdblockPrivateApiTest() {}
  ~AdblockPrivateApiTest() override {}

 protected:
  bool RunTest(const std::string& subtest) {
    const std::string page_url = "main.html?" + subtest;
    return RunExtensionTest("adblock_private", {.page_url = page_url.c_str()},
                            {.load_as_component = true});
  }

  DISALLOW_COPY_AND_ASSIGN(AdblockPrivateApiTest);
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
  EXPECT_TRUE(RunTest("builtInSubscriptionsManagement")) << message_;
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

// Test currently fails, see DPD-848.
IN_PROC_BROWSER_TEST_F(AdblockPrivateApiTest, DISABLED_Events) {
  EXPECT_TRUE(RunTest("events")) << message_;
}

}  // namespace extensions

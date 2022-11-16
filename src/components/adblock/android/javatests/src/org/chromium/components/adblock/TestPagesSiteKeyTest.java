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

package org.chromium.components.adblock;

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesSiteKeyTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifySitekeyException() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##.testcase-sitekey-eh");
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/sitekey/outofframe.png");
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/sitekey/inframe.png");
        mHelper.addCustomFilter(
                "@@$document,sitekey=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANGtTstne7e8MbmDHDiMFkGbcuBgXmiVesGOG3gtYeM1EkrzVhBjGUvKXYE4GLFwqty3v5MuWWbvItUWBTYoVVsCAwEAAQ");
        // DP testpages does not work.
        final String TEST_URL = "https://testpages.adblockplus.org/exceptions/sitekey";
        mHelper.loadUrl(TEST_URL);
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertTrue(mHelper.isBlocked(
                "https://testpages.adblockplus.org/testfiles/sitekey/outofframe.png"));
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(mHelper.isAllowed(
                "https://testpages.adblockplus.org/testfiles/sitekey/inframe.png"));
        mHelper.verifyHiddenCount(1, "img");
        mHelper.verifyHiddenCount(1, "div");
    }
}

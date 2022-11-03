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

package org.chromium.chrome.browser.adblock;

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.adblock.TestPagesTestsHelper;
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
        mHelper.setUp(mActivityTestRule, TestPagesTestsHelper.TESTPAGE_SUBSCRIPTION);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifySitekeyException() throws TimeoutException, InterruptedException {
        // DP testpages does not work.
        final String TEST_URL = "https://testpages.adblockplus.org/exceptions/sitekey";
        mHelper.loadUrl(TEST_URL);
        Assert.assertEquals(3, mHelper.numBlocked());
        Assert.assertTrue(mHelper.isBlocked(
                "https://testpages.adblockplus.org/testfiles/sitekey/outofframe.png"));
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertTrue(mHelper.isAllowed(
                "https://testpages.adblockplus.org/testfiles/sitekey/inframe.png"));
        mHelper.verifyHiddenCount(1, "img");
        mHelper.verifyHiddenCount(1, "div");
    }
}

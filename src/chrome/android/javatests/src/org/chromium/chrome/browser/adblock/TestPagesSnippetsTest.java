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
public class TestPagesSnippetsTest {
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
    public void testAbortCurrentInlineScript() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "abort-current-inline-script");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyRead() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "abort-on-property-read");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWrite() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "abort-on-property-write");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyRead() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "abort-on-iframe-property-read");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWrite() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "abort-on-iframe-property-write");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testDirString() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "dir-string");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContains() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(0, "span[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(2, "p[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsAndMatchesStyle() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-contains-and-matches-style");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(2, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsImage() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-contains-image");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsImageHash() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-contains-image-hash");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(2, "img[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsVisibleText() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-contains-visible-text");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfHasAndMatchesStyle() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-has-and-matches-style");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfLabeledBy() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-labelled-by");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPath() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-matches-xpath");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(3, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfShadowContains() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "hide-if-shadow-contains");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testStripFetchQueryParameter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.SNIPPETS_COMMON_UNIT_TESTCASES_ROOT
                + "strip-fetch-query-parameter");
        mHelper.verifyDisplayedCount(0, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(3, "div[data-expectedresult='pass']");
    }
}

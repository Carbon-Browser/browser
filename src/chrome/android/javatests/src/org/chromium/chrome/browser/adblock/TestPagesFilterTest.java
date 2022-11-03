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
import org.chromium.chrome.browser.adblock.AdblockController;
import org.chromium.chrome.browser.adblock.TestPagesTestsHelper;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;

import java.lang.Thread;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesFilterTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    @Before
    public void setUp() {
        mHelper.setUp(
                mActivityTestRule, TestPagesTestsHelper.DISTRIBUTION_UNIT_TESTPAGE_SUBSCRIPTION);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "element-hiding");
        mHelper.verifyHiddenCount(11, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideEmuFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT
                + "element-hiding-emulation");
        mHelper.verifyHiddenCount(9, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testBlockingFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "blocking");
        Assert.assertEquals(5, mHelper.numBlocked());
        Assert.assertEquals(5, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        mHelper.verifyHiddenCount(5, "img[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyScriptFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "script");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_SCRIPT));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "script/script.js"));

        String childCount =
                JavaScriptUtils.executeJavaScriptAndWaitForResult(mHelper.getWebContents(),
                        "document.getElementById(\"script-target\").childElementCount");
        Assert.assertEquals("1", childCount);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyImageFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "image");
        Assert.assertEquals(2, mHelper.numBlocked());
        Assert.assertEquals(2, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "image/static/static.png"));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "image/dynamic/dynamic.png"));
        mHelper.verifyHiddenCount(2, "img[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyStylesheetFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "stylesheet");

        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(
                1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_STYLESHEET));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "stylesheet/stylesheet.css"));
        String value = DOMUtils.getNodeContents(mHelper.getWebContents(), "stylesheet-target");
        Assert.assertEquals("Passed. Stylesheet was blocked.", value);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyPopupFilters() throws TimeoutException, InterruptedException {
        final String POPUP_TESTACE_URL =
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "popup";
        mHelper.loadUrl(POPUP_TESTACE_URL);
        Assert.assertEquals(1, mHelper.getTabCount());
        String numElements =
                JavaScriptUtils.executeJavaScriptAndWaitForResult(mHelper.getWebContents(),
                        "var elements = document.getElementsByClassName(\"testcase-trigger\");"
                                + "for (let i = 0; i < elements.length; ++i) {"
                                + "    elements[i].click();"
                                + "}"
                                + "elements.length;");
        Assert.assertEquals("3", numElements);
        Assert.assertEquals(3, mHelper.numBlocked());
        Assert.assertEquals(3, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_POPUP));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "popup/link.html"));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "popup/script-window.html"));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "popup/script-tab.html"));
        Assert.assertEquals(1, mHelper.getTabCount());
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyXHRFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "xmlhttprequest");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(
                1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_XMLHTTPREQUEST));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "xmlhttprequest/text.txt"));
        String value = DOMUtils.getNodeContents(mHelper.getWebContents(), "testcase-status");
        Assert.assertEquals("Passed. Connection was blocked.", value);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifySubdocumentFilters() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "subdocument");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(
                1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_SUBDOCUMENT));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "subdocument/subdocument.html"));
        mHelper.verifyHiddenCount(1, "iframe[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyRewrite() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "rewrite");
        Assert.assertEquals(3, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_SCRIPT));
        Assert.assertEquals(2, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_MEDIA));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "rewrite/audio.mp3"));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "rewrite/video.mp4"));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "rewrite/script.js"));
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyMatchCaseFilter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "match-case");
        Assert.assertEquals(2, mHelper.numBlocked());
        Assert.assertEquals(2, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        mHelper.verifyHiddenCount(2, "img[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyThirdPartyFilter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(
                TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "third-party");
        Assert.assertEquals(2, mHelper.numBlocked());
        Assert.assertEquals(2, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        mHelper.verifyHiddenCount(2, "img[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(2, "img[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyOtherFilter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "other");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_OTHER));
        Assert.assertTrue(mHelper.isBlocked(
                TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT + "other/image.png"));
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyDomainFilter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "domain");
        Assert.assertEquals(2, mHelper.numBlocked());
        Assert.assertEquals(2, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "domain/static/target/image.png"));
        Assert.assertTrue(mHelper.isBlocked(TestPagesTestsHelper.DISTRIBUTION_UNIT_RESOURCES_ROOT
                + "domain/dynamic/image.png"));
        mHelper.verifyHiddenCount(2, "img[data-expectedresult='fail']");
    }
}

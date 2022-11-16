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
import org.chromium.components.adblock.TestPagesTestsHelper.IncludeSubframes;

import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesHeaderFilterTest {
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
    public void testHeaderFilterScript() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "||testpages.adblockplus.org/testfiles/header/$header=content-type=application/javascript");
        mHelper.loadUrl("https://testpages.adblockplus.org/en/filters/header");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_SCRIPT));
        Assert.assertTrue(
                mHelper.isBlocked("https://testpages.adblockplus.org/testfiles/header/script.js"));
        mHelper.verifyDisplayedCount(
                0, "div#functionproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHeaderFilterImage() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "||testpages.adblockplus.org/testfiles/header/$header=content-type=image/png");
        mHelper.loadUrl("https://testpages.adblockplus.org/en/filters/header");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        Assert.assertTrue(
                mHelper.isBlocked("https://testpages.adblockplus.org/testfiles/header/image.png"));
        mHelper.verifyDisplayedCount(0, "img[id='image-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHeaderFilterStylesheet() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "||testpages.adblockplus.org/testfiles/header/$header=content-type=text/css");
        mHelper.loadUrl("https://testpages.adblockplus.org/en/filters/header");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(
                1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_STYLESHEET));
        Assert.assertTrue(mHelper.isBlocked(
                "https://testpages.adblockplus.org/testfiles/header/stylesheet.css"));
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHeaderFilterException() throws TimeoutException, InterruptedException {
        // TODO: uncomment this part of test after DPD-1257.
        // Add blocking filter, expect blocked image
        // mHelper.addCustomFilter(
        //         "||testpages.adblockplus.org/testfiles/header_exception/$header=content-type=image/png");
        // mHelper.loadUrl("https://testpages.adblockplus.org/en/exceptions/header");
        // Assert.assertEquals(1, mHelper.numBlocked());
        // Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        // mHelper.verifyDisplayedCount(0, "img[id='image-header-exception-pass-1']");

        // Add exception filter, expect image allowed
        mHelper.addCustomFilter(
                "||testpages.adblockplus.org/testfiles/header_exception/$header=content-type=image/png");
        mHelper.addCustomFilter("@@testpages.adblockplus.org/testfiles/header_exception/$header");
        mHelper.loadUrl("https://testpages.adblockplus.org/en/exceptions/header");
        Assert.assertEquals(1, mHelper.numAllowed());
        Assert.assertEquals(1, mHelper.numAllowedByType(AdblockContentType.CONTENT_TYPE_IMAGE));
        Assert.assertTrue(mHelper.isAllowed(
                "https://testpages.adblockplus.org/testfiles/header_exception/image.png"));
        mHelper.verifyDisplayedCount(1, "img[id='image-header-exception-pass-1']");
    }
}

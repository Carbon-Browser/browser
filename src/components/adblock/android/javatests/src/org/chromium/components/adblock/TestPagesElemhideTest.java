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
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesElemhideTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    public static final String ELEMENT_HIDING_TESTPAGES_URL =
            "https://testpages.adblockplus.org/en/filters/element-hiding";
    public static final String ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL =
            "https://testpages.adblockplus.org/en/exceptions/elemhide";

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
    public void testElemHideFiltersIdSelector() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org###eh-id");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='eh-id']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersIdSelectorDoubleCurlyBraces()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[id='{{eh-id}}']");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='{{eh-id}}']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersClassSelector() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##.eh-class");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[class='eh-class']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersDescendantSelector()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##.testcase-area > .eh-descendant");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[class='eh-descendant']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersSiblingSelector() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org##.testcase-examplecontent + .eh-sibling");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[class='eh-sibling']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector1()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[height=\"100\"][width=\"100\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='attribute-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector2()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org##div[href=\"http://testcase-attribute.com/\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='attribute-selector-2-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersAttributeSelector3()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[style=\"width: 200px;\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='attribute-selector-3-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersStartsWithSelector1()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org##div[href^=\"http://testcase-startswith.com/\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='starts-with-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersStartsWithSelector2()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[style^=\"width: 201px;\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='starts-with-selector-2-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersEndsWithSelector1()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[style$=\"width: 202px;\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='ends-with-selector-1-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersContains() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##div[style*=\"width: 203px;\"]");
        mHelper.loadUrl(ELEMENT_HIDING_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "div[id='contains-fail-1']");
    }

    // Exceptions:
    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersBasicException() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##.ex-elemhide");
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/elemhide/basic/*");
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        // No exceptions added yet, both objects should be blocked.
        mHelper.verifyHiddenCount(1, "img[id='basic-usage-fail-1']");
        mHelper.verifyHiddenCount(1, "div[id='basic-usage-pass-1']");
        // Add exception filter and reload.
        mHelper.addCustomFilter("@@testpages.adblockplus.org/en/exceptions/elemhide$elemhide");
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        // Image should remain blocked, div should be unblocked.
        mHelper.verifyHiddenCount(1, "img[id='basic-usage-fail-1']");
        mHelper.verifyDisplayedCount(
                1, "div[id='basic-usage-area'] > div[id='basic-usage-pass-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testElemHideFiltersIframeException() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org##.targ-elemhide");
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/elemhide/iframe/*.png");
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "img[id='iframe-fail-1']");
        mHelper.verifyHiddenCount(1, "div[id='iframe-pass-1']");

        // Add exception filter and reload.
        mHelper.addCustomFilter("@@testpages.adblockplus.org/en/exceptions/elemhide$elemhide");
        mHelper.loadUrl(ELEMENT_HIDING_EXCEPTIONS_TESTPAGES_URL);
        mHelper.verifyHiddenCount(1, "img[id='iframe-fail-1']");
        mHelper.verifyDisplayedCount(1, "div[id='iframe-pass-1']");
    }
}

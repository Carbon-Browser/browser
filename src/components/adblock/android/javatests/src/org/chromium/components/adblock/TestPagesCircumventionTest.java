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
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;

import java.lang.Thread;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesCircumventionTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
        mHelper.addFilterList(TestPagesTestsHelper.DISTRIBUTION_UNIT_TESTPAGE_SUBSCRIPTION);
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyCircumventionInlineStyleNotImportant()
            throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.CIRCUMVENTION_DISTRIBUTION_UNIT_TESTCASES_ROOT
                + "inline-style-important");
        mHelper.verifyHiddenCount(2, "div");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyCircumventionAnonymousFrameDocumentWrite()
            throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.CIRCUMVENTION_DISTRIBUTION_UNIT_TESTCASES_ROOT
                + "anoniframe-documentwrite");
        mHelper.verifyHiddenCount(1, "span[data-expectedresult='fail']");
        String numHidden = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                mHelper.getWebContents(),
                "var hiddenCount = 0;"
                        + "var elements = document.getElementById(\"write\").contentWindow.document.getElementsByTagName(\"span\");"
                        + "for (let i = 0; i < elements.length; ++i) {"
                        + "    if (window.getComputedStyle(elements[i]).display == \"none\") ++hiddenCount;"
                        + "}"
                        + "hiddenCount;");
        Assert.assertEquals("1", numHidden);
    }
}

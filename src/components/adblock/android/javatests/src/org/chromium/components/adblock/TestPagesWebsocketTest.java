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

import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesWebsocketTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    @Before
    public void setUp() {
        mHelper.setUp(mActivityTestRule);
        mHelper.addCustomFilter(
                "$websocket,domain=dp-testpages.adblockplus.org");
    }

    @After
    public void tearDown() {
        mHelper.tearDown();
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testVerifyWebsocketFilter() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "websocket");
        Assert.assertEquals(1, mHelper.numBlocked());
        Assert.assertEquals(1, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_WEBSOCKET));
        Assert.assertTrue(mHelper.isBlocked("wss://echo.websocket.org"));
    }

    @Test
    @LargeTest
    @CommandLineFlags.Add({"disable-adblock"})
    @Feature({"adblock"})
    public void testVerifyWebsocketFilterWhenAbpDisabled() throws TimeoutException, InterruptedException {
        mHelper.loadUrl(TestPagesTestsHelper.FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT + "websocket");
        Assert.assertEquals(0, mHelper.numBlocked());
        Assert.assertEquals(0, mHelper.numBlockedByType(AdblockContentType.CONTENT_TYPE_WEBSOCKET));
        Assert.assertFalse(mHelper.isBlocked("wss://echo.websocket.org"));
    }
}

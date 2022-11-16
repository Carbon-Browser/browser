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
import org.chromium.content_public.browser.test.util.JavaScriptUtils;

import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TestPagesRewriteTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    public static final String REWRITE_TEST_URL =
       "https://testpages.adblockplus.org/en/filters/rewrite";

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
    public void testRewriteScript() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.js$rewrite=abp-resource:blank-js,domain=testpages.adblockplus.org");
        mHelper.loadUrl(REWRITE_TEST_URL);
        mHelper.verifyDisplayedCount(0, "div[id='script-fail-1']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteStylesheet() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.css$rewrite=abp-resource:blank-css,domain=testpages.adblockplus.org");
        mHelper.loadUrl(REWRITE_TEST_URL);
        mHelper.verifyGreenBackground("stylesheet-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteSubdocument() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.html$rewrite=abp-resource:blank-html,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("subdocument-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteText() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.txt$rewrite=abp-resource:blank-text,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("text-status");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteGif() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/1x1.gif$rewrite=abp-resource:1x1-transparent-gif,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("1x1-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite2x2Png() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/2x2.png$rewrite=abp-resource:2x2-transparent-png,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("2x2-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite3x2Png() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/3x2.png$rewrite=abp-resource:3x2-transparent-png,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("3x2-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewrite32x32Png() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/32x32.png$rewrite=abp-resource:32x32-transparent-png,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      mHelper.verifySelfTestPass("32x32-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteAudio() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.mp3$rewrite=abp-resource:blank-mp3,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      String value = JavaScriptUtils.executeJavaScriptAndWaitForResult(
          mActivityTestRule.getActivity().getActivityTab().getWebContents(),
          "document.getElementById('audio-area').lastChild.getAttribute('data-expectedresult')");
      Assert.assertEquals("\"pass\"", value);
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testRewriteVideo() throws TimeoutException, InterruptedException {
      mHelper.addCustomFilter("||testpages.adblockplus.org/testfiles/rewrite/*.mp4$rewrite=abp-resource:blank-mp4,domain=testpages.adblockplus.org");
      mHelper.loadUrl(REWRITE_TEST_URL);
      String value = JavaScriptUtils.executeJavaScriptAndWaitForResult(
          mActivityTestRule.getActivity().getActivityTab().getWebContents(),
          "document.getElementById('video-area').lastChild.getAttribute('data-expectedresult')");
      Assert.assertEquals("\"pass\"", value);
    }
}

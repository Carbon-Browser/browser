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
public class TestPagesSnippetsTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private TestPagesTestsHelper mHelper = new TestPagesTestsHelper();

    public static final String SNIPPETS_COMMON_UNIT_TESTCASES_ROOT =
            "https://testpages.adblockplus.org/en/snippets/";

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
    public void testAbortCurrentInlineScriptBasic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-current-inline-script console.group");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-current-inline-script");
        // All "Abort" snippets cancel creation of the target div, so it won't be hidden - it will
        // not exist in DOM. Therefore we verify it's not displayed instead of verifying it's
        // hidden.
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortCurrentInlineScriptSearch() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-current-inline-script console.info acis-search");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-current-inline-script");
        mHelper.verifyDisplayedCount(0, "div#search-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortCurrentInlineScriptRegex() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-current-inline-script console.warn '/acis-regex[1-2]/'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-current-inline-script");
        mHelper.verifyDisplayedCount(0, "div#regex-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadBasic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-property-read aoprb");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-read");
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadSubProperty() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-property-read aopr.sp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-read");
        mHelper.verifyDisplayedCount(0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyReadFunctionProperty()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-property-read aoprf.fp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-read");
        mHelper.verifyDisplayedCount(
                0, "div#functionproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteBasic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-property-write window.aopwb");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-write");
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteSubProperty()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-property-write window.aopwsp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-write");
        mHelper.verifyDisplayedCount(0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnPropertyWriteFunctionProperty()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-property-write aopwf.fp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-property-write");
        mHelper.verifyDisplayedCount(
                0, "div#functionproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadBasic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter("testpages.adblockplus.org#$#abort-on-iframe-property-read aoiprb");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-read");
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadSubProperty()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-iframe-property-read aoipr.sp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-read");
        mHelper.verifyDisplayedCount(0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyReadMultipleProperties()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-iframe-property-read aoipr1 aoipr2");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-read");
        mHelper.verifyDisplayedCount(
                0, "div#multipleproperties-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteBasic()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-iframe-property-write aoipwb");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-write");
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteSubProperty()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-iframe-property-write aoipw.sp");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-write");
        mHelper.verifyDisplayedCount(0, "div#subproperty-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testAbortOnIframePropertyWriteMultipleProperties()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#abort-on-iframe-property-write aoipw1 aoipw2");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "abort-on-iframe-property-write");
        mHelper.verifyDisplayedCount(
                0, "div#multipleproperties-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsStatic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains 'hic-basic-static' p[id]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        mHelper.verifyHiddenCount(1, "p#hic-static-id");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsDynamic() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains 'hic-basic-dynamic' p[id]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        mHelper.verifyHiddenCount(1, "p#hic-dynamic-id");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsSearch() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains 'hic-search' p[id] .target");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        mHelper.verifyHiddenCount(1, "div#search2-target > p.target");
        mHelper.verifyDisplayedCount(1, "div#search1-target > p[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsRegex() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains /hic-regex-[2-3]/ p[id]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        // "hic-regex-1" does not match regex, should remain displayed.
        mHelper.verifyDisplayedCount(1, "p#hic-regex-1");

        // "hic-regex-2" and "hic-regex-2" do match regex, should be hidden.
        mHelper.verifyHiddenCount(1, "p#hic-regex-2");
        mHelper.verifyHiddenCount(1, "p#hic-regex-3");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsFrame() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains hidden span#frame-target");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains");
        mHelper.verifyHiddenCount(1, "span#frame-target");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsAndMatchesStyleStatic()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains-and-matches-style hicamss div[id] span.label /./ 'display: inline;'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains-and-matches-style");
        mHelper.verifyHiddenCount(1, "div#static-usage-area > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div#static-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsAndMatchesStyleDynamic()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains-and-matches-style hicamsd div[id] span.label /./ 'display: inline;'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains-and-matches-style");
        mHelper.verifyHiddenCount(1, "div#dynamic-target > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div#dynamic-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsImage() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains-image /^89504e470d0a1a0a0000000d4948445200000064000000640802/ div[shouldhide] div");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains-image");
        mHelper.verifyHiddenCount(2, "div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsVisibleTextBasicUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains-visible-text Sponsored-hicvt-basic '#parent-basic > .article' '#parent-basic > .article .label'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains-visible-text");
        mHelper.verifyHiddenCount(1, "div#parent-basic > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfContainsVisibleTextContentUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-contains-visible-text Sponsored-hicvt-content '#parent-content > .article' '#parent-content > .article .label'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-contains-visible-text");
        mHelper.verifyHiddenCount(1, "div#parent-content > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfHasAndMatchesStyleBasicUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-has-and-matches-style a[href=\"#basic-target-ad\"] div[id] span.label /./ 'display: inline;'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-has-and-matches-style");
        mHelper.verifyHiddenCount(1, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfHasAndMatchesStyleLegitElements()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-has-and-matches-style a[href=\"#comments-target-ad\"] div[id] span.label ';' /\\\\bdisplay:\\ inline\\;/");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-has-and-matches-style");
        mHelper.verifyDisplayedCount(1, "div#comments-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfLabeledBy() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-labelled-by 'Label' '#hilb-target [aria-labelledby]' '#hilb-target'");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-labelled-by");
        mHelper.verifyHiddenCount(1, "div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathBasicStaticUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-matches-xpath //*[@id=\"isnfnv\"]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-matches-xpath");
        mHelper.verifyHiddenCount(
                1, "div#basic-static-usage-area > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(
                1, "div#basic-static-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathClassUsage() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-matches-xpath //*[@class=\"to-be-hidden\"]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-matches-xpath");
        mHelper.verifyHiddenCount(1, "div#class-usage-area > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div#class-usage-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfMatchesXPathIdStartsWith() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-matches-xpath //div[starts-with(@id,\"fail\")]");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-matches-xpath");
        mHelper.verifyHiddenCount(
                1, "div#hide-if-id-starts-with-area > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(
                1, "div#hide-if-id-starts-with-area > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfShadowContainsBasicUsage() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-shadow-contains 'hisc-basic' p");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-shadow-contains");
        mHelper.verifyHiddenCount(1, "div#basic-target > p[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testHideIfShadowContainsRegexUsage() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#hide-if-shadow-contains '/hisc-regex[1-2]/' div");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "hide-if-shadow-contains");
        mHelper.verifyHiddenCount(2, "div#regex-target > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div#regex-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testJsonPrune() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#json-prune 'data-expectedresult jsonprune aria-label' ");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "json-prune?delay=100");
        // The object does not get hidden, it no longer exists in the DOM.
        mHelper.verifyDisplayedCount(0, "div#testcase-area > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testOverridePropertyRead() throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#override-property-read overridePropertyRead.fp false ");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "override-property-read");
        mHelper.verifyHiddenCount(1, "div#basic-target > div[data-expectedresult='fail']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testStripFetchQueryParameterBasicUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#strip-fetch-query-parameter basicBlocked testpages.adblockplus.org");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "strip-fetch-query-parameter");
        mHelper.verifyDisplayedCount(0, "div#basic-target > div[data-expectedresult='fail']");
        mHelper.verifyDisplayedCount(1, "div#basic-target > div[data-expectedresult='pass']");
    }

    @Test
    @LargeTest
    @Feature({"adblock"})
    public void testStripFetchQueryParameterOtherUsage()
            throws TimeoutException, InterruptedException {
        mHelper.addCustomFilter(
                "testpages.adblockplus.org#$#strip-fetch-query-parameter otherAllowed2 other-domain");
        mHelper.loadUrl(SNIPPETS_COMMON_UNIT_TESTCASES_ROOT + "strip-fetch-query-parameter");
        mHelper.verifyDisplayedCount(2, "div#other-target > div[data-expectedresult='pass']");
    }
}

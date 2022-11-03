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

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.IntegrationTest;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AdblockControllerTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    private CallbackHelper mHelper = new CallbackHelper();

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @IntegrationTest
    @Feature({"adblock"})
    public void composeFilterSuggestionsForObject() throws TimeoutException {
        final String url = "https://test.com/page/";
        Map<String, String> attributes;

        attributes = new HashMap<>();
        attributes.put("name", "src");
        attributes.put("value", "/data1");
        final AdblockElement child1 = new AdblockElement("param", url, attributes);

        attributes = new HashMap<>();
        attributes.put("name", "_src");
        attributes.put("value", "/data2");
        final AdblockElement child2 = new AdblockElement("param", url, attributes);

        attributes = new HashMap<>();
        attributes.put("name", "src");
        attributes.put("value", "/data3");
        final AdblockElement child3 = new AdblockElement("div", url, attributes);

        attributes = new HashMap<>();
        final AdblockElement object = new AdblockElement(
                "object", url, attributes, new AdblockElement[] {child1, child2, child3});

        final List<String> suggestions = new ArrayList<>();

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().composeFilterSuggestions(
                    object, new AdblockComposeFilterSuggestionsCallback() {
                        @Override
                        public void onDone(AdblockElement element, String[] filters) {
                            suggestions.addAll(Arrays.asList(filters));
                            mHelper.notifyCalled();
                        }
                    });
        });

        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        Assert.assertArrayEquals(
                new String[] {"||test.com/page/data1"}, suggestions.toArray(new String[] {}));
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void filterTestNoFilter() throws MalformedURLException, TimeoutException {
        URL testURL = new URL("https://somedomain.com?page=1&aimageid=123");
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.FilterMatchCheckParams params =
                    new AdblockController.FilterMatchCheckParams(
                            testURL, AdblockContentType.CONTENT_TYPE_OTHER, null, null, false);
            AdblockController.getInstance().hasMatchingFilter(
                    params, new AdblockMatchingFilterCheckCallback() {
                        @Override
                        public void onMatchingFilterCheckResult(final boolean foundMatchingFilter) {
                            Assert.assertFalse(foundMatchingFilter);
                            mHelper.notifyCalled();
                        }
                    });
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void filterTestMatchingFilter() throws MalformedURLException, TimeoutException {
        URL testURL = new URL("https://somedomain.com?page=1&mytestcustomfilter=123");
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addCustomFilter("&mytestcustomfilter=");
            AdblockController.FilterMatchCheckParams params =
                    new AdblockController.FilterMatchCheckParams(
                            testURL, AdblockContentType.CONTENT_TYPE_OTHER, null, null, false);
            AdblockController.getInstance().hasMatchingFilter(
                    params, new AdblockMatchingFilterCheckCallback() {
                        @Override
                        public void onMatchingFilterCheckResult(final boolean foundMatchingFilter) {
                            Assert.assertTrue(foundMatchingFilter);
                            mHelper.notifyCalled();
                        }
                    });
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void filterTestMatchingFilterWithDocUrl()
            throws MalformedURLException, TimeoutException {
        URL testURL = new URL("https://somedomain.com?page=1&mytestcustomfilter=123");
        URL docURL = new URL("https://somedomain.com/");
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addCustomFilter("&mytestcustomfilter=");
            AdblockController.FilterMatchCheckParams params =
                    new AdblockController.FilterMatchCheckParams(
                            testURL, AdblockContentType.CONTENT_TYPE_OTHER, docURL, null, false);
            AdblockController.getInstance().hasMatchingFilter(
                    params, new AdblockMatchingFilterCheckCallback() {
                        @Override
                        public void onMatchingFilterCheckResult(final boolean foundMatchingFilter) {
                            Assert.assertTrue(foundMatchingFilter);
                            mHelper.notifyCalled();
                        }
                    });
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void filterTestNoMatchingFilter() throws MalformedURLException, TimeoutException {
        URL testURL = new URL("https://somedomain.com?page=1&foobarbaz=123");
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addCustomFilter("&foobar=");
            AdblockController.FilterMatchCheckParams params =
                    new AdblockController.FilterMatchCheckParams(
                            testURL, AdblockContentType.CONTENT_TYPE_OTHER, null, null, false);
            AdblockController.getInstance().hasMatchingFilter(
                    params, new AdblockMatchingFilterCheckCallback() {
                        @Override
                        public void onMatchingFilterCheckResult(final boolean foundMatchingFilter) {
                            Assert.assertFalse(foundMatchingFilter);
                            mHelper.notifyCalled();
                        }
                    });
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    public void addingAllowedDomains() throws TimeoutException {
        final List<String> allowedDomains = new ArrayList<>();
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addAllowedDomain("foobar.com");
            AdblockController.getInstance().addAllowedDomain("domain.com/path/to/page.html");
            AdblockController.getInstance().addAllowedDomain("domain.com/duplicate.html");
            AdblockController.getInstance().addAllowedDomain("https://scheme.com/path.html");
            AdblockController.getInstance().addAllowedDomain("gibberish");
            allowedDomains.addAll(AdblockController.getInstance().getAllowedDomains());
            mHelper.notifyCalled();
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        // We expect to see a sorted collection of domains (not URLs) without duplicates.
        ArrayList<String> expectedAllowedDomains = new ArrayList<String>();
        expectedAllowedDomains.add("domain.com");
        expectedAllowedDomains.add("foobar.com");
        expectedAllowedDomains.add("scheme.com");
        expectedAllowedDomains.add("www.gibberish.com");
        Assert.assertEquals(expectedAllowedDomains, allowedDomains);
    }

    @Test
    @IntegrationTest
    @LargeTest
    @Feature({"adblock"})
    @DisabledTest(message = " https://jira.eyeo.com/browse/DPD-688")
    public void filterTestRemoveCustomFilter() throws MalformedURLException, TimeoutException {
        URL testURL = new URL("https://somedomain.com?page=1&mytestcustomfilter=789");
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addCustomFilter("&mytestcustomfilter=");
            AdblockController.getInstance().removeCustomFilter("&mytestcustomfilter=");
            AdblockController.FilterMatchCheckParams params =
                    new AdblockController.FilterMatchCheckParams(
                            testURL, AdblockContentType.CONTENT_TYPE_OTHER, null, null, false);
            AdblockController.getInstance().hasMatchingFilter(
                    params, new AdblockMatchingFilterCheckCallback() {
                        @Override
                        public void onMatchingFilterCheckResult(final boolean foundMatchingFilter) {
                            Assert.assertFalse(foundMatchingFilter);
                            mHelper.notifyCalled();
                        }
                    });
        });
        mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
    }
}

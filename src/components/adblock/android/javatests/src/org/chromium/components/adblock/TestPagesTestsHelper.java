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

import org.junit.Assert;
import org.junit.Rule;

import org.chromium.base.Log;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.lang.Thread;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class TestPagesTestsHelper {
    public static final String MAIN_DISTRIBUTION_UNIT_TEST_PAGE =
            "https://dp-testpages.adblockplus.org/";
    public static final String MAIN_COMMON_TEST_PAGE = "https://testpages.adblockplus.org/";
    public static final String DISTRIBUTION_UNIT_TESTCASES_ROOT =
            MAIN_DISTRIBUTION_UNIT_TEST_PAGE + "en/";
    public static final String COMMON_UNIT_TESTCASES_ROOT = MAIN_COMMON_TEST_PAGE + "en/";
    public static final String FILTER_DISTRIBUTION_UNIT_TESTCASES_ROOT =
            DISTRIBUTION_UNIT_TESTCASES_ROOT + "filters/";
    public static final String EXCEPTION_DISTRIBUTION_UNIT_TESTCASES_ROOT =
            DISTRIBUTION_UNIT_TESTCASES_ROOT + "exceptions/";
    public static final String CIRCUMVENTION_DISTRIBUTION_UNIT_TESTCASES_ROOT =
            DISTRIBUTION_UNIT_TESTCASES_ROOT + "circumvention/";
    public static final String SNIPPETS_COMMON_UNIT_TESTCASES_ROOT =
            COMMON_UNIT_TESTCASES_ROOT + "snippets/";
    public static final String DISTRIBUTION_UNIT_RESOURCES_ROOT =
            MAIN_DISTRIBUTION_UNIT_TEST_PAGE + "testfiles/";
    public static final String DISTRIBUTION_UNIT_TESTPAGE_SUBSCRIPTION =
            DISTRIBUTION_UNIT_TESTCASES_ROOT + "abp-testcase-subscription.txt";
    public static final String DISTRIBUTION_UNIT_WEBSOCKET_TESTPAGE_SUBSCRIPTION =
            DISTRIBUTION_UNIT_TESTCASES_ROOT + "websocket-webrtc-subscription.txt";
    public static final String TESTPAGE_SUBSCRIPTION =
            "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt";

    public enum IncludeSubframes {
        YES,
        NO,
    }

    private static final int TEST_TIMEOUT_SEC = 30;

    private static final String MATCHES_HIDDEN_FUNCTION = "function matches(element) {"
            + "  return window.getComputedStyle(element).display == \"none\""
            + "}";

    private static final String MATCHES_DISPLAYED_FUNCTION = "function matches(element) {"
            + "  return window.getComputedStyle(element).display != \"none\""
            + "}";

    private static final String COUNT_ELEMENT_FUNCTION =
            "function countElements(selector, includeSubframes) {"
            + "  var count = 0;"
            + "  for (let element of document.querySelectorAll(selector)) {"
            + "    if (matches(element))"
            + "      count++"
            + "  }"
            + "  if (includeSubframes) {"
            + "    for (let frame of document.querySelectorAll(\"iframe\")) {"
            + "      for (let element of frame.contentWindow.document.body.querySelectorAll(selector)) {"
            + "        if (matches(element))"
            + "          count++"
            + "      }"
            + "    }"
            + "  }"
            + "  return count"
            + "}";

    private URL mTestSubscriptionUrl;
    private CallbackHelper mHelper = new CallbackHelper();
    private TestAdBlockedObserver mObserver = new TestAdBlockedObserver();
    private TestSubscriptionUpdatedObserver mSubscriptionUpdateObserver =
            new TestSubscriptionUpdatedObserver();
    private ChromeTabbedActivityTestRule mActivityTestRule;

    private class TestSubscriptionUpdatedObserver
            implements AdblockController.SubscriptionUpdateObserver {
        @Override
        public void onSubscriptionDownloaded(final URL url) {
            if (mTestSubscriptionUrl == null) return;
            if (url.toString().contains(mTestSubscriptionUrl.toString())) {
                Log.d("TestSubscriptionUpdatedObserver",
                        "Notify subscription updated: " + url.toString());
                mHelper.notifyCalled();
            }
        }
    }

    private class TestAdBlockedObserver implements AdblockController.AdBlockedObserver {
        @Override
        public void onAdAllowed(AdblockCounters.ResourceInfo info) {
            if (countDownLatch != null) {
                countDownLatch.countDown();
            }
            // to be remove after DPD-961
            if (alreadyCounted.contains(info.getRequestUrl())) {
                return;
            }
            alreadyCounted.add(info.getRequestUrl());
            allowedInfos.add(info);
            Assert.assertTrue(info.getSubscription().equals(getExpectedSubscriptionUrl()));
        }

        @Override
        public void onAdBlocked(AdblockCounters.ResourceInfo info) {
            if (countDownLatch != null) {
                countDownLatch.countDown();
            }
            // to be remove after DPD-961
            if (alreadyCounted.contains(info.getRequestUrl())) {
                return;
            }
            alreadyCounted.add(info.getRequestUrl());
            blockedInfos.add(info);
            Assert.assertTrue(info.getSubscription().equals(getExpectedSubscriptionUrl()));
        }

        @Override
        public void onPageAllowed(AdblockCounters.ResourceInfo info) {
            allowedPageInfos.add(info);
            Assert.assertTrue(info.getSubscription().equals(getExpectedSubscriptionUrl()));
        }

        @Override
        public void onPopupAllowed(AdblockCounters.ResourceInfo info) {
            allowedPopupsInfos.add(info);
            Assert.assertTrue(info.getSubscription().equals(getExpectedSubscriptionUrl()));
        }

        @Override
        public void onPopupBlocked(AdblockCounters.ResourceInfo info) {
            blockedPopupsInfos.add(info);
            Assert.assertTrue(info.getSubscription().equals(getExpectedSubscriptionUrl()));
        }

        public boolean isBlocked(String url) {
            for (AdblockCounters.ResourceInfo info : blockedInfos) {
                if (info.getRequestUrl().contains(url)) return true;
            }

            return false;
        }

        public boolean isPopupBlocked(String url) {
            for (AdblockCounters.ResourceInfo info : blockedPopupsInfos) {
                if (info.getRequestUrl().contains(url)) return true;
            }

            return false;
        }

        public int numBlockedByType(AdblockContentType type) {
            int result = 0;
            for (AdblockCounters.ResourceInfo info : blockedInfos) {
                if (info.getAdblockContentType() == type) ++result;
            }
            return result;
        }

        public int numBlockedPopups() {
            int result = 0;
            for (AdblockCounters.ResourceInfo info : blockedPopupsInfos) {
                ++result;
            }
            return result;
        }

        public boolean isAllowed(String url) {
            for (AdblockCounters.ResourceInfo info : allowedInfos) {
                if (info.getRequestUrl().contains(url)) return true;
            }

            return false;
        }

        public boolean isPageAllowed(String url) {
            for (AdblockCounters.ResourceInfo info : allowedPageInfos) {
                if (info.getRequestUrl().contains(url)) return true;
            }

            return false;
        }

        public boolean isPopupAllowed(String url) {
            for (AdblockCounters.ResourceInfo info : allowedPopupsInfos) {
                if (info.getRequestUrl().contains(url)) return true;
            }

            return false;
        }

        public int numAllowedByType(AdblockContentType type) {
            int result = 0;
            for (AdblockCounters.ResourceInfo info : allowedInfos) {
                if (info.getAdblockContentType() == type) ++result;
            }
            return result;
        }

        public int numAllowedPopups() {
            int result = 0;
            for (AdblockCounters.ResourceInfo info : allowedPopupsInfos) {
                ++result;
            }
            return result;
        }

        String getExpectedSubscriptionUrl() {
            if (mTestSubscriptionUrl != null) return mTestSubscriptionUrl.toString();
            return "adblock:custom";
        }

        public List<AdblockCounters.ResourceInfo> blockedInfos = new CopyOnWriteArrayList<>();
        public List<AdblockCounters.ResourceInfo> allowedInfos = new CopyOnWriteArrayList<>();
        public List<AdblockCounters.ResourceInfo> allowedPageInfos = new CopyOnWriteArrayList<>();
        public List<AdblockCounters.ResourceInfo> blockedPopupsInfos = new CopyOnWriteArrayList<>();
        public List<AdblockCounters.ResourceInfo> allowedPopupsInfos = new CopyOnWriteArrayList<>();
        public CountDownLatch countDownLatch;
        // public remove after DPD-961
        Set<String> alreadyCounted = new HashSet<String>();
    };

    public void setUp(ChromeTabbedActivityTestRule activityRule) {
        mActivityTestRule = activityRule;
        mActivityTestRule.startMainActivityOnBlankPage();
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addOnAdBlockedObserver(mObserver);
            AdblockController.getInstance().addSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
        });
    }

    public void addFilterList(String filterListUrl) {
        try {
            mTestSubscriptionUrl = new URL(filterListUrl);
        } catch (MalformedURLException e) {
        }
        Assert.assertNotNull("Test subscription url", mTestSubscriptionUrl);
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addCustomSubscription(mTestSubscriptionUrl);
        });
        try {
            mHelper.waitForCallback(0, 1, TEST_TIMEOUT_SEC, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            Assert.assertEquals(
                    "Test subscription was properly added", "Failed to add test subscription");
        }
    }

    public void addCustomFilter(String filter) {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { AdblockController.getInstance().addCustomFilter(filter); });
    }

    public void tearDown() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().removeSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
            AdblockController.getInstance().removeOnAdBlockedObserver(mObserver);
        });
    }

    public WebContents getWebContents() {
        return mActivityTestRule.getActivity().getCurrentWebContents();
    }

    public int getTabCount() {
        return mActivityTestRule.getActivity().getTabModelSelector().getTotalTabCount();
    }

    public void verifyHiddenCount(int num, String selector) throws TimeoutException {
        verifyHiddenCount(num, selector, IncludeSubframes.YES);
    }

    public void verifyHiddenCount(int num, String selector, IncludeSubframes includeSubframes)
            throws TimeoutException {
        verifyMatchesCount(num, MATCHES_HIDDEN_FUNCTION, selector, includeSubframes);
    }

    public void verifyDisplayedCount(int num, String selector) throws TimeoutException {
        verifyDisplayedCount(num, selector, IncludeSubframes.YES);
    }

    public void verifyDisplayedCount(int num, String selector, IncludeSubframes includeSubframes)
            throws TimeoutException {
        verifyMatchesCount(num, MATCHES_DISPLAYED_FUNCTION, selector, includeSubframes);
    }

    public void verifyMatchesCount(int num, String matchesFunction, String selector,
            IncludeSubframes includeSubframes) throws TimeoutException {
        final Tab tab = mActivityTestRule.getActivity().getActivityTab();
        final String boolIncludeSubframes =
                includeSubframes == IncludeSubframes.YES ? "true" : "false";
        final String count = JavaScriptUtils.executeJavaScriptAndWaitForResult(tab.getWebContents(),
                matchesFunction + COUNT_ELEMENT_FUNCTION + "\n countElements(\"" + selector + "\", "
                        + boolIncludeSubframes + ");");
        Assert.assertEquals(Integer.toString(num), count);
    }

    public void verifyGreenBackground(String elemId) throws TimeoutException {
        String color = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                mActivityTestRule.getActivity().getActivityTab().getWebContents(),
                "window.getComputedStyle(document.getElementById('" + elemId
                        + "')).backgroundColor");
        Assert.assertEquals("\"rgb(13, 199, 75)\"", color);
    }

    // For some cases it is better to rely on page script testing element
    // rather than invent a specific script to check condition. For example
    // checks for rewrite filters replaces content proper way.
    public void verifySelfTestPass(String elemId) throws TimeoutException {
        String value = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                mActivityTestRule.getActivity().getActivityTab().getWebContents(),
                "document.getElementById('" + elemId + "').getAttribute('data-expectedresult')");
        Assert.assertEquals("\"pass\"", value);
    }

    public void setOnAdMatchedLatch(final CountDownLatch countDownLatch) {
        mObserver.countDownLatch = countDownLatch;
    }

    public boolean isBlocked(String url) {
        return mObserver.isBlocked(url);
    }

    public boolean isPopupBlocked(String url) {
        return mObserver.isPopupBlocked(url);
    }

    public int numBlockedByType(AdblockContentType type) {
        return mObserver.numBlockedByType(type);
    }

    public int numBlockedPopups() {
        return mObserver.numBlockedPopups();
    }

    public int numAllowedByType(AdblockContentType type) {
        return mObserver.numAllowedByType(type);
    }

    public int numAllowedPopups() {
        return mObserver.numAllowedPopups();
    }

    public boolean isAllowed(String url) {
        return mObserver.isAllowed(url);
    }

    public boolean isPageAllowed(String url) {
        return mObserver.isPageAllowed(url);
    }

    public boolean isPopupAllowed(String url) {
        return mObserver.isPopupAllowed(url);
    }

    public void loadUrl(String url) throws InterruptedException {
        mActivityTestRule.loadUrl(url, TEST_TIMEOUT_SEC);
        // Some tests were using a short delay (up to 1s) on case by case basis.
        // Do this for all to get rid of some flackiness and surprises when
        // adding new tests.
        // TODO(pstanek): Why it's suddenly needed and so long.
        // TODO(mpawlowski): get rid of this sleep - DPD-1207
        Thread.sleep(2000);
    }

    public int numBlocked() {
        return mObserver.blockedInfos.size();
    }

    public int numAllowed() {
        return mObserver.allowedInfos.size();
    }
}

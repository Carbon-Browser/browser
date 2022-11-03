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

import org.junit.Assert;
import org.junit.Rule;

import org.chromium.base.Log;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.adblock.AdblockController;
import org.chromium.chrome.browser.adblock.AdblockCounters;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.lang.Thread;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class TestPagesTestsHelper {
    public static final String MAIN_DISTRIBUTION_UNIT_TEST_PAGE =
            "https://dp-testpages.adblockplus.org/";
    public static final String MAIN_COMMON_TEST_PAGE =
            "https://testpages.adblockplus.org/";
    public static final String DISTRIBUTION_UNIT_TESTCASES_ROOT =
            MAIN_DISTRIBUTION_UNIT_TEST_PAGE + "en/";
    public static final String COMMON_UNIT_TESTCASES_ROOT =
            MAIN_COMMON_TEST_PAGE + "en/";
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
    private static final int TEST_TIMEOUT_SEC = 30;
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
            if (url.toString().contains(mTestSubscriptionUrl.toString())) {
                Log.d("TestSubscriptionUpdatedObserver",
                        "Notify subscription updated: " + url.toString());
                mHelper.notifyCalled();
            }
        }

        @Override
        public void onRecommendedSubscriptionsAvailable() {}
    }

    private class TestAdBlockedObserver implements AdblockController.AdBlockedObserver {
        @Override
        public void onAdMatched(AdblockCounters.ResourceInfo info, boolean wasBlocked) {
            if (wasBlocked) {
                blockedInfos.add(info);
            } else {
                allowedInfos.add(info);
            }
            Assert.assertTrue(info.getSubscriptions().contains(mTestSubscriptionUrl.toString())
                    || (info.getSubscriptions().isEmpty()));
        }

        public boolean isBlocked(String url) {
            for (AdblockCounters.ResourceInfo info : blockedInfos) {
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

        public boolean isAllowed(String url) {
            for (AdblockCounters.ResourceInfo info : allowedInfos) {
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

        public List<AdblockCounters.ResourceInfo> blockedInfos = new CopyOnWriteArrayList<>();
        public List<AdblockCounters.ResourceInfo> allowedInfos = new CopyOnWriteArrayList<>();
    };

    public void setUp(ChromeTabbedActivityTestRule activityRule, String filterListUrl) {
        mActivityTestRule = activityRule;
        mActivityTestRule.startMainActivityOnBlankPage();
        try {
            mTestSubscriptionUrl = new URL(filterListUrl);
        } catch (MalformedURLException e) {
        }
        Assert.assertNotNull("Test subscription url", mTestSubscriptionUrl);
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
            AdblockController.getInstance().addCustomSubscription(mTestSubscriptionUrl);
            AdblockController.getInstance().addOnAdBlockedObserver(mObserver);
        });
        try {
            mHelper.waitForCallback(0, 1, TEST_TIMEOUT_SEC, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            Assert.assertEquals(
                    "Test subscription was properly added", "Failed to add test subscription");
        }
    }

    public void tearDown() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().removeSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
            AdblockController.getInstance().removeCustomSubscription(mTestSubscriptionUrl);
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
        final Tab tab = mActivityTestRule.getActivity().getActivityTab();
        String numHidden = JavaScriptUtils.executeJavaScriptAndWaitForResult(tab.getWebContents(),
                "var hiddenCount = 0;"
                        + "var elements = document.querySelectorAll(\"" + selector + "\");"
                        + "for (let i = 0; i < elements.length; ++i) {"
                        + "    if (window.getComputedStyle(elements[i]).display == \"none\") ++hiddenCount;"
                        + "}"
                        + "hiddenCount;");
        Log.d("verifyHiddenCount", numHidden);
        Assert.assertEquals(Integer.toString(num), numHidden);
    }

    public void verifyDisplayedCount(int num, String selector) throws TimeoutException {
        final Tab tab = mActivityTestRule.getActivity().getActivityTab();
        String numDisplayed = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                tab.getWebContents(),
                "var displayedCount = 0;"
                        + "var elements = document.querySelectorAll(\"" + selector + "\");"
                        + "for (let i = 0; i < elements.length; ++i) {"
                        + "    if (window.getComputedStyle(elements[i]).display != \"none\") ++displayedCount;"
                        + "}"
                        + "displayedCount;");
        Log.d("verifyDisplayedCount", numDisplayed);
        Assert.assertEquals(Integer.toString(num), numDisplayed);
    }

    public void verifyGreenBackground(String elemId) throws TimeoutException {
        String color = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                mActivityTestRule.getActivity().getActivityTab().getWebContents(),
                "window.getComputedStyle(document.getElementById('" + elemId
                        + "')).backgroundColor");
        Assert.assertEquals("\"rgb(13, 199, 75)\"", color);
    }

    public boolean isBlocked(String url) {
        return mObserver.isBlocked(url);
    }

    public int numBlockedByType(AdblockContentType type) {
        return mObserver.numBlockedByType(type);
    }

    public int numAllowedByType(AdblockContentType type) {
        return mObserver.numAllowedByType(type);
    }

    public boolean isAllowed(String url) {
        return mObserver.isAllowed(url);
    }

    public void loadUrl(String url) throws InterruptedException {
        mActivityTestRule.loadUrl(url, TEST_TIMEOUT_SEC);
        // Some tests were using a short delay (up to 1s) on case by case basis.
        // Do this for all to get rid of some flackiness and surprises when
        // adding new tests.
        // TODO(pstanek): Why it's suddenly needed and so long.
        Thread.sleep(10000);
    }

    public int numBlocked() {
        return mObserver.blockedInfos.size();
    }

    public int numAllowed() {
        return mObserver.allowedInfos.size();
    }
}

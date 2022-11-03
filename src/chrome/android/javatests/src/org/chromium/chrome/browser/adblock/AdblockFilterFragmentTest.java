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

import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Log;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.adblock.settings.AdblockFilterListsFragment;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.net.URL;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AdblockFilterFragmentTest {
    public SettingsActivityTestRule<AdblockFilterListsFragment> mTestRule =
            new SettingsActivityTestRule<>(AdblockFilterListsFragment.class);
    private CallbackHelper mHelper = new CallbackHelper();
    private TestSubscriptionUpdatedObserver mSubscriptionUpdateObserver =
            new TestSubscriptionUpdatedObserver();
    private boolean mShouldWaitForRecommendedSubscriptions;

    private class TestSubscriptionUpdatedObserver
            implements AdblockController.SubscriptionUpdateObserver {
        @Override
        public void onSubscriptionDownloaded(final URL url) {}

        @Override
        public void onRecommendedSubscriptionsAvailable() {
            mHelper.notifyCalled();
        }
    }

    @Before
    public void setUp() throws Exception {
        // This triggers "first run" i.e. makes sure caching has a chance
        // to happen.
        mTestRule.startSettingsActivity();
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().addSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
            AdblockController.getInstance().getRecommendedSubscriptions(
                    InstrumentationRegistry.getContext());
            mShouldWaitForRecommendedSubscriptions =
                    !AdblockController.getInstance().isRecommendedSubscriptionsAvailable();
        });
        try {
            if (mShouldWaitForRecommendedSubscriptions)
                mHelper.waitForCallback(0, 1, 10, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            Assert.assertEquals(
                    "Recommended subscriptions became available", "No recommended subscriptions");
        }
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AdblockController.getInstance().removeSubscriptionUpdateObserver(
                    mSubscriptionUpdateObserver);
        });
        ApplicationTestUtils.finishActivity(mTestRule.getActivity());
    }

    @Test
    @SmallTest
    @Feature({"Adblock"})
    public void testLanguageFiltersNotEmpty() {
        mTestRule.startSettingsActivity();
        AdblockFilterListsFragment fragment = mTestRule.getFragment();
        Assert.assertNotNull(fragment);
        Assert.assertNotEquals(0, fragment.getListView().getCount());
    }
}

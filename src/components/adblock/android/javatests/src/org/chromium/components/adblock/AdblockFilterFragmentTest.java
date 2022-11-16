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
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.adblock.settings.AdblockFilterListsFragment;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

import java.net.URL;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AdblockFilterFragmentTest {
    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
          new ChromeTabbedActivityTestRule();
    @Rule
    public SettingsActivityTestRule<AdblockFilterListsFragment> mTestRule =
            new SettingsActivityTestRule<>(AdblockFilterListsFragment.class);

    @Before
    public void setUp() throws Exception {
      mActivityTestRule.startMainActivityOnBlankPage();
      mTestRule.startSettingsActivity();
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

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tasks.tab_management;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;

import static org.chromium.base.test.util.Batch.PER_CLASS;
import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_NON_LOW_END_DEVICE;
import static org.chromium.chrome.browser.tasks.tab_management.TabUiTestHelper.createTabs;
import static org.chromium.chrome.browser.tasks.tab_management.TabUiTestHelper.enterTabSwitcher;

import android.content.res.Configuration;

import androidx.test.filters.MediumTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.tab_ui.R;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.ActivityTestUtils;
import org.chromium.chrome.test.util.ChromeRenderTestRule;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.ui.test.util.UiRestriction;

import java.io.IOException;

/**
 * Render tests for incognito re-auth promo message card.
 *
 * TODO(crbug.com/1227656): Add render tests for snack bar when integrated with review action.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
@Restriction({UiRestriction.RESTRICTION_TYPE_PHONE, RESTRICTION_TYPE_NON_LOW_END_DEVICE})
@Features.EnableFeatures({ChromeFeatureList.INCOGNITO_REAUTHENTICATION_FOR_ANDROID,
        ChromeFeatureList.TAB_GROUPS_CONTINUATION_ANDROID})
@Batch(PER_CLASS)
public class IncognitoReauthPromoCardRenderTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Rule
    public ChromeRenderTestRule mRenderTestRule =
            ChromeRenderTestRule.Builder.withPublicCorpus()
                    .setBugComponent(ChromeRenderTestRule.Component.PRIVACY_INCOGNITO)
                    .build();

    @Before
    public void setUp() {
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(true);
        IncognitoReauthPromoMessageService.setIsPromoEnabledForTesting(true);
        mActivityTestRule.startMainActivityOnBlankPage();

        CriteriaHelper.pollUiThread(
                mActivityTestRule.getActivity().getTabModelSelector()::isTabStateInitialized);
    }

    @After
    public void tearDown() {
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(false);
        IncognitoReauthPromoMessageService.setIsPromoEnabledForTesting(false);
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testRenderReauthPromoMessageCard_Portrait() throws IOException {
        final ChromeTabbedActivity cta = mActivityTestRule.getActivity();

        createTabs(cta, true, 1);
        enterTabSwitcher(cta);
        CriteriaHelper.pollUiThread(TabSwitcherCoordinator::hasAppendedMessagesForTesting);
        onView(withId(R.id.large_message_card_item)).check(matches(isDisplayed()));
        mRenderTestRule.render(
                cta.findViewById(R.id.large_message_card_item), "incognito_reauth_promo_portrait");
    }

    @Test
    @MediumTest
    @Feature({"RenderTest"})
    public void testRenderReauthPromoMessageCard_Landscape() throws IOException {
        final ChromeTabbedActivity cta = mActivityTestRule.getActivity();

        createTabs(cta, true, 1);
        ActivityTestUtils.rotateActivityToOrientation(cta, Configuration.ORIENTATION_LANDSCAPE);

        enterTabSwitcher(cta);
        CriteriaHelper.pollUiThread(TabSwitcherCoordinator::hasAppendedMessagesForTesting);
        onView(withId(R.id.large_message_card_item)).check(matches(isDisplayed()));
        mRenderTestRule.render(
                cta.findViewById(R.id.large_message_card_item), "incognito_reauth_promo_landscape");
    }
}
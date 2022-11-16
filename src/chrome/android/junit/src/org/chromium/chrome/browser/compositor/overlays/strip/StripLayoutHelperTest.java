// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.overlays.strip;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.layouts.LayoutRenderHost;
import org.chromium.chrome.browser.compositor.layouts.LayoutUpdateHost;
import org.chromium.chrome.browser.compositor.layouts.components.CompositorButton;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutTab.StripLayoutTabDelegate;
import org.chromium.chrome.browser.compositor.overlays.strip.TabLoadTracker.TabLoadTrackerCallback;
import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.layouts.animation.CompositorAnimationHandler;
import org.chromium.chrome.browser.layouts.animation.CompositorAnimator;
import org.chromium.chrome.browser.layouts.components.VirtualView;
import org.chromium.chrome.browser.tasks.tab_groups.TabGroupModelFilter;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.ui.base.LocalizationUtils;

import java.util.ArrayList;
import java.util.List;

/** Tests for {@link StripLayoutHelper}. */
@RunWith(BaseRobolectricTestRunner.class)
@Features.EnableFeatures({ChromeFeatureList.TAB_STRIP_IMPROVEMENTS,
        ChromeFeatureList.GRID_TAB_SWITCHER_FOR_TABLETS, ChromeFeatureList.TAB_GROUPS_FOR_TABLETS})
@Config(manifest = Config.NONE, sdk = Build.VERSION_CODES.M, qualifiers = "sw600dp")
public class StripLayoutHelperTest {
    @Rule
    public TestRule mFeaturesProcessorRule = new Features.JUnitProcessor();
    @Mock
    private LayoutUpdateHost mUpdateHost;
    @Mock
    private LayoutRenderHost mRenderHost;
    @Mock
    private CompositorButton mModelSelectorBtn;
    @Mock
    private TabGroupModelFilter mTabGroupModelFilter;

    private Activity mActivity;
    private TestTabModel mModel = new TestTabModel();
    private StripLayoutHelper mStripLayoutHelper;
    private boolean mIncognito;
    private static final String[] TEST_TAB_TITLES = {"Tab 1", "Tab 2", "Tab 3", "", null};
    private static final String EXPECTED_MARGIN = "The tab should have a trailing margin.";
    private static final String EXPECTED_NO_MARGIN = "The tab should not have a trailing margin.";
    private static final String CLOSE_TAB = "Close %1$s tab";
    private static final String IDENTIFIER = "Tab";
    private static final String IDENTIFIER_SELECTED = "Selected Tab";
    private static final String INCOGNITO_IDENTIFIER = "Incognito Tab";
    private static final String INCOGNITO_IDENTIFIER_SELECTED = "Selected Incognito Tab";
    private static final float SCREEN_WIDTH = 800.f;
    private static final float SCREEN_WIDTH_LANDSCAPE = 1200.f;
    private static final float SCREEN_HEIGHT = 1600.f;
    private static final float TAB_WIDTH_1 = 140.f;
    private static final float TAB_WIDTH_2 = 160.f;
    private static final float TAB_WIDTH_SMALL = 108.f;
    private static final float TAB_OVERLAP_WIDTH = 24.f;
    private static final float TAB_WIDTH_MEDIUM = 156.f;
    private static final float TAB_MARGIN_WIDTH = 95.f;
    private static final long TIMESTAMP = 5000;
    private static final float NEW_TAB_BTN_X = 700.f;
    private static final float NEW_TAB_BTN_Y = 1400.f;
    private static final float NEW_TAB_BTN_WIDTH = 100.f;
    private static final float NEW_TAB_BTN_HEIGHT = 100.f;

    private static final float CLOSE_BTN_VISIBILITY_THRESHOLD_END = 72;
    private static final float CLOSE_BTN_VISIBILITY_THRESHOLD_END_MODEL_SELECTOR = 120;

    private static final float EPSILON = 0.001f;

    /** Reset the environment before each test. */
    @Before
    public void beforeTest() {
        MockitoAnnotations.initMocks(this);
        when(mModelSelectorBtn.isVisible()).thenReturn(true);
        when(mTabGroupModelFilter.hasOtherRelatedTabs(any())).thenReturn(false);

        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        TabUiFeatureUtilities.setTabMinWidthForTesting(null);
    }

    @After
    public void tearDown() {
        if (mStripLayoutHelper != null) {
            mStripLayoutHelper.stopReorderModeForTesting();
            mStripLayoutHelper.setTabAtPositionForTesting(null);
        }
    }

    /**
     * Test method for {@link StripLayoutHelper#getVirtualViews(List<VirtualView>)}.
     *
     * Checks that it returns the correct order of tabs, including correct content.
     */
    @Test
    @Feature({"Accessibility"})
    public void testSimpleTabOrder() {
        initializeTest(false, false, 0);

        assertTabStripAndOrder(getExpectedAccessibilityDescriptions(0));
    }

    /**
     * Test method for {@link StripLayoutHelper#getVirtualViews(List<VirtualView>)}.
     *
     * Checks that it returns the correct order of tabs, even when a tab except the first one is
     * selected.
     */
    @Test
    @Feature({"Accessibility"})
    public void testTabOrderWithIndex() {
        initializeTest(false, false, 1);

        // Tabs should be in left to right order regardless of index
        assertTabStripAndOrder(getExpectedAccessibilityDescriptions(1));
    }

    /**
     * Test method for {@link StripLayoutHelper#getVirtualViews(List<VirtualView>)}.
     *
     * Checks that it returns the correct order of tabs, even in RTL mode.
     */
    @Test
    @Feature({"Accessibility"})
    public void testTabOrderRtl() {
        initializeTest(true, false, 0);

        // Tabs should be in linear order even in RTL.
        // Android will take care of reversing it.
        assertTabStripAndOrder(getExpectedAccessibilityDescriptions(0));
    }

    /**
     * Test method for {@link StripLayoutHelper#getVirtualViews(List<VirtualView>)}.
     *
     * Checks that it returns the correct order of tabs, even in incognito mode.
     */
    @Test
    @Feature({"Accessibility"})
    public void testIncognitoAccessibilityDescriptions() {
        initializeTest(false, true, 0);

        assertTabStripAndOrder(getExpectedAccessibilityDescriptions(0));
    }

    @Test
    @Feature("Tab Strip Improvements")
    @Config(qualifiers = "sw800dp")
    public void testStripStacker_TabStripImprovementsEnabled_Scroll() {
        initializeTest(false, true, 0);

        // Assert
        assertFalse(mStripLayoutHelper.shouldCascadeTabs());
    }

    @Test
    @Feature("Tab Strip Improvements")
    @Config(qualifiers = "sw800dp")
    @Features.DisableFeatures(ChromeFeatureList.TAB_STRIP_IMPROVEMENTS)
    public void testStripStacker_TabStripImprovementsDisabled_Cascade() {
        initializeTest(false, true, 0);

        // Assert
        assertTrue(mStripLayoutHelper.shouldCascadeTabs());
    }

    @Test
    public void testAllTabsClosed() {
        initializeTest(false, false, 0);
        assertTrue(mStripLayoutHelper.getStripLayoutTabs().length == TEST_TAB_TITLES.length);

        // Close all tabs
        mModel.closeAllTabs();

        // Notify strip of tab closure
        mStripLayoutHelper.willCloseAllTabs();

        // Verify strip has no tabs.
        assertTrue(mStripLayoutHelper.getStripLayoutTabs().length == 0);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testStripStacker_UpdateCloseButtons() {
        // Set fourth tab as selected
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_SMALL);
        initializeTest(false, true, 3);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_1);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Close btn should be visible on the selected tab.
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        // Close btn is hidden on unselected tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(false);
        Mockito.verify(tabs[1]).setCanShowCloseButton(false);
        Mockito.verify(tabs[2]).setCanShowCloseButton(false);
        Mockito.verify(tabs[4]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_SelectedTab_EdgeTab_HideCloseBtn() {
        // Set fourth tab as selected
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_SMALL);
        initializeTest(false, true, 3);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_1);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        when(tabs[3].getDrawX()).thenReturn(600.f);

        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Close btn should be hidden on the selected tab as its an edge tab.
        Mockito.verify(tabs[3]).setCanShowCloseButton(false);
        // Close btn is hidden on unselected tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(false);
        Mockito.verify(tabs[1]).setCanShowCloseButton(false);
        Mockito.verify(tabs[2]).setCanShowCloseButton(false);
        Mockito.verify(tabs[4]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_EdgeTab_Start_Ltr_HideCloseBtn() {
        // Arrange
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2);
        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.getNewTabButton().setX(600.f);
        // The leftmost tab is partially hidden
        when(tabs[0].getDrawX()).thenReturn(-80.f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[0]).setCanShowCloseButton(false);
        // Close button is visible for the rest of the tabs.
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        Mockito.verify(tabs[4]).setCanShowCloseButton(true);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_EdgeTab_End_Ltr_HideCloseBtn() {
        // Arrange
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, false, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2);
        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.getNewTabButton().setX(700);
        float drawXToHideCloseBtn = SCREEN_WIDTH - CLOSE_BTN_VISIBILITY_THRESHOLD_END_MODEL_SELECTOR
                - TAB_WIDTH_2 + TAB_OVERLAP_WIDTH + 1; // 1 is to cross threshold
        when(tabs[3].getDrawX()).thenReturn(drawXToHideCloseBtn);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close button is visible for the rest of the tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(true);
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[4]).setCanShowCloseButton(true);
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[3]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_LastTab_ShowCloseBtn() {
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, false, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2);
        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.getNewTabButton().setX(NEW_TAB_BTN_X);
        // newTabBtn.X(700.f) - tab.width(160.f) + mTabOverlapWidth(24.f)
        when(tabs[4].getDrawX()).thenReturn(564.f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close button is visible for all tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(true);
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        Mockito.verify(tabs[4]).setCanShowCloseButton(true);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_LastTab_EdgeTab_HideCloseBtn() {
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, false, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2);

        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.getNewTabButton().setX(NEW_TAB_BTN_X);
        // newTabBtn.X(700.f) - tab.width(160.f) + mTabOverlapWidth(24.f) + 1
        when(tabs[4].getDrawX()).thenReturn(565.f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close btn should be visible for rest of the tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(true);
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[4]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_EdgeTab_End_Ltr_NoModelSelBtn_HideCloseBtn() {
        // Arrange
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        when(mModelSelectorBtn.isVisible()).thenReturn(false);

        initializeTest(false, false, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2);
        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.getNewTabButton().setX(700);
        float drawXToHideCloseBtn = SCREEN_WIDTH - CLOSE_BTN_VISIBILITY_THRESHOLD_END - TAB_WIDTH_2
                + TAB_OVERLAP_WIDTH + 1; // 1 is to cross threshold
        when(tabs[3].getDrawX()).thenReturn(drawXToHideCloseBtn);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close button is visible for the rest of the tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(true);
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[4]).setCanShowCloseButton(true);
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[3]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_EdgeTab_Start_Rtl_HideCloseBtn() {
        // Arrange
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(true, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2, 150.f, 5);
        // Set mWidth value to 800.f.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // The leftmost tab is partially hidden.
        when(tabs[0].getDrawX()).thenReturn(60.f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[0]).setCanShowCloseButton(false);
        // Close button is visible for the rest of the tabs.
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        Mockito.verify(tabs[4]).setCanShowCloseButton(true);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_EdgeTab_End_Rtl_HideCloseBtn() {
        // Arrange
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(true, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_2, 150.f, 5);
        // Set mWidth value to 800.f
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // To make rightmost tab partially hidden value should be gt than (SCREEN_WIDTH - 120.f -
        // TAB_WIDTH_2).
        when(tabs[4].getDrawX()).thenReturn(710.f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Act
        mStripLayoutHelper.tabSelected(1, 3, 0, false);

        // Assert
        // Close button is visible for the rest of the tabs.
        Mockito.verify(tabs[0]).setCanShowCloseButton(true);
        Mockito.verify(tabs[1]).setCanShowCloseButton(true);
        Mockito.verify(tabs[2]).setCanShowCloseButton(true);
        Mockito.verify(tabs[3]).setCanShowCloseButton(true);
        // Close btn should be hidden for the partially visible edge tab.
        Mockito.verify(tabs[4]).setCanShowCloseButton(false);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnResume_StartOnLeft_SelectedRightmostTab() {
        // Arrange: Initialize tabs with last tab selected.
        initializeTest(false, true, 9, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Set screen width to 800dp.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);

        mStripLayoutHelper.scrollTabToView(TIMESTAMP, false);

        int expectedFinalX = -1004; // delta(optimalRight(-980) - scrollOffset(0)
                                    // - tabOverlapWidth(24)) + scrollOffset(0)
        assertEquals(expectedFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnResume_StartOnLeft_NoModelSelBtn_SelectedRightmostTab() {
        // Arrange: Initialize tabs with last tab selected.
        when(mModelSelectorBtn.isVisible()).thenReturn(false);
        initializeTest(false, true, 9, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Set screen width to 800dp.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);

        mStripLayoutHelper.scrollTabToView(TIMESTAMP, false);

        int expectedFinalX = -956; // delta(optimalRight(-932) - scrollOffset(0)
        // - tabOverlapWidth(24)) + scrollOffset(0)
        assertEquals(expectedFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnResume_StartOnRight_SelectedLeftmostTab() {
        // Arrange: Initialize tabs with first tab selected.
        initializeTest(false, true, 0, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.testSetScrollOffset(1200);

        // Set screen width to 800dp.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);

        mStripLayoutHelper.scrollTabToView(TIMESTAMP, false);

        int expectedFinalX = 490; // delta(optimalRight(514) - tabOverlapWidth(24)
                                  // - scrollOffset(1200)) + scrollOffset(1200)
        assertEquals(expectedFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnResume_StartOnRight_NoModelSelBtn_SelectedRightmostTab() {
        // Arrange: Initialize tabs with first tab selected.
        when(mModelSelectorBtn.isVisible()).thenReturn(false);
        initializeTest(false, true, 0, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.testSetScrollOffset(1200);

        // Set screen width to 800dp.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);

        mStripLayoutHelper.scrollTabToView(TIMESTAMP, false);

        int expectedFinalX = 538; // delta(optimalRight(562) - tabOverlapWidth(24)
        // - scrollOffset(1200)) + scrollOffset(1200)
        assertEquals(expectedFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnOrientationChange_SelectedTabVisible() {
        // Arrange: Initialize tabs with last tab selected.
        when(mModelSelectorBtn.isVisible()).thenReturn(false);
        initializeTest(false, true, 9, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        when(tabs[9].isVisible()).thenReturn(true);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.testSetScrollOffset(1000);

        // Set screen width to 1200 to start.
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH_LANDSCAPE, SCREEN_HEIGHT, false, TIMESTAMP);

        // Assert: finalX value before orientation change.
        int initialFinalX = -1494;
        assertEquals(initialFinalX, mStripLayoutHelper.getScroller().getFinalX());

        // Act: change orientation.
        when(tabs[9].getDrawX()).thenReturn(-1.f);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, true, TIMESTAMP);

        // Assert: finalX value after orientation change.
        int expectedFinalX = -956; // delta(optimalRight(-932) - tabOverlapWidth(24)) -
                                   // scrollOffset(1000) + scrollOffset(1000)
        assertEquals(expectedFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollOffset_OnOrientationChange_SelectedTabNotVisible() {
        // Arrange: Initialize tabs with last tab selected.
        when(mModelSelectorBtn.isVisible()).thenReturn(false);
        initializeTest(false, true, 9, 10);
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM, 150.f, 10);
        when(tabs[9].isVisible()).thenReturn(false);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.testSetScrollOffset(1000);

        // Set screen width to 1200 to start
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH_LANDSCAPE, SCREEN_HEIGHT, false, TIMESTAMP);

        // Assert: finalX value before orientation change.
        int initialFinalX = -1494;
        assertEquals(initialFinalX, mStripLayoutHelper.getScroller().getFinalX());

        // Act: change orientation.
        when(tabs[9].getDrawX()).thenReturn(-1.f);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, true, TIMESTAMP);

        // Assert: finalX value remains the same on orientation change.
        assertEquals(initialFinalX, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_AfterTabClose_SkipsAutoScroll() {
        initializeTest(false, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        // Set initial scroller position to 1000.
        mStripLayoutHelper.getScroller().setFinalX(1000);

        // Act: close a non selected tab.
        mStripLayoutHelper.handleCloseButtonClick(tabs[1], TIMESTAMP);

        // Assert: scroller position is not modified.
        assertEquals(1000, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabSelected_AfterSelectedTabClose_SkipsAutoScroll() {
        initializeTest(false, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        // Set initial scroller position to 1000.
        mStripLayoutHelper.getScroller().setFinalX(1000);

        // Act: close the selected tab.
        mStripLayoutHelper.handleCloseButtonClick(tabs[3], TIMESTAMP);

        // Assert: scroller position is not modified.
        assertEquals(1000, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabCreated_RestoredTab_SkipsAutoscroll() {
        initializeTest(false, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        // Set initial scroller position to 1000.
        mStripLayoutHelper.getScroller().setFinalX(1000);

        // Act: Tab was restored after undoing a tab closure.
        boolean restoredTab = true;
        mStripLayoutHelper.tabCreated(TIMESTAMP, 6, 3, false, restoredTab);

        // Assert: scroller position is not modified.
        assertEquals(1000, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabCreated_NonRestoredTab_SkipsAutoscroll() {
        initializeTest(false, true, 3);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(TAB_WIDTH_MEDIUM);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        // Set initial scroller position to 1000.
        mStripLayoutHelper.getScroller().setFinalX(1000);

        // Act: Tab was restored after undoing a tab closure.
        boolean restoredTab = false;
        mStripLayoutHelper.tabCreated(TIMESTAMP, 6, 3, false, restoredTab);

        // Assert: scroller position is modified.
        assertNotEquals(1000, mStripLayoutHelper.getScroller().getFinalX());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollDuration() {
        initializeTest(false, true, 3);

        // Act: Set scroll offset less than 960.
        mStripLayoutHelper.testSetScrollOffset(800);

        // Assert: Expand duration is 250.
        assertEquals(mStripLayoutHelper.getExpandDurationForTesting(), 250);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollDuration_Medium() {
        initializeTest(false, true, 3);

        // Act: Set scroll offset between 960 and 1920.
        mStripLayoutHelper.testSetScrollOffset(1000);

        // Assert: Expand duration is 350.
        assertEquals(mStripLayoutHelper.getExpandDurationForTesting(), 350);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testScrollDuration_Large() {
        initializeTest(false, true, 3);

        // Act: Set scroll offset greater than 1920
        mStripLayoutHelper.testSetScrollOffset(2000);

        // Assert: Expand duration is 450.
        assertEquals(mStripLayoutHelper.getExpandDurationForTesting(), 450);
    }

    @Test
    public void testOnDown_OnNewTabButton() {
        // Initialize.
        initializeTest(false, false, 0);

        // Set new tab button location and dimensions.
        mStripLayoutHelper.getNewTabButton().setX(NEW_TAB_BTN_X);
        mStripLayoutHelper.getNewTabButton().setY(NEW_TAB_BTN_Y);
        mStripLayoutHelper.getNewTabButton().setWidth(NEW_TAB_BTN_WIDTH);
        mStripLayoutHelper.getNewTabButton().setHeight(NEW_TAB_BTN_HEIGHT);

        // Press down on new tab button.
        // CenterX = getX() + (getWidth() / 2) = 700 + (100 / 2) = 750
        // CenterY = getY() + (getHeight() / 2) = 1400 + (100 / 2) = 1450
        mStripLayoutHelper.onDown(TIMESTAMP, 750f, 1450f, false, 0);

        // Verify.
        assertTrue("New tab button should be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertNull("Should not set an interacting tab when pressing down on new tab button.",
                mStripLayoutHelper.getInteractingTab());
        assertFalse("Should not start reorder mode when pressing down on new tab button.",
                mStripLayoutHelper.getInReorderModeForTesting());
    }

    @Test
    public void testOnDown_OnTab() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Press down on second tab.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(false);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onDown(TIMESTAMP, 150f, 0f, false, 0);

        // Verify.
        assertFalse("New tab button should not be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertEquals("Second tab should be interacting tab.", tabs[1],
                mStripLayoutHelper.getInteractingTab());
        assertFalse("Should not start reorder mode when pressing down on tab without mouse.",
                mStripLayoutHelper.getInReorderModeForTesting());
        verify(tabs[1], never()).setClosePressed(anyBoolean());
    }

    @Test
    public void testOnDown_OnTab_WithMouse() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Press down on second tab with mouse.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(false);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onDown(TIMESTAMP, 150f, 0f, true, 0);

        // Verify.
        assertFalse("New tab button should not be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertEquals("Second tab should be interacting tab.", tabs[1],
                mStripLayoutHelper.getInteractingTab());
        assertTrue("Should start reorder mode when pressing down on tab with mouse.",
                mStripLayoutHelper.getInReorderModeForTesting());
        verify(tabs[1], never()).setClosePressed(anyBoolean());
    }

    @Test
    public void testOnDown_OnTabCloseButton() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Press down on second tab's close button.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(true);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onDown(TIMESTAMP, 150f, 0f, false, 0);

        // Verify.
        assertFalse("New tab button should not be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertEquals("Second tab should be interacting tab.", tabs[1],
                mStripLayoutHelper.getInteractingTab());
        assertFalse("Should not start reorder mode from close button.",
                mStripLayoutHelper.getInReorderModeForTesting());
        verify(tabs[1]).setClosePressed(eq(true));
    }

    @Test
    public void testOnDown_OnTabCloseButton_WithMouse() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Press down on second tab's close button with mouse.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(true);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onDown(TIMESTAMP, 150f, 0f, true, 0);

        // Verify.
        assertFalse("New tab button should not be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertEquals("Second tab should be interacting tab.", tabs[1],
                mStripLayoutHelper.getInteractingTab());
        assertFalse("Should not start reorder mode from close button.",
                mStripLayoutHelper.getInReorderModeForTesting());
        verify(tabs[1]).setClosePressed(eq(true));
    }

    @Test
    public void testOnDown_WhileScrolling() {
        // Initialize and assert scroller is finished.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        assertTrue("Scroller should be finished right after initializing.",
                mStripLayoutHelper.getScroller().isFinished());

        // Start scroll and assert scroller is not finished.
        mStripLayoutHelper.getScroller().startScroll(0, 0, 0, 0, TIMESTAMP, 1000);
        assertFalse("Scroller should not be finished after starting scroll.",
                mStripLayoutHelper.getScroller().isFinished());

        // Press down on second tab and assert scroller is finished.
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onDown(TIMESTAMP, 150f, 0f, false, 0);
        assertFalse("New tab button should not be pressed.",
                mStripLayoutHelper.getNewTabButton().isPressed());
        assertNull("Should not set an interacting tab when pressing down to stop scrolling.",
                mStripLayoutHelper.getInteractingTab());
        assertTrue("Scroller should be force finished after pressing down on strip.",
                mStripLayoutHelper.getScroller().isFinished());
    }

    @Test
    public void testOnLongPress_OnTab() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Long press on second tab.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(false);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onLongPress(TIMESTAMP, 150f, 0f);

        // Verify that we enter reorder mode.
        assertTrue("Should be in reorder mode after long press on tab.",
                mStripLayoutHelper.getInReorderModeForTesting());
        assertFalse("Should not show tab menu after long press on tab.",
                mStripLayoutHelper.isTabMenuShowing());
    }

    @Test
    public void testOnLongPress_OnCloseButton() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Mock tab's view.
        View tabView = new View(mActivity);
        tabView.setLayoutParams(new MarginLayoutParams(150, 50));
        when(mModel.getTabAt(1).getView()).thenReturn(tabView);

        // Long press on second tab's close button.
        when(tabs[1].checkCloseHitTest(anyFloat(), anyFloat())).thenReturn(true);
        mStripLayoutHelper.setTabAtPositionForTesting(tabs[1]);
        mStripLayoutHelper.onLongPress(TIMESTAMP, 150f, 0f);

        // Verify that we show the "Close all tabs" popup menu.
        assertFalse("Should not be in reorder mode after long press on tab close button.",
                mStripLayoutHelper.getInReorderModeForTesting());
        assertTrue("Should show tab menu after long press on tab close button.",
                mStripLayoutHelper.isTabMenuShowing());
    }

    @Test
    public void testOnLongPress_OffTab() {
        // Initialize.
        initializeTest(false, false, 0);
        StripLayoutTab[] tabs = getMockedStripLayoutTabs(150f);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);

        // Long press past the last tab.
        mStripLayoutHelper.setTabAtPositionForTesting(null);
        mStripLayoutHelper.onLongPress(TIMESTAMP, 150f, 0f);

        // Verify that we show the "Close all tabs" popup menu.
        assertFalse("Should not be in reorder mode after long press on empty space on tab strip.",
                mStripLayoutHelper.getInReorderModeForTesting());
        assertFalse("Should not show after long press on empty space on tab strip.",
                mStripLayoutHelper.isTabMenuShowing());
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_BetweenTabs() {
        // Initialize with 3 tabs.
        initializeTest(false, false, 0, 3);

        // Start reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);

        // Verify no tabs have a trailing margin, since there are no tab groups.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_NO_MARGIN, tabs[0].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[1].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[2].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_TabToLeft() {
        // Mock 1 tab to the left of a tab group with 3 tabs.
        initializeTest(false, false, 0, 4);
        groupTabs(1, 4);

        // Start reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);

        // Verify the leftmost tab has a trailing margin.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_MARGIN, tabs[0].getTrailingMargin(), TAB_MARGIN_WIDTH, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[1].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[2].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[3].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_TabToRight() {
        // Mock 1 tab to the right of a tab group with 3 tabs.
        initializeTest(false, false, 0, 4);
        groupTabs(0, 3);

        // Start reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);

        // Verify the rightmost tab in the tab group has a trailing margin.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_NO_MARGIN, tabs[0].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[1].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_MARGIN, tabs[2].getTrailingMargin(), TAB_MARGIN_WIDTH, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[3].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_BetweenGroups() {
        // Mock a tab group with 2 tabs to the left of a tab group with 3 tabs.
        initializeTest(false, false, 0, 5);
        groupTabs(0, 2);
        groupTabs(2, 5);

        // Start reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);

        // Verify the rightmost tab in the first group has a trailing margin.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_NO_MARGIN, tabs[0].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_MARGIN, tabs[1].getTrailingMargin(), TAB_MARGIN_WIDTH, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[2].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[3].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[4].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_BetweenGroups_Rtl() {
        // Mock a tab group with 2 tabs to the right of a tab group with 3 tabs.
        initializeTest(true, false, 0, 5);
        groupTabs(0, 2);
        groupTabs(2, 5);

        // Start reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);

        // Verify the leftmost tab in the first group has a trailing margin.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_NO_MARGIN, tabs[0].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_MARGIN, tabs[1].getTrailingMargin(), TAB_MARGIN_WIDTH, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[2].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[3].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[4].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_ResetMarginsOnStopReorder() {
        // Mock 1 tab to the left of a tab group with 3 tabs.
        initializeTest(false, false, 0, 4);
        groupTabs(1, 4);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);

        // Start then stop reorder.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);
        mStripLayoutHelper.stopReorderModeForTesting();

        // Verify no tabs have a trailing margin when reordering is stopped.
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        assertEquals(EXPECTED_NO_MARGIN, tabs[0].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[1].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[2].getTrailingMargin(), 0f, EPSILON);
        assertEquals(EXPECTED_NO_MARGIN, tabs[3].getTrailingMargin(), 0f, EPSILON);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_NoScrollOnReorder() {
        // Mock 1 tab to the right of 2 tab groups with 2 tabs each.
        initializeTest(false, false, 0, 5);
        groupTabs(0, 2);
        groupTabs(2, 4);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(0);

        // Start reorder on leftmost tab. No margins to left of tab, so shouldn't scroll.
        // Verify the scroll offset is still 0.
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);
        assertEquals("There are no margins left of the selected tab, so we shouldn't scroll.", 0,
                mStripLayoutHelper.getScrollOffset());

        // Stop reorder. Verify the scroll offset is still 0.
        mStripLayoutHelper.stopReorderModeForTesting();
        assertEquals("Scroll offset should return to 0 after stopping reorder mode.", 0,
                mStripLayoutHelper.getScrollOffset());
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testTabGroupMargins_ScrollOnReorder() {
        // Mock 1 tab to the right of 2 tab groups with 2 tabs each.
        initializeTest(false, false, 0, 5);
        groupTabs(0, 2);
        groupTabs(2, 4);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(0);

        // Start reorder on rightmost tab. 2 margins to left of tab, so should scroll.
        // Verify the scroll offset is 2 * (-marginWidth) = 2 * -95 = -190
        int expectedOffset = -190;
        mStripLayoutHelper.startReorderModeAtIndexForTesting(4);
        assertEquals("There are margins left of the selected tab, so we should scroll",
                expectedOffset, mStripLayoutHelper.getScrollOffset());

        // Stop reorder. Verify the scroll offset is once again 0.
        mStripLayoutHelper.stopReorderModeForTesting();
        assertEquals("Scroll offset should return to 0 after stopping reorder mode.", 0,
                mStripLayoutHelper.getScrollOffset());
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testReorder_NoGroups() {
        // Mock 5 tabs.
        initializeTest(false, false, 0, 5);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        StripLayoutTab thirdTab = tabs[2];
        StripLayoutTab fourthTab = tabs[3];

        // Start reorder on third tab. Drag right to trigger swap with fourth tab.
        // 100 > tabWidth * flipThreshold = (190-24) * 0.53 = 88
        mStripLayoutHelper.startReorderModeAtIndexForTesting(2);
        float dragDistance = 100f;
        float startX = mStripLayoutHelper.getLastReorderX();
        mStripLayoutHelper.drag(TIMESTAMP, startX + dragDistance, 0f, dragDistance, 0f, 0f, 0f);

        // Assert the tabs swapped.
        assertEquals("Third and fourth tabs should have swapped.", thirdTab, tabs[3]);
        assertEquals("Third and fourth tabs should have swapped.", fourthTab, tabs[2]);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testReorder_DragOutOfGroup() {
        // Mock a tab group with 3 tabs with 1 tab to the left and 1 tab to the right.
        initializeTest(false, false, 0, 5);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        StripLayoutTab fourthTab = tabs[3];
        groupTabs(1, 4);

        // Start reorder on fourth tab. Drag right over the tab group.
        // 60 > marginWidth * flipThreshold = 95 * 0.53 = 51
        mStripLayoutHelper.startReorderModeAtIndexForTesting(3);
        float dragDistance = 60f;
        float startX = mStripLayoutHelper.getLastReorderX();
        mStripLayoutHelper.drag(TIMESTAMP, startX + dragDistance, 0f, dragDistance, 0f, 0f, 0f);

        // Verify fourth tab was dragged out of group, but not reordered.
        assertEquals("Fourth tab should not have moved.", fourthTab, tabs[3]);
        verify(mTabGroupModelFilter).moveTabOutOfGroupInDirection(fourthTab.getId(), true);
    }

    @Test
    @Feature("Tab Groups on Tab Strip")
    public void testReorder_DragPastGroup() {
        // Mock a tab group with 3 tabs with 1 tab to the left and 1 tab to the right.
        initializeTest(false, false, 0, 5);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        StripLayoutTab[] tabs = mStripLayoutHelper.getStripLayoutTabs();
        StripLayoutTab firstTab = tabs[0];
        groupTabs(1, 4);

        // Start reorder on first tab. Drag right over the tab group.
        // 650 > 3*tabWidth + margin + flipThreshold*margin = 3*(190-24) + 1.53*95 = 644 > 300
        mStripLayoutHelper.startReorderModeAtIndexForTesting(0);
        float dragDistance = 300f;
        float startX = mStripLayoutHelper.getLastReorderX();
        mStripLayoutHelper.drag(TIMESTAMP, startX + dragDistance, 0f, dragDistance, 0f, 0f, 0f);
        // Verify no reordering, since we have not hovered over the tab group long enough.
        assertEquals("First tab should not have moved.", firstTab, tabs[0]);

        // Drag past the tab group.
        dragDistance = 650f;
        mStripLayoutHelper.drag(TIMESTAMP, startX + dragDistance, 0f, dragDistance, 0f, 0f, 0f);
        // Verify reordering, since we have dragged past the tab group.
        assertEquals("First tab should now be the fourth tab.", firstTab.getId(), tabs[3].getId());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabClosing_NoTabResize() {
        // Arrange
        int tabCount = 10;
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, false, 9, tabCount);
        StripLayoutTab[] tabs = getRealStripLayoutTabs(TAB_WIDTH_MEDIUM, tabCount);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        setupForAnimations();

        // Assert: New tab button position before starting tab closure.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        assertEquals("Unexpected starting newTabButton position.", 764.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);

        // Act: Call on close tab button handler.
        mStripLayoutHelper.handleCloseButtonClick(tabs[9], TIMESTAMP);

        // Assert: Animations started.
        assertTrue("MultiStepAnimations should have started.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());

        // Act: End the tab closing animations to apply final values.
        CompositorAnimator runningAnimator =
                (CompositorAnimator) mStripLayoutHelper.getRunningAnimatorForTesting();
        runningAnimator.end();

        // Assert: Tab is closed and animations are still running. New button position isn't
        // modified.
        int expectedTabCount = 9;
        assertEquals("Unexpected tabs count.", expectedTabCount,
                mStripLayoutHelper.getStripLayoutTabs().length);
        assertTrue("MultiStepAnimations should still be running.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());
        assertEquals("NewTabButton should not have moved.", 764.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);

        // Act: End next set of animations to apply final values.
        mStripLayoutHelper.getRunningAnimatorForTesting().end();

        // Assert: Animations completed. The tab width is not resized and drawX does not change.
        float expectedDrawX =
                -460.f; // Since we are focused on the last tab, start tabs are off screen.
        StripLayoutTab[] updatedTabs = mStripLayoutHelper.getStripLayoutTabs();
        for (StripLayoutTab stripTab : updatedTabs) {
            assertEquals("Unexpected tab width after resize.", 156.f, stripTab.getWidth(), 0.0);
            assertEquals("Unexpected tab position.", expectedDrawX, stripTab.getDrawX(), 0.0);
            expectedDrawX += (TAB_WIDTH_MEDIUM - TAB_OVERLAP_WIDTH);
        }
        assertEquals("NewTabButton should not have moved.", 764.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);
        assertFalse("MultiStepAnimations should have stopped running.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testTabClosing_NonLastTab_TabResize_NewTabButtonMoved() {
        // Arrange
        int tabCount = 4;
        TabUiFeatureUtilities.setTabMinWidthForTesting(TAB_WIDTH_MEDIUM);
        initializeTest(false, false, 4, tabCount);
        StripLayoutTab[] tabs = getRealStripLayoutTabs(TAB_WIDTH_MEDIUM, tabCount);
        mStripLayoutHelper.setStripLayoutTabsForTest(tabs);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        setupForAnimations();

        // Assert: New tab button position before starting tab closure.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        assertEquals("Unexpected starting newTabButton position.", 764.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);

        // Act: Call on close tab button handler.
        mStripLayoutHelper.handleCloseButtonClick(tabs[2], TIMESTAMP);

        // Assert: Animations started.
        assertTrue("MultiStepAnimations should have started.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());

        // Act: End the animations to apply final values.
        CompositorAnimator runningAnimator =
                (CompositorAnimator) mStripLayoutHelper.getRunningAnimatorForTesting();
        runningAnimator.end();

        // Assert: Tab is closed and animations are still running. New tab button position is not
        // modified.
        int expectedTabCount = 3;
        assertEquals(expectedTabCount, mStripLayoutHelper.getStripLayoutTabs().length);
        assertTrue("MultiStepAnimations should still be running.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());
        assertEquals("NewTabButton should not have moved.", 764.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);

        // Act: Set animation time forward by 250ms for next set of animations.
        mStripLayoutHelper.getRunningAnimatorForTesting().end();

        // Assert: Animations completed. The tab width is resized, tab.drawX is changed and
        // newTabButton.drawX is also changed.
        float expectedDrawX = 0.f;
        float expectedWidthAfterResize = 265.f;
        StripLayoutTab[] updatedTabs = mStripLayoutHelper.getStripLayoutTabs();
        for (int i = 0; i < updatedTabs.length; i++) {
            StripLayoutTab stripTab = updatedTabs[i];
            assertEquals("Unexpected tab width after resize.", expectedWidthAfterResize,
                    stripTab.getWidth(), 0.0);
            assertEquals("Unexpected tab position.", expectedDrawX, stripTab.getDrawX(), 0.0);
            expectedDrawX += (expectedWidthAfterResize - TAB_OVERLAP_WIDTH);
        }
        assertEquals("NewTabButton position is incorrect.", 759.f,
                mStripLayoutHelper.getNewTabButton().getX(), 0.0f);
        assertFalse("MultiStepAnimations should have ended.",
                mStripLayoutHelper.isMultiStepCloseAnimationsRunning());
    }

    @Test
    public void testFlingLeft() {
        // Arrange
        initializeTest(false, false, 9, 10);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(-150);

        // Act: Perform a fling and update layout.
        float velocityX = -7000f;
        // The velocityX value is used to calculate the scroller.finalX value.
        mStripLayoutHelper.fling(TIMESTAMP, 0, 0, velocityX, 0);
        // This will use the scroller.finalX value to update the scrollOffset. The timestamp
        // value here will determine the fling duration and affects the final offset value.
        mStripLayoutHelper.updateLayout(TIMESTAMP + 10);

        // Assert: Final scrollOffset.
        // The calculation of this value is done using the velocity. The velocity along a friction
        // constant is used to calculate deceleration and distance. That together with the animation
        // duration determines the final scroll offset position.
        float expectedOffset = -220.f;
        assertEquals("Unexpected scroll offset.", expectedOffset,
                mStripLayoutHelper.getScrollOffset(), 0.0);
    }

    @Test
    public void testFlingRight() {
        // Arrange
        initializeTest(false, false, 9, 10);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // When updateLayout is called for the first time, bringSelectedTabToVisibleArea() method is
        // invoked. That also affects the scrollOffset value. So we call updateLayout before
        // performing a fling so that bringSelectedTabToVisible area isn't called after the fling.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(-150);

        // Act: Perform a fling and update layout.
        float velocity = 5000f;
        // The velocityX value is used to calculate the scroller.finalX value.
        mStripLayoutHelper.fling(TIMESTAMP, 0, 0, velocity, 0);
        // This will use the scroller.finalX value to update the scrollOffset. The timestamp
        // value here will determine the fling duration and affects the final offset value.
        mStripLayoutHelper.updateLayout(TIMESTAMP + 20);

        // Assert: Final scrollOffset.
        // The calculation of this value is done using the velocity. The velocity along a friction
        // constant is used to calculate deceleration and distance. That together with the animation
        // duration determines the final scroll offset position.
        float expectedOffset = -50.f;
        assertEquals("Unexpected scroll offset.", expectedOffset,
                mStripLayoutHelper.getScrollOffset(), 0.0);
    }

    @Test
    @Feature("Tab Strip Improvements")
    public void testDrag_UpdatesScrollOffset_ScrollingStrip() {
        // Arrange
        initializeTest(false, false, 9, 10);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // When updateLayout is called for the first time, bringSelectedTabToVisibleArea() method is
        // invoked. That also affects the scrollOffset value. So we call updateLayout before
        // performing a fling so that bringSelectedTabToVisible area isn't called after the fling.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(-150);

        // Act: Drag and update layout.
        float dragDeltaX = -200.f;
        float totalY = 85.f; // totalY > 50.f to cross reorder threshold.
        mStripLayoutHelper.drag(TIMESTAMP, 374.74f, 24.276f, dragDeltaX, -0.304f, -16.078f, totalY);

        float expectedOffset = -350; // mScrollOffset + dragDeltaX = -200 - 150 = -350
        // Assert scroll offset position.
        assertEquals("Unexpected scroll offset.", expectedOffset,
                mStripLayoutHelper.getScrollOffset(), 0.0);
        // Reorder mode is disabled for scrolling strip.
        assertFalse(mStripLayoutHelper.isInReorderModeForTesting());
    }

    @Test
    public void testDrag_UpdatesScrollOffset_CascadingStrip() {
        // Arrange
        CachedFeatureFlags.setForTesting(ChromeFeatureList.TAB_STRIP_IMPROVEMENTS, false);
        initializeTest(false, false, 0, 10);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // When updateLayout is called for the first time, bringSelectedTabToVisibleArea() method is
        // invoked. That also affects the scrollOffset value. So we call updateLayout before
        // performing a fling so that bringSelectedTabToVisible area isn't called after the fling.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(-250);

        // Act: Drag and update layout.
        float dragDeltaX = -200.f;
        mStripLayoutHelper.drag(
                TIMESTAMP, 374.74f, 24.276f, dragDeltaX, -0.304f, -16.078f, -4.476f);

        // Assert
        float expectedOffset = -450; // mScrollOffset + dragDeltaX = -200 - 250 = -450
        assertEquals("Unexpected scroll offset.", expectedOffset,
                mStripLayoutHelper.getScrollOffset(), 0.0);
        assertFalse("Reorder mode should not enabled when totalY <= 50.",
                mStripLayoutHelper.isInReorderModeForTesting());
    }

    @Test
    public void testDrag_ReorderMode_CascadingStrip() {
        // Arrange
        CachedFeatureFlags.setForTesting(ChromeFeatureList.TAB_STRIP_IMPROVEMENTS, false);
        initializeTest(false, false, 5, 10);
        mStripLayoutHelper.onSizeChanged(SCREEN_WIDTH, SCREEN_HEIGHT, false, TIMESTAMP);
        // When updateLayout is called for the first time, bringSelectedTabToVisibleArea() method is
        // invoked. That also affects the scrollOffset value. So we call updateLayout before
        // performing a fling so that bringSelectedTabToVisible area isn't called after the fling.
        mStripLayoutHelper.updateLayout(TIMESTAMP);
        mStripLayoutHelper.setScrollOffsetForTesting(-250);

        // Assert: Ensure reorder mode is disabled when starting drag.
        assertFalse("Reorder mode should be disabled before drag.",
                mStripLayoutHelper.isInReorderModeForTesting());

        // Act
        float dragDeltaX = -200.f;
        float totalY = 75.f; // Drag with totalY > 50.f to cross reorder mode threshold.
        mStripLayoutHelper.drag(TIMESTAMP, 374.74f, 24.276f, dragDeltaX, -0.304f, -16.078f, totalY);

        // Assert: Reorder mode is enabled.
        assertTrue("Reorder mode was not enabled after drag.",
                mStripLayoutHelper.isInReorderModeForTesting());
        float expectedOffset = -450; // mScrollOffset + dragDeltaX = -200 - 250 = -450
        assertEquals("Unexpected scroll offset.", expectedOffset,
                mStripLayoutHelper.getScrollOffset(), 0.0);
    }

    private void setupForAnimations() {
        CompositorAnimationHandler mHandler = new CompositorAnimationHandler(() -> {});
        // CompositorAnimationHandler.setTestingMode(true);
        when(mUpdateHost.getAnimationHandler()).thenReturn(mHandler);

        // Update layout when updateHost.requestUpdate is called.
        doAnswer(invocation -> {
            mStripLayoutHelper.updateLayout(TIMESTAMP);
            return null;
        })
                .when(mUpdateHost)
                .requestUpdate();
    }

    private void initializeTest(boolean rtl, boolean incognito, int tabIndex, int numTabs) {
        mStripLayoutHelper = createStripLayoutHelper(rtl, incognito);
        mIncognito = incognito;
        if (numTabs <= 5) {
            for (int i = 0; i < numTabs; i++) {
                mModel.addTab(TEST_TAB_TITLES[i]);
                when(mModel.getTabAt(i).isHidden()).thenReturn(tabIndex != i);
                when(mTabGroupModelFilter.getRootId(eq(mModel.getTabAt(i)))).thenReturn(i);
            }
        } else {
            for (int i = 0; i < numTabs; i++) {
                mModel.addTab("Tab " + i);
                when(mModel.getTabAt(i).isHidden()).thenReturn(tabIndex != i);
                when(mTabGroupModelFilter.getRootId(eq(mModel.getTabAt(i)))).thenReturn(i);
            }
        }
        mModel.setIndex(tabIndex);
        mStripLayoutHelper.setTabModel(mModel, null);
        mStripLayoutHelper.setTabGroupModelFilter(mTabGroupModelFilter);
        mStripLayoutHelper.tabSelected(0, tabIndex, 0, false);
        // Flush UI updated
    }

    private void initializeTest(boolean rtl, boolean incognito, int tabIndex) {
        initializeTest(rtl, incognito, tabIndex, 5);
    }

    private void assertTabStripAndOrder(String[] expectedAccessibilityDescriptions) {
        // Each tab has a "close button", and there is one additional "new tab" button
        final int expectedNumberOfViews = 2 * expectedAccessibilityDescriptions.length + 1;

        final List<VirtualView> views = new ArrayList<>();
        mStripLayoutHelper.getVirtualViews(views);
        assertEquals(expectedNumberOfViews, views.size());

        // Tab titles
        for (int i = 0; i < expectedNumberOfViews - 1; i++) {
            final String expectedDescription = i % 2 == 0
                    ? expectedAccessibilityDescriptions[i / 2]
                    : String.format(CLOSE_TAB, TEST_TAB_TITLES[i / 2]);
            assertEquals(expectedDescription, views.get(i).getAccessibilityDescription());
        }

        assertEquals(mActivity.getResources().getString(mIncognito
                                     ? R.string.accessibility_toolbar_btn_new_incognito_tab
                                     : R.string.accessibility_toolbar_btn_new_tab),
                views.get(views.size() - 1).getAccessibilityDescription());
    }

    private StripLayoutHelper createStripLayoutHelper(boolean rtl, boolean incognito) {
        LocalizationUtils.setRtlForTesting(rtl);
        final StripLayoutHelper stripLayoutHelper = new StripLayoutHelper(
                mActivity, mUpdateHost, mRenderHost, incognito, mModelSelectorBtn);
        // Initialize StackScroller
        stripLayoutHelper.onContextChanged(mActivity);
        return stripLayoutHelper;
    }

    private String[] getExpectedAccessibilityDescriptions(int tabIndex) {
        final String[] expectedAccessibilityDescriptions = new String[TEST_TAB_TITLES.length];
        for (int i = 0; i < TEST_TAB_TITLES.length; i++) {
            final boolean isHidden = (i != tabIndex);
            String suffix;
            if (mIncognito) {
                suffix = isHidden ? INCOGNITO_IDENTIFIER : INCOGNITO_IDENTIFIER_SELECTED;
            } else {
                suffix = isHidden ? IDENTIFIER : IDENTIFIER_SELECTED;
            }
            String expectedDescription = "";
            if (!TextUtils.isEmpty(TEST_TAB_TITLES[i])) {
                expectedDescription += TEST_TAB_TITLES[i] + ", ";
            }
            expectedAccessibilityDescriptions[i] = expectedDescription + suffix;
        }
        return expectedAccessibilityDescriptions;
    }

    private StripLayoutTab[] getMockedStripLayoutTabs(float tabWidth, float mDrawX, int numTabs) {
        StripLayoutTab[] tabs = new StripLayoutTab[mModel.getCount()];

        for (int i = 0; i < numTabs; i++) {
            tabs[i] = mockStripTab(i, tabWidth, mDrawX);
        }

        return tabs;
    }

    private StripLayoutTab[] getMockedStripLayoutTabs(float tabWidth) {
        return getMockedStripLayoutTabs(tabWidth, 0.f, 5);
    }

    private StripLayoutTab[] getRealStripLayoutTabs(float tabWidth, int numTabs) {
        StripLayoutTab[] tabs = new StripLayoutTab[mModel.getCount()];
        for (int i = 0; i < numTabs; i++) {
            tabs[i] = getRealStripTab(i, tabWidth, i * (tabWidth - TAB_OVERLAP_WIDTH));
        }
        return tabs;
    }

    private StripLayoutTab mockStripTab(int id, float tabWidth, float mDrawX) {
        StripLayoutTab tab = mock(StripLayoutTab.class);
        when(tab.getWidth()).thenReturn(tabWidth);
        when(tab.getId()).thenReturn(id);
        when(tab.getDrawX()).thenReturn(mDrawX);
        when(tab.getVisiblePercentage()).thenReturn(1f);
        return tab;
    }

    private StripLayoutTab getRealStripTab(int id, float tabWidth, float mDrawX) {
        Context context = mock(Context.class);
        Resources res = mock(Resources.class);
        DisplayMetrics dm = new DisplayMetrics();
        dm.widthPixels = Math.round(SCREEN_WIDTH);
        dm.heightPixels = Math.round(SCREEN_HEIGHT);
        when(res.getDisplayMetrics()).thenReturn(dm);
        when(context.getResources()).thenReturn(res);

        StripLayoutTabDelegate delegate = mock(StripLayoutTabDelegate.class);
        TabLoadTracker.TabLoadTrackerCallback loadTrackerCallback =
                mock(TabLoadTrackerCallback.class);

        StripLayoutTab tab = new StripLayoutTab(
                context, id, delegate, loadTrackerCallback, mRenderHost, mUpdateHost, false);
        tab.setWidth(tabWidth);
        tab.setDrawX(mDrawX);
        return tab;
    }

    /**
     * Mock that the sequence of tabs from startIndex to endIndex are part of that same tab group.
     * @param startIndex The index where we start including tabs in the group (inclusive).
     * @param endIndex The index where we stop including tabs in the group (exclusive).
     */
    private void groupTabs(int startIndex, int endIndex) {
        int groupRootId = mModel.getTabAt(startIndex).getId();
        int numTabs = endIndex - startIndex;

        for (int i = startIndex; i < endIndex; i++) {
            when(mTabGroupModelFilter.hasOtherRelatedTabs(eq(mModel.getTabAt(i)))).thenReturn(true);
            when(mTabGroupModelFilter.getRootId(eq(mModel.getTabAt(i)))).thenReturn(groupRootId);
        }
        when(mTabGroupModelFilter.getRelatedTabCountForRootId(eq(groupRootId))).thenReturn(numTabs);
    }
}

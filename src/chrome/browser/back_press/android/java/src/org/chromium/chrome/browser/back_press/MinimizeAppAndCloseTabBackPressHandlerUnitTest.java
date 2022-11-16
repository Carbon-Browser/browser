// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.back_press;

import android.os.Build.VERSION_CODES;

import androidx.test.filters.SmallTest;

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.Callback;
import org.chromium.base.FeatureList;
import org.chromium.base.FeatureList.TestValues;
import org.chromium.base.Predicate;
import org.chromium.base.UserDataHost;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.MetricsUtils.HistogramDelta;
import org.chromium.chrome.browser.back_press.MinimizeAppAndCloseTabBackPressHandler.MinimizeAppAndCloseTabType;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAssociatedApp;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

/**
 * Unit tests for {@link MinimizeAppAndCloseTabBackPressHandler}.
 */
@Batch(Batch.UNIT_TESTS)
@RunWith(BaseJUnit4ClassRunner.class)
public class MinimizeAppAndCloseTabBackPressHandlerUnitTest {
    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @Mock
    private TabModelSelector mTabModelSelector;

    @Mock
    private Callback<Tab> mSendToBackground;

    @Mock
    private Predicate<Tab> mShouldCloseTab;

    private MinimizeAppAndCloseTabBackPressHandler mHandler;
    private ObservableSupplierImpl<TabModelSelector> mTabModelSelectorObservableSupplier;

    @BeforeClass
    public static void setUpClass() {
        ObservableSupplierImpl.setIgnoreThreadChecksForTesting(true);
    }

    @AfterClass
    public static void afterClass() {
        ObservableSupplierImpl.setIgnoreThreadChecksForTesting(false);
    }

    @Before
    public void setUp() {
        createBackPressHandler();
    }

    @After
    public void tearDown() {
        MinimizeAppAndCloseTabBackPressHandler.setVersionForTesting(null);
    }

    @Test
    @SmallTest
    public void testMinimizeAppAndCloseTab() {
        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.MINIMIZE_APP_AND_CLOSE_TAB);
        Tab tab = Mockito.mock(Tab.class);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(tab);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });
        Mockito.when(mShouldCloseTab.test(tab)).thenReturn(true);
        UserDataHost userDataHost = new UserDataHost();
        Mockito.when(tab.getUserDataHost()).thenReturn(userDataHost);
        TabAssociatedApp.from(tab);
        Mockito.when(tab.getLaunchType()).thenReturn(TabLaunchType.FROM_EXTERNAL_APP);
        mHandler.handleBackPress();

        Mockito.verify(mSendToBackground,
                       Mockito.description("App should be minimized with tab being closed"))
                .onResult(tab);
        Assert.assertEquals(1, d1.getDelta());
    }

    @Test
    @SmallTest
    public void testMinimizeAppAndCloseTab_SystemBack() {
        createBackPressHandler(true);
        // Expect no change.
        testMinimizeAppAndCloseTab();
    }

    @Test
    @SmallTest
    public void testCloseTab() {
        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.CLOSE_TAB);
        Tab tab = Mockito.mock(Tab.class);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(tab);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });
        Mockito.when(mShouldCloseTab.test(tab)).thenReturn(true);
        UserDataHost userDataHost = new UserDataHost();
        Mockito.when(tab.getUserDataHost()).thenReturn(userDataHost);
        TabAssociatedApp.from(tab);
        Mockito.when(tab.getLaunchType()).thenReturn(TabLaunchType.FROM_LONGPRESS_FOREGROUND);
        mHandler.handleBackPress();

        Mockito.verify(mSendToBackground,
                       Mockito.never().description(
                               "Tab should be closed without minimizing the app."))
                .onResult(tab);
        Assert.assertEquals(1, d1.getDelta());
    }

    @Test
    @SmallTest
    public void testCloseTab_SystemBack() {
        createBackPressHandler(true);
        // Expect no change.
        testCloseTab();
    }

    @Test
    @SmallTest
    public void testMinimizeApp() {
        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.MINIMIZE_APP);
        Tab tab = Mockito.mock(Tab.class);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(tab);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });
        Mockito.when(mShouldCloseTab.test(tab)).thenReturn(false);
        mHandler.handleBackPress();

        Mockito.verify(mSendToBackground,
                       Mockito.description("App should be minimized without closing any tab"))
                .onResult(null);
        Assert.assertEquals(1, d1.getDelta());
    }

    @Test
    @SmallTest
    public void testMinimizeApp_SystemBack() {
        createBackPressHandler(true);

        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.MINIMIZE_APP);
        Tab tab = Mockito.mock(Tab.class);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(tab);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });
        Mockito.when(mShouldCloseTab.test(tab)).thenReturn(false);

        Assert.assertFalse("Back press should be handled by OS.",
                mHandler.getHandleBackPressChangedSupplier().get());
        Assert.assertEquals(0, d1.getDelta());
    }

    @Test
    @SmallTest
    public void testMinimizeApp_NoValidTab() {
        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.MINIMIZE_APP);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(null);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });
        mHandler.handleBackPress();

        Mockito.verify(mSendToBackground).onResult(null);
        Mockito.verify(mSendToBackground,
                       Mockito.description("App should be minimized without closing any tab"))
                .onResult(null);
        Assert.assertEquals(1, d1.getDelta());
    }

    @Test
    @SmallTest
    public void testMinimizeApp_NoValidTab_SystemBack() {
        createBackPressHandler(true);

        HistogramDelta d1 = new HistogramDelta(MinimizeAppAndCloseTabBackPressHandler.HISTOGRAM,
                MinimizeAppAndCloseTabType.MINIMIZE_APP);
        Mockito.when(mTabModelSelector.getCurrentTab()).thenReturn(null);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier.set(mTabModelSelector); });

        Assert.assertFalse("Back press should be handled by OS.",
                mHandler.getHandleBackPressChangedSupplier().get());

        thrown.expect(Matchers.instanceOf(AssertionError.class));
        thrown.expectMessage(
                "Should be disabled when there is no valid tab and back press will be consumed.");

        mHandler.handleBackPress();

        Mockito.verify(mSendToBackground, Mockito.never()).onResult(Mockito.any());
        Mockito.verify(mShouldCloseTab, Mockito.never()).test(Mockito.any());
        Assert.assertEquals(0, d1.getDelta());
    }

    private void createBackPressHandler() {
        createBackPressHandler(false);
    }

    private void createBackPressHandler(boolean systemBack) {
        TestValues testValues = new TestValues();
        testValues.addFeatureFlagOverride(ChromeFeatureList.BACK_GESTURE_REFACTOR, true);
        testValues.addFieldTrialParamOverride(
                ChromeFeatureList.BACK_GESTURE_REFACTOR, "system_back", systemBack + "");
        FeatureList.setTestValues(testValues);
        if (systemBack) {
            MinimizeAppAndCloseTabBackPressHandler.setVersionForTesting(VERSION_CODES.S);
        }
        TestThreadUtils.runOnUiThreadBlocking(
                () -> { mTabModelSelectorObservableSupplier = new ObservableSupplierImpl<>(); });
        mHandler = TestThreadUtils.runOnUiThreadBlockingNoException(
                ()
                        -> new MinimizeAppAndCloseTabBackPressHandler(
                                mTabModelSelectorObservableSupplier, mShouldCloseTab,
                                mSendToBackground));
    }
}

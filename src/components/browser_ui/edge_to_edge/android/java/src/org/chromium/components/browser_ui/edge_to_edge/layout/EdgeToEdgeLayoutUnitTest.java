// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.browser_ui.edge_to_edge.layout;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.graphics.Rect;
import android.view.View;
import android.view.View.MeasureSpec;
import android.widget.FrameLayout;

import androidx.core.graphics.Insets;
import androidx.core.view.WindowInsetsCompat;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.ui.InsetObserver;
import org.chromium.ui.InsetObserver.WindowInsetsConsumer.InsetConsumerSource;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.test.util.WindowInsetsTestUtils.SpyWindowInsetsBuilder;

@RunWith(BaseRobolectricTestRunner.class)
public class EdgeToEdgeLayoutUnitTest {
    private static final int STATUS_BAR_SIZE = 100;
    private static final int NAV_BAR_SIZE = 150;
    private static final int CAPTION_BAR_SIZE = 180;
    private static final int CUTOUT_SIZE = 75;

    private static final int STATUS_BARS = WindowInsetsCompat.Type.statusBars();
    private static final int NAVIGATION_BARS = WindowInsetsCompat.Type.navigationBars();
    private static final int CAPTION_BAR = WindowInsetsCompat.Type.captionBar();
    private static final int SYSTEM_BARS = WindowInsetsCompat.Type.systemBars();
    private static final int DISPLAY_CUTOUT = WindowInsetsCompat.Type.displayCutout();

    @Rule public MockitoRule mockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock InsetObserver mInsetObserver;

    private View mOriginalContentView;
    private EdgeToEdgeLayoutCoordinator mEdgeToEdgeLayoutCoordinator;
    private Activity mActivity;
    private EdgeToEdgeBaseLayout mEdgeToEdgeLayout;

    @Before
    public void setup() {
        mActivityScenarioRule.getScenario().onActivity(activity -> mActivity = activity);
        mOriginalContentView = new FrameLayout(mActivity);
    }

    @Test
    public void testInitialize() {
        initialize(null);
        assertEquals(mEdgeToEdgeLayout, mOriginalContentView.getParent());
    }

    @Test
    public void testInitialize_withInsetObserver() {
        initialize(mInsetObserver);
        assertEquals(mEdgeToEdgeLayout, mOriginalContentView.getParent());
        verify(mInsetObserver)
                .addInsetsConsumer(any(), eq(InsetConsumerSource.EDGE_TO_EDGE_LAYOUT_COORDINATOR));
    }

    // ┌────────┐
    // ├────────┤
    // │        │
    // │        │
    // ├────────┤
    // └────────┘
    @Test
    @Config(qualifiers = "w400dp-h600dp")
    public void testPortrait_TopBottomSystemBar() {
        initialize(null);
        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topBottomInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(0, 0, 0, NAV_BAR_SIZE))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topBottomInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(STATUS_BAR_SIZE + NAV_BAR_SIZE));

        measureAndLayoutRootView(400, 600);
        assertPaddings(
                /* left= */ 0,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ 0,
                /* bottom= */ NAV_BAR_SIZE);

        // status bar is with Rect(0,0, WINDOW_WIDTH, STATUS_BAR_SIZE)
        assertEquals(
                "Status bar is at the top of the window.",
                new Rect(0, 0, 400, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        // nav bar is with Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar is at the bottom of the screen.",
                new Rect(0, 450, 400, 600),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        // Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar divider is the top 1px height for the nav bar.",
                new Rect(0, 450, 400, 451),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┌────────┐
    // ├────────┤
    // │        │
    // │        │
    // └────────┘
    // Case when window is on split screen top.
    @Test
    @Config(qualifiers = "w400dp-h600dp")
    public void testPortrait_NoNavigationBar() {
        initialize(null);
        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topBottomInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topBottomInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(STATUS_BAR_SIZE + NAV_BAR_SIZE));

        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ STATUS_BAR_SIZE, /* right= */ 0, /* bottom= */ 0);

        // status bar is with Rect(0,0, WINDOW_WIDTH, STATUS_BAR_SIZE)
        assertEquals(
                "Status bar is at the top of the window.",
                new Rect(0, 0, 400, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar insets doesn't exists.",
                new Rect(),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider doesn't exists.",
                new Rect(),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┌────────┐
    // │        │
    // │        │
    // ├────────┤
    // └────────┘
    // Case when window is on bottom split screen bottom.
    @Test
    @Config(qualifiers = "w400dp-h600dp")
    public void testPortrait_NoStatusBar() {
        initialize(null);
        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topBottomInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(NAV_BAR_SIZE, Insets.of(0, 0, 0, NAV_BAR_SIZE))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topBottomInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(STATUS_BAR_SIZE + NAV_BAR_SIZE));

        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ NAV_BAR_SIZE);

        assertEquals(
                "Status bar insets should be empty .",
                new Rect(),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        // nav bar is with Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar is at the bottom of the screen.",
                new Rect(0, 450, 400, 600),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        // Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar divider is the top 1px height for the nav bar.",
                new Rect(0, 450, 400, 451),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┌────────┐
    // ├────────┤
    // │        │
    // │        │
    // └────────┘
    // Case when window is in freeform on certain OEMs. captionBar is introduced in API 30.
    @Test
    @Config(qualifiers = "w400dp-h600dp", sdk = 30)
    public void testCaptionBar() {
        initialize(null);
        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat captionBarInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(CAPTION_BAR, Insets.of(0, CAPTION_BAR_SIZE, 0, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, captionBarInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(WindowInsetsCompat.Type.systemBars()));

        measureAndLayoutRootView(400, 600);
        assertPaddings(/* left= */ 0, /* top= */ CAPTION_BAR_SIZE, /* right= */ 0, /* bottom= */ 0);

        // Both status bar and navigation bar should be empty.
        assertEquals(
                "Status bar insets should be empty.",
                new Rect(),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar insets should be empty.",
                new Rect(),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider rect should be empty.",
                new Rect(),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┌───┬─────────────┐
    // │   ├─────────────┤
    // │   │             │
    // │   │             │
    // └───┴─────────────┘
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_LeftNavBar() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topLeftInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(NAV_BAR_SIZE, 0, 0, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(mEdgeToEdgeLayout, topLeftInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(STATUS_BAR_SIZE + NAV_BAR_SIZE));

        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ NAV_BAR_SIZE,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ 0,
                /* bottom= */ 0);

        mEdgeToEdgeLayout.measure(-1, -1);
        // status bar is with Rect(NAV_BAR_SIZE, 0, WINDOW_WIDTH, STATUS_BAR_SIZE)
        assertEquals(
                "Status bar is at the top of the window, avoid overlap with nav bar.",
                new Rect(150, 0, 600, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        // nav bar is with Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar is at the left of the screen.",
                new Rect(0, 0, 150, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        // Rect(0, WINDOW_SIZE - NAV_BAR_SIZE, WINDOW_WIDTH, WINDOW_SIZE)
        assertEquals(
                "Nav bar divider is the right most 1px for the nav bar.",
                new Rect(149, 0, 150, 400),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┌───────────────┬───┐
    // ├───────────────┤   │
    // │               │   │
    // │               │   │
    // └───────────────┴───┘
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_RightNavBar() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topRightInsets =
                new WindowInsetsCompat.Builder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(0, 0, NAV_BAR_SIZE, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(mEdgeToEdgeLayout, topRightInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(STATUS_BAR_SIZE + NAV_BAR_SIZE));
        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ 0,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ NAV_BAR_SIZE,
                /* bottom= */ 0);

        assertEquals(
                "Status bar is at the top of the window, avoid overlap with nav bar.",
                new Rect(0, 0, 450, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar is at the right of the screen.",
                new Rect(450, 0, 600, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider is the left most 1px for the nav bar.",
                new Rect(450, 0, 451, 400),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
    }

    // ┏━┱───────────────┬───┐
    // ┃╳┠───────────────┤   │
    // ┃╳┃               │   │
    // ┃╳┃               │   │
    // ┗━┹───────────────┴───┘
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_RightNavBarLeftCutout() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topRightSysBarsLeftCutoutInsets =
                new SpyWindowInsetsBuilder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(0, 0, NAV_BAR_SIZE, 0))
                        .setInsets(DISPLAY_CUTOUT, Insets.of(CUTOUT_SIZE, 0, 0, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topRightSysBarsLeftCutoutInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(SYSTEM_BARS + DISPLAY_CUTOUT));

        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ CUTOUT_SIZE,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ NAV_BAR_SIZE,
                /* bottom= */ 0);

        assertEquals(
                "Status bar is at the top of the window, avoid overlap with display cutout and nav"
                        + " bar.",
                new Rect(75, 0, 450, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar is at the right of the screen.",
                new Rect(450, 0, 600, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider is the left most 1px for the nav bar.",
                new Rect(450, 0, 451, 400),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
        assertEquals(
                "Display cutout is at the left of the window.",
                new Rect(0, 0, 75, 400),
                mEdgeToEdgeLayout.getCutoutRectLeftForTesting());
    }

    // ┌───┬─────────────┲━┓
    // │   ├─────────────┨╳┃
    // │   │             ┃╳┃
    // │   │             ┃╳┃
    // └───┴─────────────┺━┛
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_LeftNavBarRightCutout() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topRightSysBarsLeftCutoutInsets =
                new SpyWindowInsetsBuilder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(NAV_BAR_SIZE, 0, 0, 0))
                        .setInsets(DISPLAY_CUTOUT, Insets.of(0, 0, CUTOUT_SIZE, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topRightSysBarsLeftCutoutInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(SYSTEM_BARS + DISPLAY_CUTOUT));

        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ NAV_BAR_SIZE,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ CUTOUT_SIZE,
                /* bottom= */ 0);

        assertEquals(
                "Status bar is at the top of the window, avoid overlap with display cutout and nav"
                        + " bar.",
                new Rect(150, 0, 525, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar is at the right of the screen.",
                new Rect(0, 0, 150, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider is the left most 1px for the nav bar.",
                new Rect(149, 0, 150, 400),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());

        assertEquals(
                "Display cutout left is empty.",
                new Rect(),
                mEdgeToEdgeLayout.getCutoutRectLeftForTesting());
        assertEquals(
                "Display cutout is at the right of the window.",
                new Rect(525, 0, 600, 400),
                mEdgeToEdgeLayout.getCutoutRectRightForTesting());
    }

    // ┏━┱──────────────────┐
    // ┃╳┠──────────────────┤
    // ┃╳┃                  │
    // ┃╳┠──────────────────┤
    // ┗━┹──────────────────┘
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_GestureModeLeftCutout() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topBottomSysBarsLeftCutoutInsets =
                new SpyWindowInsetsBuilder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(0, 0, 0, NAV_BAR_SIZE))
                        .setInsets(DISPLAY_CUTOUT, Insets.of(CUTOUT_SIZE, 0, 0, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topBottomSysBarsLeftCutoutInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(SYSTEM_BARS + DISPLAY_CUTOUT));

        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ CUTOUT_SIZE,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ 0,
                /* bottom= */ NAV_BAR_SIZE);

        assertEquals(
                "Status bar is at the top of the window, avoid overlap with display cutout.",
                new Rect(75, 0, 600, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar is at the bottom of the screen, avoid overlap with display cutout.",
                new Rect(75, 250, 600, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider is the left most 1px for the nav bar.",
                new Rect(75, 250, 600, 251),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
        assertEquals(
                "Display cutout is at the left of the window.",
                new Rect(0, 0, 75, 400),
                mEdgeToEdgeLayout.getCutoutRectLeftForTesting());
        assertEquals(
                "Display cutout is at the left of the window.",
                new Rect(),
                mEdgeToEdgeLayout.getCutoutRectRightForTesting());
    }

    // ┌─────────────────┲━┓
    // ├─────────────────┨╳┃
    // │                 ┃╳┃
    // ├─────────────────┨╳┃
    // └─────────────────┺━┛
    @Test
    @Config(qualifiers = "w600dp-h400dp")
    public void testLandscape_GestureModeRightCutout() {
        initialize(null);
        measureAndLayoutRootView(600, 400);
        assertPaddings(/* left= */ 0, /* top= */ 0, /* right= */ 0, /* bottom= */ 0);

        WindowInsetsCompat topBottomSysBarsLeftCutoutInsets =
                new SpyWindowInsetsBuilder()
                        .setInsets(STATUS_BARS, Insets.of(0, STATUS_BAR_SIZE, 0, 0))
                        .setInsets(NAVIGATION_BARS, Insets.of(0, 0, 0, NAV_BAR_SIZE))
                        .setInsets(DISPLAY_CUTOUT, Insets.of(0, 0, CUTOUT_SIZE, 0))
                        .build();
        WindowInsetsCompat newInsets =
                mEdgeToEdgeLayoutCoordinator.onApplyWindowInsets(
                        mEdgeToEdgeLayout, topBottomSysBarsLeftCutoutInsets);
        assertEquals(
                "Window insets should be consumed",
                Insets.NONE,
                newInsets.getInsets(SYSTEM_BARS + DISPLAY_CUTOUT));

        measureAndLayoutRootView(600, 400);
        assertPaddings(
                /* left= */ 0,
                /* top= */ STATUS_BAR_SIZE,
                /* right= */ CUTOUT_SIZE,
                /* bottom= */ NAV_BAR_SIZE);

        assertEquals(
                "Status bar is at the top of the window, avoid overlap with display cutout.",
                new Rect(0, 0, 525, 100),
                mEdgeToEdgeLayout.getStatusBarRectForTesting());
        assertEquals(
                "Nav bar is at the bottom of the screen, avoid overlap with display cutout.",
                new Rect(0, 250, 525, 400),
                mEdgeToEdgeLayout.getNavigationBarRectForTesting());
        assertEquals(
                "Nav bar divider is the left most 1px for the nav bar.",
                new Rect(0, 250, 525, 251),
                mEdgeToEdgeLayout.getNavigationBarDividerRectForTesting());
        assertEquals(
                "Display cutout left is empty.",
                new Rect(),
                mEdgeToEdgeLayout.getCutoutRectLeftForTesting());
        assertEquals(
                "Display cutout is at the right of the window.",
                new Rect(525, 0, 600, 400),
                mEdgeToEdgeLayout.getCutoutRectRightForTesting());
    }

    private void initialize(InsetObserver insetObserver) {
        mEdgeToEdgeLayoutCoordinator = new EdgeToEdgeLayoutCoordinator(mActivity, insetObserver);
        mEdgeToEdgeLayout =
                (EdgeToEdgeBaseLayout)
                        mEdgeToEdgeLayoutCoordinator.wrapContentView(mOriginalContentView);
        mActivity.setContentView(mEdgeToEdgeLayout);
    }

    private void assertPaddings(int left, int top, int right, int bottom) {
        assertEquals("Padding left is wrong.", left, mEdgeToEdgeLayout.getPaddingLeft());
        assertEquals("Padding top is wrong.", top, mEdgeToEdgeLayout.getPaddingTop());
        assertEquals("Padding right is wrong.", right, mEdgeToEdgeLayout.getPaddingRight());
        assertEquals("Padding bottom is wrong.", bottom, mEdgeToEdgeLayout.getPaddingBottom());
    }

    private void measureAndLayoutRootView(int width, int height) {
        mEdgeToEdgeLayout.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        mEdgeToEdgeLayout.layout(0, 0, width, height);
    }
}

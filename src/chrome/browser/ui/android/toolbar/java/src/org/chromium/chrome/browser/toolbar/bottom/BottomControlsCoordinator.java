// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsViewBinder.ViewHolder;
import org.chromium.components.browser_ui.widget.gesture.BackPressHandler;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;
import org.chromium.ui.resources.ResourceManager;
import org.chromium.ui.widget.Toast;
import org.chromium.ui.widget.ChromeImageButton;
import android.view.View.OnClickListener;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.toolbar.top.ToggleTabStackButton;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarThemeCommunicator;
import org.chromium.base.ApiCompatibilityUtils;
import android.graphics.Color;
import android.content.res.ColorStateList;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.chrome.browser.theme.ThemeUtils;
import androidx.appcompat.content.res.AppCompatResources;

/**
 * The root coordinator for the bottom controls component. This component is intended for use with
 * bottom UI that re-sizes the web contents, scrolls off-screen, and hides when the keyboard is
 * shown. This class has two primary components, an Android view and a composited texture that draws
 * when the controls are being scrolled off-screen. The Android version does not draw unless the
 * controls offset is 0.
 */
public class BottomControlsCoordinator implements BackPressHandler, BottomToolbarVisibilityController, BottomToolbarThemeCommunicator {
    /**
     * Interface for the BottomControls component to hide and show itself.
     */
    public interface BottomControlsVisibilityController {
        void setBottomControlsVisible(boolean isVisible);
    }

    /** The mediator that handles events from outside the bottom controls. */
    private final BottomControlsMediator mMediator;

    /** The Delegate for the split toolbar's bottom toolbar component UI operation. */
    private @Nullable BottomControlsContentDelegate mContentDelegate;

    private View mBottomToolbarWrapper;
    private View mContainer;
    private ChromeImageButton mSearchAccelerator;
    // private ChromeImageButton mSpeedDialButton;
    private ChromeImageButton mHomeButton;
    private ChromeImageButton mSettingsButton;
    private ChromeImageButton mRewardsButton;
    private ToggleTabStackButton mTabSwitcherButton;
    /** Used to know the Layout state. */
    private LayoutStateProvider mLayoutStateProvider;
    private final LayoutStateProvider.LayoutStateObserver mLayoutStateObserver;

    private TabModelSelector mTabModelSelector;

    /**
     * Build the coordinator that manages the bottom controls.
     * @param activity Activity instance to use.
     * @param windowAndroid A {@link WindowAndroid} for watching keyboard visibility events.
     * @param controlsSizer A {@link BrowserControlsSizer} to update the bottom controls
     *                          height for the renderer.
     * @param fullscreenManager A {@link FullscreenManager} to listen for fullscreen changes.
     * @param stub The bottom controls {@link ViewStub} to inflate.
     * @param contentDelegate Delegate for bottom controls UI operations.
     * @param overlayPanelVisibilitySupplier Notifies overlay panel visibility event.
     * @param resourceManager A {@link ResourceManager} for loading textures into the compositor.
     * @param layoutManager A {@link LayoutManagerImpl} to attach overlays to.
     */
    @SuppressLint("CutPasteId") // Not actually cut and paste since it's View vs ViewGroup.
    public BottomControlsCoordinator(Activity activity, WindowAndroid windowAndroid,
            LayoutManager layoutManager, ResourceManager resourceManager,
            BrowserControlsSizer controlsSizer, FullscreenManager fullscreenManager,
            ScrollingBottomViewResourceFrameLayout root,
            BottomControlsContentDelegate contentDelegate,
            ObservableSupplier<Boolean> overlayPanelVisibilitySupplier,
            BottomToolbarCoordinator BottomToolbarCoordinator, OnClickListener tabSwitcherClickHandler,
            TabModelSelector tabModelSelector, TabCountProvider tabCountProvider) {
        PropertyModel model = new PropertyModel(BottomControlsProperties.ALL_KEYS);

        ScrollingBottomViewSceneLayer sceneLayer =
                new ScrollingBottomViewSceneLayer(root, root.getTopShadowHeight());
        PropertyModelChangeProcessor.create(
                model, new ViewHolder(root, sceneLayer), BottomControlsViewBinder::bind);
        layoutManager.createCompositorMCP(
                model, sceneLayer, BottomControlsViewBinder::bindCompositorMCP);
        int bottomControlsHeightId = R.dimen.bottom_controls_height_new;

        // View container = root.findViewById(R.id.bottom_container_slot);
        // ViewGroup.LayoutParams params = container.getLayoutParams();
        // params.height = root.getResources().getDimensionPixelOffset(bottomControlsHeightId);
        mBottomToolbarWrapper = root.findViewById(R.id.bottom_toolbar_wrapper);
        mContainer = root.findViewById(R.id.bottom_container_slot);
        mMediator =
                new BottomControlsMediator(windowAndroid, model, controlsSizer, fullscreenManager,
                        root.getResources().getDimensionPixelOffset(bottomControlsHeightId),
                        overlayPanelVisibilitySupplier, this, root.getResources().getDimensionPixelOffset(R.dimen.bottom_controls_height_hidden_controls));

        resourceManager.getDynamicResourceLoader().registerResource(
                root.getId(), root.getResourceAdapter());

        mContentDelegate = contentDelegate;
        Toast.setGlobalExtraYOffset(
                root.getResources().getDimensionPixelSize(bottomControlsHeightId));

        // Set the visibility of BottomControls to false by default. Components within
        // BottomControls should update the visibility explicitly if needed.
        setBottomToolbarVisible(true);

        sceneLayer.setIsVisible(mMediator.isCompositedViewVisible());
        layoutManager.addSceneOverlay(sceneLayer);

        if (mContentDelegate != null) {
            mContentDelegate.initializeWithNative(activity, mMediator::setBottomControlsVisible);
        }

        mTabSwitcherButton = root.findViewById(R.id.tab_switcher_button_bottom);
        if (mTabSwitcherButton != null) {
            mTabSwitcherButton.setOnClickListener(tabSwitcherClickHandler);
            mTabSwitcherButton.setTabCountProvider(tabCountProvider);
        }

        mSettingsButton = root.findViewById(R.id.settings_button_bottom);
        mSettingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BottomToolbarCoordinator.openSettings(v);
            }
        });

        mHomeButton = root.findViewById(R.id.bottom_home_button);
        mHomeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BottomToolbarCoordinator.loadHomepage(v);
            }
        });

        mSearchAccelerator = root.findViewById(R.id.search_accelerator);
        mSearchAccelerator.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // do callback
                BottomToolbarCoordinator.focusUrlBar();
            }
        });

        // mRewardsButton = root.findViewById(R.id.rewards_button_bottom);
        // if (mRewardsButton != null) {
        //     mRewardsButton.setOnClickListener(new View.OnClickListener() {
        //         @Override
        //         public void onClick(View v) {
        //             // do callback
        //             BottomToolbarCoordinator.openRewardsPopup(v);
        //         }
        //     });
        // }

        // Set the visibility of BottomControls to false by default. Components within
        // BottomControls should update the visibility explicitly if needed.

        mLayoutStateObserver = new LayoutStateProvider.LayoutStateObserver() {
            @Override
            public void onStartedShowing(@LayoutType int layoutType, boolean showToolbar) {
                if (layoutType == LayoutType.TAB_SWITCHER) {;
                    // SHOWING OVERVIEW
                    setBottomToolbarVisible(false);
                } else if (layoutType == LayoutType.BROWSING) {
                    // SHOWING BROWSING
                    setBottomToolbarVisible(true);
                }
            }

            @Override
            public void onStartedHiding(
                    @LayoutType int layoutType, boolean showToolbar, boolean delayAnimation) { }
        };
    }

    /**
     * @param layoutStateProvider {@link LayoutStateProvider} object.
     */
    public void setLayoutStateProvider(LayoutStateProvider layoutStateProvider) {
        mMediator.setLayoutStateProvider(layoutStateProvider);

        mLayoutStateProvider = layoutStateProvider;
        mLayoutStateProvider.addObserver(mLayoutStateObserver);
    }

    /**
     * @param isVisible Whether the bottom control is visible.
     */
    public void setBottomControlsVisible(boolean isVisible) {
        // mMediator.setBottomControlsVisible(isVisible);
    }

    public void setBottomToolbarVisible(boolean isVisible) {
        mMediator.setBottomToolbarVisible(isVisible);
    }

    private boolean isColorDark(int color){
        double darkness = 1-(0.299*Color.red(color) + 0.587*Color.green(color) + 0.114*Color.blue(color))/255;
        if(darkness<0.5){
            return false; // It's a light color
        }else{
            return true; // It's a dark color
        }
    }

    @Override
    public void onThemeChanged(int color) {
        if (mBottomToolbarWrapper == null) return;
        mBottomToolbarWrapper.setBackgroundColor(color);

        ColorStateList tint = isColorDark(color) ? AppCompatResources.getColorStateList(mHomeButton.getContext(), R.color.default_icon_color_light_tint_list)
                        : AppCompatResources.getColorStateList(mHomeButton.getContext(), R.color.default_icon_color_dark_tint_list);

        ApiCompatibilityUtils.setImageTintList(mSearchAccelerator, tint);
        ApiCompatibilityUtils.setImageTintList(mHomeButton, tint);
        ApiCompatibilityUtils.setImageTintList(mTabSwitcherButton, tint);
        mTabSwitcherButton.setBrandedColorScheme(isColorDark(color) ? BrandedColorScheme.DARK_BRANDED_THEME : BrandedColorScheme.LIGHT_BRANDED_THEME);
    }

    @Override
    public void onTintChanged(ColorStateList tint, @BrandedColorScheme int brandedColorScheme) {

        // ApiCompatibilityUtils.setImageTintList(mSearchAccelerator, tint);
        // // ApiCompatibilityUtils.setImageTintList(mSpeedDialButton, tint);
        // ApiCompatibilityUtils.setImageTintList(mHomeButton, tint);
        //
        // mTabSwitcherButton.setBrandedColorScheme(brandedColorScheme);
    }

    @Override
    public void setBottomToolbarTopSectionVisible(boolean isVisible) {
      if (!isVisible) {
        ViewGroup.LayoutParams params = mContainer.getLayoutParams();
        params.height = 0;

        mContainer.setVisibility(View.GONE);

        mContainer.invalidate();
      } else {
        ViewGroup.LayoutParams params = mContainer.getLayoutParams();
        params.height = mContainer.getResources().getDimensionPixelOffset(R.dimen.bottom_controls_height);

        mContainer.setVisibility(View.VISIBLE);

        mContainer.invalidate();
      }
    }

    /**
     * Handles system back press action if needed.
     * @return Whether or not the back press event is consumed here.
     */
    public boolean onBackPressed() {
        return mContentDelegate != null && mContentDelegate.onBackPressed();
    }

    @Override
    public void handleBackPress() {
        if (mContentDelegate != null) mContentDelegate.handleBackPress();
    }

    @Override
    public ObservableSupplier<Boolean> getHandleBackPressChangedSupplier() {
        if (mContentDelegate == null) return new ObservableSupplierImpl<>();
        return mContentDelegate.getHandleBackPressChangedSupplier();
    }

    /**
     * Clean up any state when the bottom controls component is destroyed.
     */
    public void destroy() {
        if (mContentDelegate != null) mContentDelegate.destroy();

        if (mBottomToolbarWrapper != null) mBottomToolbarWrapper = null;
        if (mContainer != null) mContainer = null;
        if (mLayoutStateProvider != null) {
            mLayoutStateProvider.removeObserver(mLayoutStateObserver);
            mLayoutStateProvider = null;
        }

        mMediator.destroy();
    }
}

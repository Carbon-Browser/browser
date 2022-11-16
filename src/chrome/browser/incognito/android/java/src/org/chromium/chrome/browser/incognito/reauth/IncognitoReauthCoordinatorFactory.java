// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.incognito.reauth;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.CallbackController;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager.IncognitoReauthCallback;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.tabmodel.IncognitoTabHostUtils;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.tab_management.TabSwitcherCustomViewManager;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.ui.modaldialog.ModalDialogManager;

/**
 * A factory to create an {@link IncognitoReauthCoordinator} instance.
 */
public class IncognitoReauthCoordinatorFactory {
    /** The context to use in order to inflate the re-auth view and fetch the resources. */
    private final @NonNull Context mContext;
    /**
     * The {@link TabModelSelector} instance used for providing functionality to toggle between tab
     * models and to close Incognito tabs.
     */
    private final @NonNull TabModelSelector mTabModelSelector;
    /** The manager responsible for showing the full screen Incognito re-auth dialog. */
    private final @NonNull ModalDialogManager mModalDialogManager;
    /** The manager responsible for invoking the underlying native re-authentication. */
    private final @NonNull IncognitoReauthManager mIncognitoReauthManager;
    /** The launcher to show the SettingsActivity. */
    private final @NonNull SettingsLauncher mSettingsLauncher;
    /**
     * A boolean to distinguish between tabbedActivity or CCT, during coordinator creation.
     *
     * TODO(crbug.com/1227656): Remove the need for this and instead populate the
     * various {@link Runnable} that we create here at the client site directly.
     */
    private final boolean mIsTabbedActivity;

    /**
     * This allows to disable/enable top toolbar elements.
     * Non-null for {@link TabSwitcherIncognitoReauthCoordinator} instance.
     */
    private final @Nullable IncognitoReauthTopToolbarDelegate mIncognitoReauthTopToolbarDelegate;
    /** A callback controller to monitor the availability of {@link TabSwitcherCustomViewManager}.*/
    private final CallbackController mTabSwitcherCustomViewManagerController =
            new CallbackController();

    /**
     * This allows to pass the re-auth view to the tab switcher.
     * Non-null for {@link TabSwitcherIncognitoReauthCoordinator}.
     */
    private @Nullable TabSwitcherCustomViewManager mTabSwitcherCustomViewManager;
    /**
     * This allows to show the regular overview mode.
     * Non-null for {@link FullScreenIncognitoReauthCoordinator}.
     */
    private @Nullable LayoutManager mLayoutManager;

    /**
     * A test-only variable used to mock the menu delegate instead of creating one.
     */
    @VisibleForTesting
    IncognitoReauthMenuDelegate mIncognitoReauthMenuDelegateForTesting;

    /**
     * @param context The {@link Context} to use for fetching the re-auth resources.
     * @param tabModelSelector The {@link TabModelSelector} to use for toggling between regular
     *         and incognito model and to close all Incognito tabs.
     * @param modalDialogManager The {@link ModalDialogManager} to use for firing the dialog
     *         containing the Incognito re-auth view.
     * @param incognitoReauthManager The {@link IncognitoReauthManager} instance which would be
     *                              used to initiate re-authentication.
     * @param  settingsLauncher A {@link SettingsLauncher} to use for launching {@link
     *         SettingsActivity} from 3 dots menu inside full-screen re-auth.
     * @param tabSwitcherCustomViewManagerOneshotSupplier {@link OneshotSupplier
     *         <TabSwitcherCustomViewManager>} to use for communicating with tab switcher to show
     *         the re-auth screen.
     * @param incognitoReauthTopToolbarDelegate A {@link IncognitoReauthTopToolbarDelegate} to use
     *         for disabling/enabling few top toolbar elements inside tab switcher.
     * @param layoutManager {@link LayoutManager} to use for showing the regular overview mode.
     * @param isTabbedActivity A boolean to indicate if the re-auth screen being fired from
     */
    public IncognitoReauthCoordinatorFactory(@NonNull Context context,
            @NonNull TabModelSelector tabModelSelector,
            @NonNull ModalDialogManager modalDialogManager,
            @NonNull IncognitoReauthManager incognitoReauthManager,
            @NonNull SettingsLauncher settingsLauncher,
            @Nullable OneshotSupplier<TabSwitcherCustomViewManager>
                    tabSwitcherCustomViewManagerOneshotSupplier,
            @Nullable IncognitoReauthTopToolbarDelegate incognitoReauthTopToolbarDelegate,
            @Nullable LayoutManager layoutManager, boolean isTabbedActivity) {
        mContext = context;
        mTabModelSelector = tabModelSelector;
        mModalDialogManager = modalDialogManager;
        mIncognitoReauthManager = incognitoReauthManager;
        mSettingsLauncher = settingsLauncher;
        mIncognitoReauthTopToolbarDelegate = incognitoReauthTopToolbarDelegate;
        mLayoutManager = layoutManager;
        mIsTabbedActivity = isTabbedActivity;

        if (isTabbedActivity) {
            assert tabSwitcherCustomViewManagerOneshotSupplier != null;
            tabSwitcherCustomViewManagerOneshotSupplier.onAvailable(
                    mTabSwitcherCustomViewManagerController.makeCancelable(manager -> {
                        assert manager != null;
                        mTabSwitcherCustomViewManager = manager;
                    }));
        } else {
            assert tabSwitcherCustomViewManagerOneshotSupplier == null;
        }
    }

    /**
     * This method is responsible for clean-up work. Typically, called when the Activity is being
     * destroyed.
     */
    void destroy() {
        mTabSwitcherCustomViewManagerController.destroy();
    }

    private IncognitoReauthMenuDelegate getIncognitoReauthMenuDelegate() {
        if (mIncognitoReauthMenuDelegateForTesting != null) {
            return mIncognitoReauthMenuDelegateForTesting;
        }

        return new IncognitoReauthMenuDelegate(
                mContext, getCloseAllIncognitoTabsRunnable(), mSettingsLauncher);
    }

    /**
     * @return {@link Runnable} to use when the user click on "See other tabs".
     */
    @VisibleForTesting
    Runnable getSeeOtherTabsRunnable() {
        if (mIsTabbedActivity) {
            return () -> {
                mTabModelSelector.selectModel(/*incognito=*/false);
                mLayoutManager.showLayout(LayoutType.TAB_SWITCHER, /*animate=*/false);
            };
        } else {
            // TODO(crbug.com/1227656): Add implementation for iCCT case.
            return () -> {};
        }
    }

    /**
     * @return {@link Runnable} to use when the user clicks on "Close all Incognito tabs" option.
     */
    @VisibleForTesting
    Runnable getCloseAllIncognitoTabsRunnable() {
        return IncognitoTabHostUtils::closeAllIncognitoTabs;
    }

    /**
     * @return {@link Runnable} to use when the user presses back while the re-auth is being shown.
     */
    Runnable getBackPressRunnable() {
        if (mIsTabbedActivity) {
            return getSeeOtherTabsRunnable();
        } else {
            return () -> {};
        }
    }

    /**
     * @return True, if the re-auth is being triggered from a tabbed Activity, false otherwise.
     */
    boolean getIsTabbedActivity() {
        return mIsTabbedActivity;
    }

    /**
     * @param incognitoReauthCallback The {@link IncognitoReauthCallback}
     *                               which would be executed after an authentication attempt.
     * @param showFullScreen A boolean indicating whether to show a fullscreen or tab switcher
     *                      Incognito reauth.
     *
     * @return {@link IncognitoReauthCoordinator} instance spliced on fullscreen/tab-switcher and
     * tabbedActivity/CCT.
     */
    IncognitoReauthCoordinator createIncognitoReauthCoordinator(
            @NonNull IncognitoReauthCallback incognitoReauthCallback, boolean showFullScreen) {
        return (showFullScreen)
                ? new FullScreenIncognitoReauthCoordinator(mContext, mIncognitoReauthManager,
                        incognitoReauthCallback, getSeeOtherTabsRunnable(), mModalDialogManager,
                        getIncognitoReauthMenuDelegate())
                : new TabSwitcherIncognitoReauthCoordinator(mContext, mIncognitoReauthManager,
                        incognitoReauthCallback, getSeeOtherTabsRunnable(),
                        mTabSwitcherCustomViewManager, mIncognitoReauthTopToolbarDelegate);
    }
}

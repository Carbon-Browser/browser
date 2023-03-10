// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.rewards;

import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetControllerProvider;
import org.chromium.ui.base.WindowAndroid;
import android.view.View;

/**
 * Coordinator for the bottom sheet content in the screen which allows a parent to approve or deny
 * a website.
 */
public class RewardsBottomSheetCoordinator {
    private final BottomSheetController mBottomSheetController;
    private final RewardsBottomSheetContent mSheetContent;

    private RewardsCommunicator mRewardsCommunicator;

    /**
     * Constructor for the co-ordinator.  Callers should then call {@link show()} to display the
     * UI.
     *
     * @param url the full URL for which the request is being made (code in this module is
     * responsible for displaying the appropriate part of the URL to the user)
     */
    public RewardsBottomSheetCoordinator(WindowAndroid windowAndroid, RewardsCommunicator rewardsCommunicator) {

        mRewardsCommunicator = rewardsCommunicator;
        mBottomSheetController = BottomSheetControllerProvider.from(windowAndroid);
        mSheetContent = new RewardsBottomSheetContent(windowAndroid.getContext().get(), mRewardsCommunicator);
    }

    /** Displays the UI to request parent approval in a new bottom sheet. */
    public void show() {
        mBottomSheetController.requestShowContent(mSheetContent, true);
    }
}

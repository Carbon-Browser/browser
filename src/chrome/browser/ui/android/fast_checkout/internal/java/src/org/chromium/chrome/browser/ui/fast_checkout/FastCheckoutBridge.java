// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ui.fast_checkout;

import androidx.annotation.Nullable;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ui.fast_checkout.data.FastCheckoutAutofillProfile;
import org.chromium.chrome.browser.ui.fast_checkout.data.FastCheckoutCreditCard;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetControllerProvider;
import org.chromium.ui.base.WindowAndroid;

/**
 * This bridge creates and initializes a {@link FastCheckoutComponent} on construction and forwards
 * native calls to it.
 */
class FastCheckoutBridge implements FastCheckoutComponent.Delegate {
    private long mNativeFastCheckoutBridge;
    private final FastCheckoutComponent mFastCheckoutComponent;

    private FastCheckoutBridge(long nativeBridge, WindowAndroid windowAndroid,
            BottomSheetController bottomSheetController) {
        mNativeFastCheckoutBridge = nativeBridge;
        mFastCheckoutComponent = new FastCheckoutCoordinator();
        mFastCheckoutComponent.initialize(
                windowAndroid.getContext().get(), bottomSheetController, this);
    }

    @CalledByNative
    private static @Nullable FastCheckoutBridge create(
            long nativeBridge, WindowAndroid windowAndroid) {
        BottomSheetController bottomSheetController =
                BottomSheetControllerProvider.from(windowAndroid);
        if (bottomSheetController == null) return null;
        return new FastCheckoutBridge(nativeBridge, windowAndroid, bottomSheetController);
    }

    @CalledByNative
    private void showBottomSheet(
            FastCheckoutAutofillProfile[] profiles, FastCheckoutCreditCard[] creditCards) {
        mFastCheckoutComponent.showOptions(profiles, creditCards);
    }

    @CalledByNative
    private static void setAutofillProfile(FastCheckoutAutofillProfile[] profiles, int index,
            FastCheckoutAutofillProfile profile) {
        profiles[index] = profile;
    }

    @CalledByNative
    private static void setCreditCard(
            FastCheckoutCreditCard[] creditCards, int index, FastCheckoutCreditCard creditCard) {
        creditCards[index] = creditCard;
    }

    @CalledByNative
    private static FastCheckoutAutofillProfile[] createAutofillProfilesArray(int size) {
        return new FastCheckoutAutofillProfile[size];
    }

    @CalledByNative
    private static FastCheckoutCreditCard[] createCreditCardsArray(int size) {
        return new FastCheckoutCreditCard[size];
    }

    @CalledByNative
    private void destroy() {
        mNativeFastCheckoutBridge = 0;
    }

    @Override
    public void onDismissed() {
        // TODO(crbug.com/1334642): Call native side to continue dismissing.
    }

    @Override
    public void onOptionsSelected(
            FastCheckoutAutofillProfile profile, FastCheckoutCreditCard creditCard) {
        // TODO(crbug.com/1334642): Call native side to continue filling.
    }
}

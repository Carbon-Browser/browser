// Copyright 2019 Hydris Apps Ltd. All rights reserved.

package org.chromium.chrome.browser.suggestions.speeddial;

import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.content.Context;
import android.view.View;
import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialInteraction;

import org.chromium.chrome.R;
import androidx.annotation.LayoutRes;

public class SpeedDialController {

    public static SpeedDialGridView inflateSpeedDial(Context context, int nColumns,
            SpeedDialInteraction speedDialInteraction, boolean isPopup) {
        return new SpeedDialGridView(context, nColumns, speedDialInteraction, isPopup);
    }

    @LayoutRes
    private static int getLayout() {
        return R.layout.speed_dial_layout;
    }
}

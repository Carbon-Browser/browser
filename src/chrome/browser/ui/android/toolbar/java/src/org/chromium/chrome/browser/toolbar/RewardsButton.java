// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.util.AttributeSet;
import androidx.core.content.ContextCompat;
import org.chromium.ui.widget.ChromeImageButton;

public class RewardsButton extends ChromeImageButton {

    public RewardsButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        setImageDrawable(ContextCompat.getDrawable(context, R.drawable.ic_adblock_active));
    }
}

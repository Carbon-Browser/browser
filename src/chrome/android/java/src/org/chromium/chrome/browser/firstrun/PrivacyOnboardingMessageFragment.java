// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;

import android.graphics.Color;
import android.graphics.Shader;
import android.graphics.LinearGradient;
import org.chromium.chrome.browser.firstrun.FirstRunFragment;

/** A {@link Fragment} that presents a set of search engines for the user to choose from. */
public class PrivacyOnboardingMessageFragment extends Fragment implements FirstRunFragment {

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(
                R.layout.privacy_first_run_fragment, container, false);

        Button mButton = (Button) rootView.findViewById(R.id.button_primary);
        // set start button gradient
        Shader textShader = new LinearGradient(0, 0, 150, 0,
            new int[]{Color.parseColor("#FF320A"),Color.parseColor("#FF9133")},
           null, Shader.TileMode.CLAMP);
        mButton.getPaint().setShader(textShader);
        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getPageDelegate().advanceToNextPage();
            }
        });

        return rootView;
    }

    @Override
    public void setInitialA11yFocus() {
    }
}

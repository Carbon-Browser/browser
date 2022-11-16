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
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

/** A {@link Fragment} that presents a set of search engines for the user to choose from. */
public class AdblockOnBoardingMessageFragment extends Fragment implements FirstRunFragment {

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(
                R.layout.adblock_first_run_fragment, container, false);

        StyleSpan boldSpan = new StyleSpan(android.graphics.Typeface.BOLD);
        TextView instructionsTextView = (TextView) rootView.findViewById(R.id.adblock_message_instructions);
        SpannableString description = SpanApplier.applySpans(
                getContext().getString(R.string.adblock_on_boarding_message_instructions),
                new SpanInfo("<b>", "</b>", boldSpan));
        instructionsTextView.setText(description);

        Button mButton = (Button) rootView.findViewById(R.id.button_primary);
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

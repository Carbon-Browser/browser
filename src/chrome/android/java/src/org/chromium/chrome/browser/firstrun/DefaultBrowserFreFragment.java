// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.provider.Settings;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.widget.TextView;
import android.content.Intent;
import androidx.fragment.app.Fragment;
import android.app.role.RoleManager;
import org.chromium.chrome.R;
import org.chromium.ui.text.SpanApplier;
import android.os.Build;
import org.chromium.ui.text.SpanApplier.SpanInfo;

/** A {@link Fragment} that presents a set of search engines for the user to choose from. */
public class DefaultBrowserFreFragment extends Fragment implements FirstRunFragment {

    private boolean supportsDefaultRoleManager() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q;
    }

    private boolean supportsDefault() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(
                R.layout.default_browser_fre, container, false);

        Button mButton = (Button) rootView.findViewById(R.id.button_primary);
        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (supportsDefaultRoleManager()) {
                    RoleManager roleManager = getActivity().getSystemService(RoleManager.class);

                    if (roleManager.isRoleAvailable(RoleManager.ROLE_BROWSER)) {
                        if (!roleManager.isRoleHeld(RoleManager.ROLE_BROWSER)) {
                            // save role manager open count as the second times onwards the dialog is shown,
                            // the system allows the user to click on "don't show again".

                            getActivity().startActivityForResult(
                                    roleManager.createRequestRoleIntent(RoleManager.ROLE_BROWSER), 69);
                        }
                    } else {
                      final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                      startActivity(intent);
                    }

                } else if (supportsDefault()) {
                    final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                }

                getPageDelegate().advanceToNextPage();
            }
        });

        Button mNotNow = (Button) rootView.findViewById(R.id.button_secondary);
        mNotNow.setOnClickListener(new View.OnClickListener() {
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

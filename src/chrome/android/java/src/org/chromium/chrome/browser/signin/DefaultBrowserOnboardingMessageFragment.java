// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

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
import android.content.Intent;
import android.app.role.RoleManager;
import android.provider.Settings;
import android.os.Build;
import org.chromium.chrome.browser.firstrun.FirstRunFragment;

/** A {@link Fragment} that presents a set of search engines for the user to choose from. */
public class DefaultBrowserOnboardingMessageFragment extends Fragment implements FirstRunFragment {

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(
                R.layout.default_browser_first_run_fragment, container, false);

        Button mDefaultBrowserBtn = (Button) rootView.findViewById(R.id.default_browser_button);
        mDefaultBrowserBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    RoleManager roleManager = getContext().getSystemService(RoleManager.class);

                    if (roleManager.isRoleAvailable(RoleManager.ROLE_BROWSER)) {
                        if (!roleManager.isRoleHeld(RoleManager.ROLE_BROWSER)) {
                            // save role manager open count as the second times onwards the dialog is shown,
                            // the system allows the user to click on "don't show again".
                            try {
                                startActivityForResult(
                                        roleManager.createRequestRoleIntent(RoleManager.ROLE_BROWSER), 69);
                            } catch (Exception ignore) {}
                        }
                    } else {
                      final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                      getContext().startActivity(intent);
                    }

                } else {
                    final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    getContext().startActivity(intent);
                }
            }
        });

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

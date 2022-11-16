// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

package org.chromium.components.adblock.settings;

import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.build.BuildConfig;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockController.ConnectionType;
import org.chromium.components.adblock.R;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

import java.lang.System;

public class AdblockSettingsFragment
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    private ChromeSwitchPreference mAdblockEnabled;
    private ChromeSwitchPreference mAcceptableAdsEnabled;
    private Preference mFilterLists;
    private Preference mAllowedDomains;
    private ChromeSwitchPreference mAllowedConnectionType;
    private Preference mMoreOptions;

    private static final String SETTINGS_ENABLED_KEY = "fragment_adblock_settings_enabled_key";
    private static final String SETTINGS_FILTER_LISTS_KEY =
            "fragment_adblock_settings_filter_lists_key";
    private static final String SETTINGS_AA_ENABLED_KEY =
            "fragment_adblock_settings_aa_enabled_key";
    private static final String SETTINGS_ALLOWED_DOMAINS_KEY =
            "fragment_adblock_settings_allowed_domains_key";
    private static final String SETTINGS_ALLOWED_CONNECTION_TYPE_KEY =
            "fragment_adblock_settings_allowed_connection_type_key";
    private static final String SETTINGS_MORE_OPTIONS_KEY =
            "fragment_adblock_settings_more_options_key";

    private int mOnOffClickCount;
    private static final int ON_OFF_TOGGLE_COUNT_TO_ENABLE_MORE_OPTIONS = 10;
    private static final int ON_OFF_TOGGLE_COUNT_TIME_WINDOW_MS = 3000;
    private long mOnOffTogleTimestamp;

    private void bindPreferences() {
        mAdblockEnabled = (ChromeSwitchPreference) findPreference(SETTINGS_ENABLED_KEY);
        mFilterLists = findPreference(SETTINGS_FILTER_LISTS_KEY);
        mAcceptableAdsEnabled = (ChromeSwitchPreference) findPreference(SETTINGS_AA_ENABLED_KEY);
        mAllowedDomains = findPreference(SETTINGS_ALLOWED_DOMAINS_KEY);
        mAllowedConnectionType =
                (ChromeSwitchPreference) findPreference(SETTINGS_ALLOWED_CONNECTION_TYPE_KEY);
        mMoreOptions = findPreference(SETTINGS_MORE_OPTIONS_KEY);
    }

    private void applyAdblockEnabled(boolean enabledValue) {
        mFilterLists.setEnabled(enabledValue);
        mAcceptableAdsEnabled.setEnabled(enabledValue);
        mAllowedDomains.setEnabled(enabledValue);
        mAllowedConnectionType.setEnabled(enabledValue);
        mMoreOptions.setEnabled(enabledValue);
        mMoreOptions.setVisible(AdblockController.getInstance().areMoreOptionsEnabled());
    }

    private void synchronizePreferences() {
        boolean enabled = AdblockController.getInstance().isEnabled();
        mAdblockEnabled.setChecked(enabled);
        mAdblockEnabled.setOnPreferenceChangeListener(this);
        applyAdblockEnabled(enabled);

        mAcceptableAdsEnabled.setChecked(AdblockController.getInstance().isAcceptableAdsEnabled());
        mAcceptableAdsEnabled.setOnPreferenceChangeListener(this);

        mAllowedConnectionType.setChecked(AdblockController.getInstance().getAllowedConnectionType()
                == AdblockController.ConnectionType.WIFI);
        mAllowedConnectionType.setOnPreferenceChangeListener(this);
    }

    private void maybeEnableMoreOptions() {
        long now = System.currentTimeMillis();
        /* Chromium does not have info about build type in its BuildConfig.
           We'd have patch it and add - which sounds like an overkill for this
           where ENABLE_ASSERTS is pretty close and equivalent unless DCHECKs are
           always on.
           enable_java_asserts = is_java_debug || dcheck_always_on */
        if (BuildConfig.ENABLE_ASSERTS
                || mOnOffTogleTimestamp + ON_OFF_TOGGLE_COUNT_TIME_WINDOW_MS >= now) {
            ++mOnOffClickCount;
        } else {
            mOnOffClickCount = 1;
        }

        mOnOffTogleTimestamp = now;
        if (mOnOffClickCount >= ON_OFF_TOGGLE_COUNT_TO_ENABLE_MORE_OPTIONS) {
            AdblockController.getInstance().setMoreOptionsEnabled(true);
        }
    }

    public AdblockSettingsFragment() {
        mOnOffClickCount = 0;
        mOnOffTogleTimestamp = 0;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.adblock_preferences);
        bindPreferences();
        synchronizePreferences();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity().setTitle(R.string.ad_block_settings_title);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference.getKey().equals(SETTINGS_ENABLED_KEY)) {
            AdblockController.getInstance().setEnabled((Boolean) newValue);

            maybeEnableMoreOptions();

            applyAdblockEnabled((Boolean) newValue);
        } else if (preference.getKey().equals(SETTINGS_ALLOWED_CONNECTION_TYPE_KEY)) {
            if ((Boolean) newValue) {
                AdblockController.getInstance().setAllowedConnectionType(
                        AdblockController.ConnectionType.WIFI);
            } else {
                AdblockController.getInstance().setAllowedConnectionType(
                        AdblockController.ConnectionType.ANY);
            }
        } else {
            assert preference.getKey().equals(SETTINGS_AA_ENABLED_KEY);
            AdblockController.getInstance().setAcceptableAdsEnabled((Boolean) newValue);
        }
        return true;
    }

    @Override
    public void onResume() {
        super.onResume();
        synchronizePreferences();
    }
}

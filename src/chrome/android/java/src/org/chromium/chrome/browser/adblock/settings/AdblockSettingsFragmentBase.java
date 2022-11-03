package org.chromium.chrome.browser.adblock.settings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

public abstract class AdblockSettingsFragmentBase extends PreferenceFragmentCompat
        implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {
    private static final String EXTRA_SHOW_FRAGMENT = "show_fragment";
    private static final String EXTRA_SHOW_FRAGMENT_ARGUMENTS = "show_fragment_args";

    @Override
    public boolean onPreferenceStartFragment(
            PreferenceFragmentCompat caller, Preference preference) {
        startFragment(preference.getFragment(), preference.getExtras());
        return true;
    }

    private void startFragment(String fragmentClass, Bundle args) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClass(getActivity(), getClass());
        intent.putExtra(EXTRA_SHOW_FRAGMENT, fragmentClass);
        intent.putExtra(EXTRA_SHOW_FRAGMENT_ARGUMENTS, args);
        startActivity(intent);
    }
}

package org.chromium.chrome.browser.adblock.settings;

import android.os.Bundle;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.adblock.settings.AdblockSettingsFragmentBase;

public class AdblockMoreOptionsFragment extends AdblockSettingsFragmentBase {
    public AdblockMoreOptionsFragment() {}

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.adblock_more_options);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity().setTitle(R.string.fragment_adblock_more_options_custom_filter_lists_title);
    }
}

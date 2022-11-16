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

import androidx.preference.PreferenceFragmentCompat;

import org.chromium.components.adblock.R;

public class AdblockMoreOptionsFragment extends PreferenceFragmentCompat {
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

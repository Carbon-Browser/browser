/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.chrome.browser.adblock.settings;

import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

import androidx.fragment.app.ListFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.adblock.AdblockController;
import org.chromium.chrome.browser.adblock.settings.AdblockFilterListsAdapter;

public class AdblockFilterListsFragment extends ListFragment {
    private AdblockFilterListsAdapter mFilterListsAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.fragment_adblock_settings_filter_lists_title);
        mFilterListsAdapter =
                new AdblockFilterListsAdapter(getActivity(), AdblockController.getInstance());
        setListAdapter(mFilterListsAdapter);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ListView listView = getListView();
        listView.setDivider(null);
        listView.setItemsCanFocus(true);
    }

    @Override
    public void onStart() {
        super.onStart();
        mFilterListsAdapter.start();
    }
}

/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.components.adblock.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;

import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.R;

import java.util.List;

public class AdblockAllowedDomainsFragment extends AdblockCustomItemFragment {
    public AdblockAllowedDomainsFragment() {}

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity().setTitle(R.string.fragment_adblock_settings_allowed_domains_title);
    }

    @Override
    protected List<String> getItems() {
        return AdblockController.getInstance().getAllowedDomains();
    }

    @Override
    protected String getCustomItemTextViewText() {
        return getString(R.string.fragment_adblock_settings_allowed_domains);
    }

    @Override
    protected String getCustomItemEditTextHint() {
        return getString(R.string.fragment_adblock_settings_add_domain);
    }

    @Override
    protected String getCustomItemTextViewContentDescription() {
        return new String("Add allowed domain text field");
    }

    @Override
    protected String getCustomItemAddButtonContentDescription() {
        return new String("Allowed domain add button");
    }

    @Override
    protected String getCustomItemRemoveButtonContentDescription() {
        return new String("Allowed domain remove button");
    }

    @Override
    protected void addItemImpl(String domain) {
        AdblockController.getInstance().addAllowedDomain(domain);
    }

    @Override
    protected void removeItemImpl(String domain) {
        AdblockController.getInstance().removeAllowedDomain(domain);
    }
}

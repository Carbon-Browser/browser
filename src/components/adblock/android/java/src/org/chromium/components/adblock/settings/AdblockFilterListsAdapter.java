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

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.TextView;

import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.R;
import org.chromium.components.browser_ui.settings.ChromeBaseCheckBoxPreference;

import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class AdblockFilterListsAdapter extends BaseAdapter implements OnClickListener {
    private final LayoutInflater mLayoutInflater;
    private final Context mContext;
    private final AdblockController mController;

    public AdblockFilterListsAdapter(Context context, AdblockController controller) {
        this.mContext = context;
        mLayoutInflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        this.mController = controller;
    }

    public void start() {
        notifyDataSetChanged();
    }

    // BaseAdapter:

    @Override
    public int getCount() {
        return mController.getRecommendedSubscriptions(mContext).size();
    }

    @Override
    public Object getItem(int pos) {
        return mController.getRecommendedSubscriptions(mContext).get(pos);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (convertView == null) {
            view = mLayoutInflater.inflate(R.layout.adblock_filter_lists_list_item, null);
        }

        AdblockController.Subscription item = (AdblockController.Subscription) getItem(position);
        view.setOnClickListener(this);
        view.setTag(item.url());

        CheckBox checkBox = view.findViewById(R.id.checkbox);
        List<AdblockController.Subscription> subscriptions = mController.getSelectedSubscriptions();
        boolean subscribed = false;
        for (AdblockController.Subscription subscription : subscriptions) {
            if (subscription.url().equals(item.url())) {
                subscribed = true;
                break;
            }
        }
        checkBox.setChecked(subscribed);
        checkBox.setContentDescription(item.title() + "filer list item checkbox");

        TextView description = view.findViewById(R.id.name);
        description.setText(item.title());
        description.setContentDescription(item.title() + "filer list item title text");
        return view;
    }

    @Override
    public void onClick(View view) {
        URL url = (URL) view.getTag();
        TextView description = view.findViewById(R.id.name);
        String title = description.getText().toString();

        CheckBox checkBox = view.findViewById(R.id.checkbox);
        if (checkBox.isChecked()) {
            mController.unselectSubscription(new AdblockController.Subscription(url, title));
        } else {
            mController.selectSubscription(new AdblockController.Subscription(url, title));
        }

        notifyDataSetChanged();
    }
}

// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox.suggestions;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.LayoutManager;

import org.chromium.base.TraceEvent;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.modelutil.SimpleRecyclerViewAdapter;

import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import java.util.UUID;

import android.widget.Filter;
import android.widget.Filterable;
import java.net.URL;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import org.chromium.chrome.browser.monetization.VeveSuggestionObj;
import org.json.JSONObject;
import org.json.JSONArray;

import java.util.ArrayList;

/** ModelListAdapter for OmniboxSuggestionsDropdown (RecyclerView version). */
class OmniboxSuggestionsDropdownAdapter extends SimpleRecyclerViewAdapter implements Filterable {
    public interface ResultsCommunicator {
        void onNewSuggestionsReceived(ArrayList<VeveSuggestionObj> list);
    }

    private int mSelectedItem = RecyclerView.NO_POSITION;
    private LayoutManager mLayoutManager;

    private String deviceID;
    private SharedPreferences mPrefs;

    private ResultsCommunicator mCommunicator;

    private String globalResults;

    public void setCommunicator(ResultsCommunicator communicator) {
        mCommunicator = communicator;
    }

    OmniboxSuggestionsDropdownAdapter(ModelList data) {
        super(data);

        if (deviceID == null) {
            if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

            deviceID = mPrefs.getString("unique_id", null);
            if (deviceID == null) {
                deviceID = UUID.randomUUID().toString().replace("-", "").substring(0, 16);
                mPrefs.edit().putString("unique_id", deviceID).apply();
            }
        }
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView view) {
        super.onAttachedToRecyclerView(view);

        mLayoutManager = view.getLayoutManager();
        mSelectedItem = RecyclerView.NO_POSITION;
    }

    @Override
    public void onViewRecycled(ViewHolder holder) {
        super.onViewRecycled(holder);
        if (holder == null || holder.itemView == null) return;
        holder.itemView.setSelected(false);
    }

    /**
     * @return Index of the currently highlighted view.
     */
    int getSelectedViewIndex() {
        return mSelectedItem;
    }

    /** @return Currently selected view. */
    @Nullable
    View getSelectedView() {
        if (mLayoutManager == null) return null;
        if (mSelectedItem < 0 || mSelectedItem >= getItemCount()) return null;

        View currentSelectedView = mLayoutManager.findViewByPosition(mSelectedItem);
        if (currentSelectedView != null) {
            return currentSelectedView;
        }

        mSelectedItem = RecyclerView.NO_POSITION;
        return null;
    }

    /** Ensures selection is reset to beginning of the list. */
    void resetSelection() {
        setSelectedViewIndex(RecyclerView.NO_POSITION);
    }

    /**
     * Move focus to another view.
     *
     * @param index end index.
     */
    boolean setSelectedViewIndex(int index) {
        if (mLayoutManager == null) return false;
        if (index != RecyclerView.NO_POSITION && (index < 0 || index >= getItemCount())) {
            return false;
        }

        View previousSelectedView = mLayoutManager.findViewByPosition(mSelectedItem);
        if (previousSelectedView != null) {
            previousSelectedView.setSelected(false);
        }

        mSelectedItem = index;
        mLayoutManager.scrollToPosition(index);

        View currentSelectedView = mLayoutManager.findViewByPosition(index);
        if (currentSelectedView != null) {
            currentSelectedView.setSelected(true);
        }

        return true;
    }

    @Override
    protected View createView(ViewGroup parent, int viewType) {
        // This skips measuring Adapter.CreateViewHolder, which is final, but it capture
        // the creation of a view holder.
        try (TraceEvent tracing =
                        TraceEvent.scoped("OmniboxSuggestionsList.CreateView", "type:" + viewType);
                SuggestionsMetrics.TimingMetric metric =
                        SuggestionsMetrics.recordSuggestionViewCreateTime()) {
            return super.createView(parent, viewType);
        }
    }

    @Override
    public Filter getFilter() {
        return new Filter() {
            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                FilterResults results = new FilterResults();
                if (constraint != null) {
                    HttpURLConnection conn = null;
                    InputStream input = null;
                    String userText = constraint.toString();
                    try {
                        URL url = new URL("http://pocu60.siteplug.com/sssapi?o=pocu60&s=126380&kw=" + userText
                        + "&itype=cs&f=json&i=0&n=2&af=0&di=" + deviceID);
                        conn = (HttpURLConnection) url.openConnection();
                        input = conn.getInputStream();
                        InputStreamReader reader = new InputStreamReader(input, "UTF-8");
                        BufferedReader buffer = new BufferedReader(reader, 8192);
                        StringBuilder builder = new StringBuilder();
                        String line;
                        while ((line = buffer.readLine()) != null) {
                            builder.append(line);
                        }

                        globalResults = builder.toString();
                    } catch (Exception e) {

                    } finally {
                        if (input != null) {
                            try {
                                input.close();
                            } catch (Exception e) {

                            }
                        }
                        if (conn != null) conn.disconnect();
                    }
                }
                return results;
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                if (results != null && mCommunicator != null && globalResults != null) {
                    try {
                        ArrayList<VeveSuggestionObj> tempArray = new ArrayList();

                        JSONObject jsonObject = new JSONObject(globalResults);
                        JSONObject jsonObjectTwo = jsonObject.getJSONObject("ads");

                        JSONArray jsonArray = jsonObjectTwo.getJSONArray("ad");

                        for (int i = 0; i != jsonArray.length(); i++) {
                            JSONObject jsonInnerObj = (JSONObject)jsonArray.getJSONObject(i);
                            String brand = jsonInnerObj.getString("brand");
                            String url = jsonInnerObj.getString("rurl");
                            String impurl = jsonInnerObj.getString("impurl");
                            String durl = jsonInnerObj.getString("durl");

                            VeveSuggestionObj veveSuggestionObj = new VeveSuggestionObj(brand, url, impurl, durl);

                            tempArray.add(veveSuggestionObj);
                        }

                        mCommunicator.onNewSuggestionsReceived(tempArray);
                    } catch (Exception ignore) { }
                }
            }
        };
    }
}

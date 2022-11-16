// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history_clusters;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;

import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableItemView;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableListUtils;

class HistoryClustersItemView extends SelectableItemView<ClusterVisit> {
    /**
     * Constructor for inflating from XML.
     */
    public HistoryClustersItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mEndButtonView.setVisibility(VISIBLE);
        mEndButtonView.setImageResource(R.drawable.btn_delete_24dp);
        mEndButtonView.setContentDescription(getContext().getString((R.string.remove)));
        ApiCompatibilityUtils.setImageTintList(mEndButtonView,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_secondary_tint_list));
        mEndButtonView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        mEndButtonView.setPaddingRelative(getResources().getDimensionPixelSize(
                                                  R.dimen.visit_item_remove_button_lateral_padding),
                getPaddingTop(),
                getResources().getDimensionPixelSize(
                        R.dimen.visit_item_remove_button_lateral_padding),
                getPaddingBottom());
    }

    @Override
    protected void onClick() {}

    void setTitleText(CharSequence text) {
        mTitleView.setText(text);
        SelectableListUtils.setContentDescriptionContext(getContext(), mEndButtonView,
                text.toString(), SelectableListUtils.ContentDescriptionSource.REMOVE_BUTTON);
    }

    void setHostText(CharSequence text) {
        mDescriptionView.setText(text);
    }

    void setIconDrawable(Drawable drawable) {
        super.setStartIconDrawable(drawable);
    }

    void setEndButtonClickHandler(OnClickListener onClickListener) {
        mEndButtonView.setOnClickListener(onClickListener);
    }
}

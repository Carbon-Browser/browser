// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.rewards;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.widget.DialogTitle;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetContent;
import org.chromium.ui.widget.ButtonCompat;

import android.widget.TextView;
import android.graphics.Color;
import android.graphics.Shader;
import android.graphics.LinearGradient;
import org.chromium.chrome.browser.rewards.RewardsAPIBridge;
import androidx.recyclerview.widget.RecyclerView;
import org.chromium.chrome.browser.rewards.RewardsRecyclerAdapter;
import org.chromium.chrome.browser.rewards.RewardsCommunicator;
import androidx.recyclerview.widget.LinearLayoutManager;
import com.bumptech.glide.request.target.DrawableImageViewTarget;
import com.bumptech.glide.Glide;
import android.widget.ImageView;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import 	android.content.res.Configuration;

/**
 * Bottom sheet content for the screen which allows a parent to approve or deny a website.
 */
class RewardsBottomSheetContent implements BottomSheetContent {
    private static final String TAG = "WebsiteApprovalSheetContent";
    private final Context mContext;
    private final View mContentView;

    private RewardsAPIBridge mRewardsBridge;

    public RewardsBottomSheetContent(Context context, RewardsCommunicator rewardsCommunicator) {
        mContext = context;
        mContentView = (LinearLayout) LayoutInflater.from(mContext).inflate(
                R.layout.rewards_bottom_sheet, null);

        boolean isDarkMode = false;
        int nightModeFlags =
            context.getResources().getConfiguration().uiMode &
            Configuration.UI_MODE_NIGHT_MASK;
        switch (nightModeFlags) {
            case Configuration.UI_MODE_NIGHT_YES:
                 isDarkMode = true;
                 break;

            case Configuration.UI_MODE_NIGHT_NO:
                 isDarkMode = false;
                 break;

            case Configuration.UI_MODE_NIGHT_UNDEFINED:

                 break;
        }

        TextView mBottomSheetTitle = mContentView.findViewById(R.id.rewards_bottom_sheet_title);
        mBottomSheetTitle.setTextColor(Color.parseColor(isDarkMode ? "#ffffff" : "#000000"));

        mRewardsBridge = RewardsAPIBridge.getInstance();

        TextView mBalanceTextView = mContentView.findViewById(R.id.bottom_sheet_rewards_balance);
        mBalanceTextView.setText(mRewardsBridge.getTotalCreditBalance() + " points");
        Shader textShader = new LinearGradient(0, 0, 250, 0,
            new int[]{Color.parseColor("#FF320A"),Color.parseColor("#FF9133")},
           null, Shader.TileMode.CLAMP);
        mBalanceTextView.getPaint().setShader(textShader);

        TextView mRewardErrorMessage = mContentView.findViewById(R.id.reward_error_message);
        LinearLayout mLoadingIndicatorContainer = mContentView.findViewById(R.id.reward_loading_container);
        ImageView mLoadingIndicator = mContentView.findViewById(R.id.reward_loading);
        Glide.with(mContentView.getContext())
            .load("https://hydrisapps.com/carbon/android-resources/images/rewards_loading.gif")
            // .load("http://qnni7t2n4hc7770iq8npfli8u4.ingress.europlots.com/wp-content/uploads/2022/12/loading.gif")
            .thumbnail(0.05f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .into(new DrawableImageViewTarget(mLoadingIndicator));

        RecyclerView mRewardsRecyclerView = mContentView.findViewById(R.id.rewards_recyclerview);
        mRewardsRecyclerView.setLayoutManager(new LinearLayoutManager(mContentView.getContext()));
        mRewardsRecyclerView.setAdapter(new RewardsRecyclerAdapter(mContentView.getContext(), mLoadingIndicatorContainer, mRewardErrorMessage, rewardsCommunicator, isDarkMode));
    }

    @Override
    public View getContentView() {
        return mContentView;
    }

    @Override
    @Nullable
    public View getToolbarView() {
        return null;
    }

    @Override
    public int getVerticalScrollOffset() {
        return 0;
    }

    @Override
    public int getPeekHeight() {
        return HeightMode.DISABLED;
    }

    @Override
    public float getHalfHeightRatio() {
        return HeightMode.DISABLED;
    }

    @Override
    public float getFullHeightRatio() {
        return HeightMode.WRAP_CONTENT;
    }

    @Override
    public void destroy() {}

    @Override
    @ContentPriority
    public int getPriority() {
        return ContentPriority.HIGH;
    }

    @Override
    public boolean swipeToDismissEnabled() {
        return true;
    }

    @Override
    public int getSheetContentDescriptionStringId() {
        return R.string.prefs_rewards_title;
    }

    @Override
    public int getSheetHalfHeightAccessibilityStringId() {
        // Half-height is disabled so no need for an accessibility string.
        return R.string.prefs_rewards_title;
    }

    @Override
    public int getSheetFullHeightAccessibilityStringId() {
        return R.string.prefs_rewards_title;
    }

    @Override
    public int getSheetClosedAccessibilityStringId() {
        return R.string.prefs_rewards_title;
    }
}

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.rewards;

import androidx.browser.customtabs.CustomTabsIntent;
import org.chromium.chrome.browser.ChromeBaseAppCompatActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.content.Intent;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;
import androidx.vectordrawable.graphics.drawable.VectorDrawableCompat;
import android.net.Uri;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import android.provider.Browser;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider.CustomTabsUiType;
import androidx.appcompat.widget.Toolbar;
import android.view.View;
import android.widget.Button;
import com.hbb20.CountryCodePicker;
import android.widget.EditText;
import android.widget.ImageView;

import com.bumptech.glide.request.RequestOptions;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;

import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;
import android.widget.TextView;
import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;

public class RewardsRedeemActivity extends ChromeBaseAppCompatActivity implements RewardsAPIBridge.RewardsRedeemCommunicator {

    private String mImageUrl;
    private String mValueDollar;
    private int mValuePoints;
    private String mName;
    private String mID;

    private SnackbarManager mSnackManager;

    @Override
    public void onRewardRedeemed() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                showSnackbar(getResources().getString(R.string.reward_redeemed));

                SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
                mPrefs.edit().putInt("total_credit_balance", RewardsAPIBridge.getInstance().getTotalCreditBalance() - mValuePoints).apply();
            }
        });
    }

    @Override
    public void onRewardRedeemError() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                showSnackbar(getResources().getString(R.string.reward_redeemed_error));
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setTitle(R.string.preference_rewards);

        setContentView(R.layout.rewards_redeem_activity);

        Toolbar actionBar = findViewById(R.id.action_bar);
        setSupportActionBar(actionBar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            mImageUrl = extras.getString("rewardImageUrl");
            mValueDollar = extras.getString("rewardValueDollar");
            mValuePoints = extras.getInt("rewardValuePoints");
            mName = extras.getString("rewardName");
            mID = extras.getString("rewardId");
        }

        EditText mEmailEditText = findViewById(R.id.rewards_email_edittext);
        EditText mEmailEditTextConfirm = findViewById(R.id.rewards_email_confirm_edittext);
        CountryCodePicker mCCP = findViewById(R.id.reward_country_picker);

        Button mRedeemConfirmButton = findViewById(R.id.rewards_redeem_confirm);
        mRedeemConfirmButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (RewardsAPIBridge.getInstance().getTotalCreditBalance() < mValuePoints) {
                    showSnackbar(getResources().getString(R.string.reward_error_points));
                    return;
                }

                String mFirstEmail = mEmailEditText.getText().toString();
                String mSecondEmail = mEmailEditTextConfirm.getText().toString();

                if (mFirstEmail.length() == 0 || mSecondEmail.length() == 0) {
                    showSnackbar(getResources().getString(R.string.reward_error_email_empty));
                    return;
                }

                if (!mFirstEmail.equals(mSecondEmail)) {
                    showSnackbar(getResources().getString(R.string.reward_error_email_mismatch));
                    return;
                }

                String mCountryCode = mCCP.getSelectedCountryCode();
                String mCountryName = mCCP.getSelectedCountryName();

                String mCountryString = mCountryCode + " " + mCountryName;

                RewardsAPIBridge.getInstance().redeemReward(mCountryString, mFirstEmail, mID, RewardsRedeemActivity.this);
            }
        });

        mSnackManager = new SnackbarManager(this, findViewById(android.R.id.content), null);

        TextView mPointsTextView = findViewById(R.id.rewards_redeem_value_points);
        mPointsTextView.setText(mValuePoints+" pts");

        TextView mMonetaryTextView = findViewById(R.id.rewards_redeem_value_monetary);
        mMonetaryTextView.setText(mValueDollar);

        if (mImageUrl.contains("amazon")) {
            mPointsTextView.setTextColor(getResources().getColor(android.R.color.white));
            mMonetaryTextView.setTextColor(getResources().getColor(android.R.color.white));
        }

        ImageView mRewardImage = findViewById(R.id.rewards_redeem_imageview);
        final float density = getResources().getDisplayMetrics().density;
        final int valueInDp = (int)(10 * density);
        Glide.with(mRewardImage)
            .load(mImageUrl)
            .thumbnail(0.05f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .fitCenter()
            //.apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
            .transform(new RoundedCorners(valueInDp))
            .into(new CustomTarget<Drawable>() {
                @Override
                public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                    if (mRewardImage.getDrawable() == null)
                        mRewardImage.setImageDrawable(resource);
                }

                @Override
                public void onLoadCleared(@Nullable Drawable placeholder) {

                }
            });
    }

    private void showSnackbar(String message) {
        Snackbar snackbar =
                Snackbar.make(message,
                                new SnackbarManager.SnackbarController() {
                                    @Override
                                    public void onAction(Object actionData) {

                                    }
                                    @Override
                                    public void onDismissNoAction(Object actionData) {

                                    }
                                },
                                Snackbar.TYPE_ACTION, Snackbar.UMA_FEED_NTP_STREAM)
                        //.setAction(actionLabel, /*actionData=*/null)
                        .setDuration(6000);

        snackbar.setSingleLine(false);
        mSnackManager.showSnackbar(snackbar);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        // By default, every screen in Settings shows a "Help & feedback" menu item.
        MenuItem help = menu.add(
                Menu.NONE, R.id.menu_id_general_help, Menu.CATEGORY_SECONDARY, R.string.menu_help);
        help.setIcon(VectorDrawableCompat.create(
                getResources(), R.drawable.ic_help_and_feedback, getTheme()));
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (menu.size() == 1) {
            MenuItem item = menu.getItem(0);
            if (item.getIcon() != null) item.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        }
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        } else if (item.getItemId() == R.id.menu_id_general_help) {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://carbon.website/#rewards"));
            // Let Chrome know that this intent is from Chrome, so that it does not close the app when
            // the user presses 'back' button.
            intent.putExtra(Browser.EXTRA_APPLICATION_ID, this.getPackageName());
            intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);
            intent.setPackage(this.getPackageName());
            this.startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}

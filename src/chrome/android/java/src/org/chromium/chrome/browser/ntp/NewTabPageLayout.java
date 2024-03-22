// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.text.Editable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.Callback;
import org.chromium.base.CallbackController;
import org.chromium.base.MathUtils;
import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.layouts.content.InvalidationAwareThumbnailProvider;
import org.chromium.chrome.browser.cryptids.ProbabilisticCryptidRenderer;
import org.chromium.chrome.browser.feed.FeedSurfaceScrollDelegate;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.lens.LensEntryPoint;
import org.chromium.chrome.browser.lens.LensMetrics;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.logo.LogoBridge.Logo;
import org.chromium.chrome.browser.logo.LogoCoordinator;
import org.chromium.chrome.browser.logo.LogoUtils;
import org.chromium.chrome.browser.logo.LogoView;
import org.chromium.chrome.browser.ntp.NewTabPage.OnSearchBoxScrollListener;
import org.chromium.chrome.browser.ntp.search.SearchBoxCoordinator;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.query_tiles.QueryTileSection;
import org.chromium.chrome.browser.query_tiles.QueryTileUtils;
import org.chromium.chrome.browser.suggestions.tile.MostVisitedTilesCoordinator;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.suggestions.tile.TileGroup.Delegate;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.chrome.browser.user_education.IPHCommandBuilder;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.browser.util.BrowserUiUtils;
import org.chromium.chrome.browser.util.BrowserUiUtils.HostSurface;
import org.chromium.chrome.browser.util.BrowserUiUtils.ModuleTypeOnStartAndNtp;
import org.chromium.chrome.features.start_surface.StartSurfaceConfiguration;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.components.browser_ui.widget.displaystyle.DisplayStyleObserver;
import org.chromium.components.browser_ui.widget.displaystyle.HorizontalDisplayStyle;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.browser_ui.widget.highlight.ViewHighlighter;
import org.chromium.components.browser_ui.widget.highlight.ViewHighlighter.HighlightParams;
import org.chromium.components.browser_ui.widget.highlight.ViewHighlighter.HighlightShape;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.text.EmptyTextWatcher;

import 	android.content.res.Configuration;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import android.widget.Button;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.night_mode.ThemeType;
import android.content.Intent;
import android.provider.Settings;
import android.net.Uri;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.app.role.RoleManager;
import android.content.Intent;
import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.wallet.WalletActivity;
import org.chromium.chrome.browser.mirada.MiradaActivity;

import org.json.JSONArray;
import org.json.JSONObject;
import org.chromium.chrome.browser.suggestions.speeddial.helper.RemoteHelper;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import android.view.View;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import org.chromium.base.ContextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.util.DisplayMetrics;
import android.widget.RelativeLayout;

import java.util.ArrayList;
import java.util.List;
import androidx.appcompat.widget.SearchView;
import android.view.WindowManager;
import android.view.Gravity;
import android.view.Window;
import android.app.AlertDialog;
import android.content.DialogInterface;
import java.io.IOException;
import android.widget.EditText;
import java.net.SocketTimeoutException;
import android.widget.ProgressBar;

import java.text.DecimalFormat;
import java.util.Arrays;

import android.text.TextWatcher;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.content_public.browser.LoadUrlParams;
import android.graphics.Color;
import android.widget.TextView;
import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import android.widget.PopupMenu;
import android.view.MenuItem;
import androidx.appcompat.widget.AppCompatImageView;
import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialController;
import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialGridView;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;
import org.chromium.ui.widget.Toast;
import android.widget.FrameLayout;
import org.chromium.base.task.AsyncTask;
import androidx.recyclerview.widget.RecyclerView;
import org.chromium.chrome.browser.ntp.news.NTPNewsRecyclerAdapter;
import androidx.recyclerview.widget.LinearLayoutManager;
import org.chromium.chrome.browser.rewards.RewardsAPIBridge;
import android.content.SharedPreferences;
import android.util.TypedValue;

/**
 * Layout for the new tab page. This positions the page elements in the correct vertical positions.
 * There are no separate phone and tablet UIs; this layout adapts based on the available space.
 */
public class NewTabPageLayout extends LinearLayout implements BackgroundController.NTPBackgroundInterface, RemoteHelper.SpeedDialInterface, RemoteHelper.TakeoverInterface{
    private static final String TAG = "NewTabPageLayout";

    // Used to signify the cached resource value is unset.
    private static final int UNSET_RESOURCE_FLAG = -1;

    private final int mTileGridLayoutBleed;
    private int mSearchBoxTwoSideMargin;
    private final Context mContext;

    private final int mMvtLandscapeLateralMarginTablet;
    private final int mMvtExtraRightMarginTablet;

    private View mMiddleSpacer; // Spacer between toolbar and Most Likely.

    /**
     * The maximum number of tiles to try and fit in a row. On smaller screens, there may not be
     * enough space to fit all of them.
     */
    private static final int MAX_TILE_COLUMNS = 4;

    private LogoCoordinator mLogoCoordinator;
    private SearchBoxCoordinator mSearchBoxCoordinator;
    private QueryTileSection mQueryTileSection;
    private ImageView mCryptidHolder;
    private ViewGroup mMvTilesContainerLayout;
    private MostVisitedTilesCoordinator mMostVisitedTilesCoordinator;

    private OnSearchBoxScrollListener mSearchBoxScrollListener;

    private NewTabPageManager mManager;
    private Activity mActivity;
    private UiConfig mUiConfig;
    private @Nullable DisplayStyleObserver mDisplayStyleObserver;
    private CallbackController mCallbackController = new CallbackController();

    /**
     * Whether the tiles shown in the layout have finished loading.
     * With {@link #mHasShownView}, it's one of the 2 flags used to track initialisation progress.
     */
    private boolean mTilesLoaded;

    /**
     * Whether the view has been shown at least once.
     * With {@link #mTilesLoaded}, it's one of the 2 flags used to track initialization progress.
     */
    private boolean mHasShownView;

    private boolean mSearchProviderHasLogo = true;
    private boolean mSearchProviderIsGoogle;
    private boolean mShowingNonStandardLogo;

    private boolean mInitialized;

    private float mUrlFocusChangePercent;
    private boolean mDisableUrlFocusChangeAnimations;
    private boolean mIsViewMoving;

    /** Flag used to request some layout changes after the next layout pass is completed. */
    private boolean mTileCountChanged;

    private boolean mSnapshotTileGridChanged;
    private boolean mIsIncognito;
    private WindowAndroid mWindowAndroid;
    private boolean mIsNtpAsHomeSurfaceOnTablet;

    /**
     * Vertical inset to add to the top and bottom of the search box bounds. May be 0 if no inset
     * should be applied. See {@link Rect#inset(int, int)}.
     */
    private int mSearchBoxBoundsVerticalInset;

    private FeedSurfaceScrollDelegate mScrollDelegate;

    private NewTabPageUma mNewTabPageUma;

    private RemoteHelper remoteDappsHelper = new RemoteHelper((RemoteHelper.SpeedDialInterface)this);
    private BackgroundController bgController = new BackgroundController();
    private SpeedDialGridView mSpeedDialView;
    private ImageView bgImageView;
    private LinearLayout mMainLayout;
    private LinearLayout mMainLayoutTopSection;
    private TextView mPhotoCredit;
    private RecyclerView mNewsRecyclerView;
    private LinearLayout mTokenTrackerContainer;
    private RewardsAPIBridge mRewardsBridge;
    private View mNoSearchLogoSpacer;

    private AlertDialog mTokenDialog;

    private boolean isDarkMode = true;

    public enum TokenTrackerEnum {
       CHART_DATA,
       TOKEN_LIST
    }

    private int mTileViewWidth;
    private int mTileViewMinIntervalPaddingTablet;
    private Integer mInitialTileNum;
    private Boolean mIsHalfMvtLandscape;
    private Boolean mIsHalfMvtPortrait;
    private boolean mIsSurfacePolishEnabled;
    private boolean mIsSurfacePolishOmniboxColorEnabled;
    private Boolean mIsMvtAllFilledLandscape;
    private Boolean mIsMvtAllFilledPortrait;
    private final int mTileViewIntervalPaddingTabletForPolish;
    private final int mTileViewEdgePaddingTabletForPolish;
    // This offset is added to the transition length when the surface polish flag is enabled in
    // order to make sure the animation is completed.
    private float mTransitionLengthOffset;
    private boolean mIsTablet;

    /** Constructor for inflating from XML. */
    public NewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        Resources res = getResources();
        mTileGridLayoutBleed = res.getDimensionPixelSize(R.dimen.tile_grid_layout_bleed);
        mMvtLandscapeLateralMarginTablet =
                res.getDimensionPixelSize(R.dimen.ntp_search_box_start_margin);
        mMvtExtraRightMarginTablet =
                res.getDimensionPixelSize(
                        R.dimen.mvt_container_to_ntp_right_extra_margin_two_feed_tablet);
        mTileViewWidth =
                getResources().getDimensionPixelOffset(org.chromium.chrome.R.dimen.tile_view_width);
        mTileViewMinIntervalPaddingTablet =
                getResources()
                        .getDimensionPixelOffset(
                                org.chromium.chrome.R.dimen
                                        .tile_carousel_layout_min_interval_margin_tablet);
        mTileViewIntervalPaddingTabletForPolish =
                getResources()
                        .getDimensionPixelOffset(
                                org.chromium.chrome.R.dimen
                                        .tile_view_padding_interval_tablet_polish);
        mTileViewEdgePaddingTabletForPolish =
                getResources()
                        .getDimensionPixelOffset(
                                org.chromium.chrome.R.dimen.tile_view_padding_edge_tablet_polish);


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
    }

    private void showPopupMenu(Context context, View view) {
        PopupMenu popup = new PopupMenu(context, view);
        popup.getMenuInflater().inflate(R.menu.ntp_rewards_menu, popup.getMenu());
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.learn_more_id) {
                    loadUrl("https://carbon.website/#rewards");
                }
                return true;
            }
        });
        popup.show();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mMiddleSpacer = findViewById(R.id.ntp_middle_spacer);
        insertSiteSectionView();

        mRewardsBridge = RewardsAPIBridge.getInstance();

        String textColor = isDarkMode ? "#ffffff" : "#000000";

        mPhotoCredit = findViewById(R.id.ntp_bg_img_credit);
        mMainLayout = findViewById(R.id.ntp_main_layout);
        mMainLayoutTopSection = findViewById(R.id.mainLayoutTopSection);
        bgImageView = findViewById(R.id.bg_image_view);
        if (remoteDappsHelper == null) remoteDappsHelper = new RemoteHelper((RemoteHelper.SpeedDialInterface)this);
        remoteDappsHelper.getDapps((ChromeActivity)getContext(), ((RemoteHelper.SpeedDialInterface)this));
        remoteDappsHelper.getTakeover((ChromeActivity)getContext(), ((RemoteHelper.TakeoverInterface)this));
        if (bgController == null) bgController = new BackgroundController();
        bgController.getBackground((ChromeActivity)getContext(), this);
        final TextView adsBlockedTextView = (TextView)findViewById(R.id.ntp_ads_blocked);
        adsBlockedTextView.setTextColor(Color.parseColor(textColor));
        int nAdsBlocked = mRewardsBridge.getAdsBlocked();
        String nAdsBlockedString = nAdsBlocked + "";
        if (nAdsBlockedString.length() > 3 && nAdsBlockedString.length() < 7) {
            nAdsBlockedString = Math.round(nAdsBlocked/1000) + "K";
        } else if (nAdsBlockedString.length() >= 7) {
            nAdsBlockedString = Math.round(nAdsBlocked/1000000) + "M";
        }
        adsBlockedTextView.setText(nAdsBlockedString);
        final TextView searchesTextView = (TextView)findViewById(R.id.ntp_searches);
        searchesTextView.setTextColor(Color.parseColor(textColor));
        int nSearches = mRewardsBridge.getSearches();
        String nSearchesString = nSearches + "";
        if (nSearchesString.length() > 3 && nSearchesString.length() < 7) {
            nSearchesString = Math.round(nSearches/1000) + "K";
        } else if (nSearchesString.length() >= 7) {
            nSearchesString = Math.round(nSearches/100000) + "M";
        }
        searchesTextView.setText(nSearchesString);

        String mDataSaved = nAdsBlocked+"";
        final TextView dataTextView = (TextView)findViewById(R.id.ntp_data_saved);
        dataTextView.setTextColor(Color.parseColor(textColor));
        if (mDataSaved.length() >= 4) {
            mDataSaved = Math.round((nAdsBlocked/1000)*1.6) + "Mb";
        } else {
            mDataSaved = Math.round(nAdsBlocked*1.6) + "kB";
        }
        dataTextView.setText(mDataSaved);

        final TextView dataTextViewDesc = (TextView)findViewById(R.id.ntp_data_saved_desc);
        final TextView searchesTextViewDesc = (TextView)findViewById(R.id.ntp_searches_desc);
        final TextView adsBlockedTextViewDesc = (TextView)findViewById(R.id.ntp_ads_blocked_desc);
        dataTextViewDesc.setTextColor(Color.parseColor(textColor));
        searchesTextViewDesc.setTextColor(Color.parseColor(textColor));
        adsBlockedTextViewDesc.setTextColor(Color.parseColor(textColor));

        final TextView earnedTodayTextView = (TextView)findViewById(R.id.ntp_rewards_earned_today);
        earnedTodayTextView.setText(mRewardsBridge.getCreditsEarnedToday()+"");
        final TextView totalBalanceTextView = (TextView)findViewById(R.id.ntp_rewards_total);
        totalBalanceTextView.setText(mRewardsBridge.getTotalCreditBalance()+"");

        final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
        boolean isDappsEnabled = mPrefs.getBoolean("ntp_dapps_toggle", true);
        if (isDappsEnabled) {
            LinearLayout mDappsLinearLayout1 = findViewById(R.id.featured_dapps_linearlayout_inner1);
            LinearLayout mDappsLinearLayout2 = findViewById(R.id.featured_dapps_linearlayout_inner2);
            View mViewMoreDappsBtn = findViewById(R.id.view_more_dapps_btn);

            mDappsLinearLayout1.setVisibility(View.VISIBLE);
            mDappsLinearLayout2.setVisibility(View.VISIBLE);
            mViewMoreDappsBtn.setVisibility(View.VISIBLE);
        }

        boolean isSpeedDialEnabled = mPrefs.getBoolean("ntp_speed_dial_toggle", true);
        if (isSpeedDialEnabled) {
            mSpeedDialView = SpeedDialController.inflateSpeedDial(getContext(), 4, null, false);
            if (isDarkMode || !isDappsEnabled) {
                mSpeedDialView.setDark(true);
                mSpeedDialView.updateTileTextTint(true);
            }
            if(mSpeedDialView.getParent() != null) ((ViewGroup)mSpeedDialView.getParent()).removeView(mSpeedDialView);
            mSpeedDialView.setId(R.id.ntp_speed_dial_view);

            LinearLayout speedDialContainer = mMainLayout.findViewById(R.id.speed_dial_container);

            speedDialContainer.addView(mSpeedDialView, 1);
        }
        Button buttonToggleSpeedDial = findViewById(R.id.overflow_button_speed_dial);
        buttonToggleSpeedDial.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PopupMenu popup = new PopupMenu(getContext(), buttonToggleSpeedDial);
                popup.getMenuInflater().inflate(R.menu.menu_ntp_toggle_speed_dial, popup.getMenu());
                popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                    public boolean onMenuItemClick(MenuItem item) {
                        try {
                            boolean isSpeedDialEnabled = mPrefs.getBoolean("ntp_speed_dial_toggle", true);
                            isSpeedDialEnabled = !isSpeedDialEnabled;
                            mPrefs.edit().putBoolean("ntp_speed_dial_toggle", isSpeedDialEnabled).apply();

                            int visibility = isSpeedDialEnabled ? View.VISIBLE : View.GONE;
                            if(mSpeedDialView != null && mSpeedDialView.getParent() != null) ((ViewGroup)mSpeedDialView.getParent()).removeView(mSpeedDialView);
                            if (isSpeedDialEnabled) {
                                mSpeedDialView = SpeedDialController.inflateSpeedDial(getContext(), 4, null, false);
                                if (isDarkMode || !isDappsEnabled) {
                                    mSpeedDialView.setDark(true);
                                    mSpeedDialView.updateTileTextTint(true);
                                }
                                mSpeedDialView.setId(R.id.ntp_speed_dial_view);
                                LinearLayout speedDialContainer = mMainLayout.findViewById(R.id.speed_dial_container);
                                speedDialContainer.addView(mSpeedDialView, 1);
                            }
                        } catch (Exception ignore) {}
                        return true;
                    }
                });
                popup.show();
            }
        });

        boolean isTokenTrackerEnabled = mPrefs.getBoolean("ntp_token_tracker_toggle", true);
        mTokenTrackerContainer = findViewById(R.id.token_tracker_container);
        if (isTokenTrackerEnabled) {
            mTokenTrackerContainer.setVisibility(View.VISIBLE);

            fetchTokenTrackerData();
        }
        Button buttonToggleTokenTracker = findViewById(R.id.overflow_button_token_tracker);
        buttonToggleTokenTracker.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PopupMenu popup = new PopupMenu(v.getContext(), buttonToggleTokenTracker);
                popup.getMenuInflater().inflate(R.menu.menu_ntp_toggle_token_tracker, popup.getMenu());
                popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                    public boolean onMenuItemClick(MenuItem item) {
                        try {
                            int id = item.getItemId();
                            if (id == R.id.menu_ntp_toggle_token_tracker) {
                                boolean isTokenTrackerEnabled = mPrefs.getBoolean("ntp_token_tracker_toggle", true);
                                isTokenTrackerEnabled = !isTokenTrackerEnabled;
                                mPrefs.edit().putBoolean("ntp_token_tracker_toggle", isTokenTrackerEnabled).apply();

                                int visibility = isTokenTrackerEnabled ? View.VISIBLE : View.GONE;

                                mTokenTrackerContainer.setVisibility(visibility);

                                if (isTokenTrackerEnabled) {
                                    mTokenTrackerContainer.setVisibility(View.VISIBLE);

                                    fetchTokenTrackerData();
                                }
                            } else if (id == R.id.menu_ntp_select_tokens) {
                                mTokenDialog = new AlertDialog.Builder(v.getContext(), R.style.TokenSelectorStyle).create();
                                mTokenDialog.setCanceledOnTouchOutside(true);

                                View container = mActivity.getLayoutInflater().inflate(R.layout.token_selector_layout, null);

                                mTokenDialog.show();
                                mTokenDialog.setContentView(container);

                                int msSinceUpdate = Long.valueOf(System.currentTimeMillis()).intValue() - mPrefs.getInt("ntp_token_list_last_update", 0);
                                // cache, refresh after 1 day
                                if (msSinceUpdate <= 86400000) {
                                    String json = mPrefs.getString("cached_token_list", null);
                                    if (json != null && !json.equals("") && !json.trim().isEmpty()) {
                                        processTokenListData(json);
                                    } else {
                                        getTokenTrackerInfo("https://api.coingecko.com/api/v3/coins/list", TokenTrackerEnum.TOKEN_LIST);
                                    }
                                } else {
                                    getTokenTrackerInfo("https://api.coingecko.com/api/v3/coins/list", TokenTrackerEnum.TOKEN_LIST);
                                }

                                mTokenDialog.getWindow().clearFlags(
                                    android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                                            android.view.WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);

                                // Set the dismiss listener
                                mTokenDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                                    @Override
                                    public void onDismiss(DialogInterface dialogInterface) {
                                        mTokenDialog = null;
                                    }
                                });

                                // Set the cancel listener
                                mTokenDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                                    @Override
                                    public void onCancel(DialogInterface dialogInterface) {
                                        mTokenDialog = null;
                                    }
                                });

                                // View mDismissButton = container.findViewById(R.id.wallet_back_button);
                                // mDismissButton.setOnClickListener(new View.OnClickListener() {
                                //         @Override
                                //         public void onClick(View v) {
                                //             mTokenDialog.dismiss();
                                //         }
                                //     });

                                // mTokenDialog.getWindow().setGravity(Gravity.BOTTOM);

                                // Set the attributes to match parent width
                                mTokenDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                                container.setMinimumHeight(v.getResources().getDimensionPixelSize(R.dimen.min_popup_dialog_height));
                            }
                        } catch (Exception ignore) {}
                        return true;
                    }
                });
                popup.show();
            }
        });

        mNewsRecyclerView = findViewById(R.id.ntp_news_recyclerview);
        boolean isNewsEnabled = mPrefs.getBoolean("ntp_news_toggle", true);
        if (isNewsEnabled)
            getNewsArticles();

        Button buttonToggleDapps = findViewById(R.id.overflow_button_dapps);
        buttonToggleDapps.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PopupMenu popup = new PopupMenu(getContext(), buttonToggleDapps);
                popup.getMenuInflater().inflate(R.menu.menu_ntp_toggle_dapps, popup.getMenu());
                popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                    public boolean onMenuItemClick(MenuItem item) {
                        try {
                            int id = item.getItemId();
                            if (id == R.id.menu_ntp_become_featured) {
                                loadUrl("https://carbon.website/adx/");
                            } else {
                                boolean isDappsEnabled = mPrefs.getBoolean("ntp_dapps_toggle", true);
                                isDappsEnabled = !isDappsEnabled;
                                mPrefs.edit().putBoolean("ntp_dapps_toggle", isDappsEnabled).apply();
                                LinearLayout mDappsLinearLayout1 = findViewById(R.id.featured_dapps_linearlayout_inner1);
                                LinearLayout mDappsLinearLayout2 = findViewById(R.id.featured_dapps_linearlayout_inner2);
                                View mViewMoreDappsBtn = findViewById(R.id.view_more_dapps_btn);

                                int visibility = isDappsEnabled ? View.VISIBLE : View.GONE;

                                mDappsLinearLayout1.setVisibility(visibility);
                                mDappsLinearLayout2.setVisibility(visibility);
                                mViewMoreDappsBtn.setVisibility(visibility);

                                if (mSpeedDialView == null) return true;

                                if (!isDappsEnabled) {
                                    mSpeedDialView.setDark(true);
                                    mSpeedDialView.updateTileTextTint(true);
                                } else {
                                    mSpeedDialView.setDark(isDarkMode);
                                    mSpeedDialView.updateTileTextTint(isDarkMode);
                                }
                            }
                        } catch (Exception ignore) {}
                        return true;
                    }
                });
                popup.show();
            }
        });

        Button buttonToggleNews = findViewById(R.id.overflow_button_news);
        buttonToggleNews.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PopupMenu popup = new PopupMenu(getContext(), buttonToggleNews);
                popup.getMenuInflater().inflate(R.menu.menu_ntp_toggle_news, popup.getMenu());
                popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                    public boolean onMenuItemClick(MenuItem item) {
                        boolean isNewsEnabled = mPrefs.getBoolean("ntp_news_toggle", true);
                        isNewsEnabled = !isNewsEnabled;
                        mPrefs.edit().putBoolean("ntp_news_toggle", isNewsEnabled).apply();
                        if (isNewsEnabled) {
                          getNewsArticles();
                        } else {
                          mNewsRecyclerView.setAdapter(null);
                        }
                        return true;
                    }
                });
                popup.show();
            }
        });

        // int variation = ExploreSitesBridge.getVariation();
        // if (ExploreSitesBridge.isExperimental(variation)) {
        //     ViewStub exploreStub = findViewById(R.id.explore_sites_stub);
        //     exploreStub.setLayoutResource(R.layout.experimental_explore_sites_section);
        //     mExploreSectionView = exploreStub.inflate();
        // }

        TextView mEarnedTodayTextView = findViewById(R.id.ntp_rewards_earned_today_desc);
        mEarnedTodayTextView.setTextColor(Color.parseColor(textColor));
        TextView mEarnedTotalTextView = findViewById(R.id.ntp_rewards_total_desc);
        mEarnedTotalTextView.setTextColor(Color.parseColor(textColor));

        TextView mNewsTitle = findViewById(R.id.news_title);
        mNewsTitle.setTextColor(Color.parseColor(textColor));

        String backgroundColor = isDarkMode ? "#262626" : "#ffffff";
        if (!isDarkMode && mMainLayout.findViewById(R.id.cta_view) == null) {
            LinearLayout mEarnedLinearLayout = findViewById(R.id.earned_linearlayout);
            LinearLayout mSavedLinearLayout = findViewById(R.id.savings_linearlayout);
            LinearLayout mComingSoonLinearLayout = findViewById(R.id.coming_soon_linearlayout);
            LinearLayout mDappsLinearLayout = findViewById(R.id.featured_dapps_linearlayout);

            mEarnedLinearLayout.setBackground(getResources().getDrawable(R.drawable.ntp_rounded_dark_background));
            mSavedLinearLayout.setBackground(getResources().getDrawable(R.drawable.ntp_rounded_dark_background));
            mComingSoonLinearLayout.setBackground(getResources().getDrawable(R.drawable.ntp_rounded_dark_background));
            mDappsLinearLayout.setBackground(getResources().getDrawable(R.drawable.ntp_rounded_dark_background));
        }

        NewTabPageLayout ntpLayout = findViewById(R.id.ntp_content);
        ntpLayout.setBackgroundColor(Color.parseColor(backgroundColor));

        TextView mToggleTheme = findViewById(R.id.switch_theme);
        mToggleTheme.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
              if (ChromeSharedPreferences.getInstance().readInt("ui_theme_setting") == ThemeType.LIGHT) {
                ChromeSharedPreferences.getInstance().writeInt("ui_theme_setting", ThemeType.DARK);
              } else {
                ChromeSharedPreferences.getInstance().writeInt("ui_theme_setting", ThemeType.LIGHT);
              }
            }
        });

        TextView mSetDefault = findViewById(R.id.default_browser);
        if (!isCarbonDefault()) {
            mSetDefault.setVisibility(View.VISIBLE);
            mSetDefault.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    //
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        RoleManager roleManager = getContext().getSystemService(RoleManager.class);

                        if (roleManager.isRoleAvailable(RoleManager.ROLE_BROWSER)) {
                            if (!roleManager.isRoleHeld(RoleManager.ROLE_BROWSER)) {
                                // save role manager open count as the second times onwards the dialog is shown,
                                // the system allows the user to click on "don't show again".
                                try {
                                    ((ChromeActivity)getContext()).startActivityForResult(
                                            roleManager.createRequestRoleIntent(RoleManager.ROLE_BROWSER), 69);
                                } catch (Exception ignore) {}
                            }
                        } else {
                          final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                          intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                          getContext().startActivity(intent);
                        }

                    } else {
                        final Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        getContext().startActivity(intent);
                    }
                }
            });
        }

        initialiseWeb3Features();

        fetchTokenTrackerData();
    }

    private void fetchTokenTrackerData() {
        try {
            final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
            boolean isTokenTrackerEnabled = mPrefs.getBoolean("ntp_token_tracker_toggle", true);
            if (!isTokenTrackerEnabled) return;

            String sharedPrefKey = "ntp_token_tracker_data";
            String cachedData = mPrefs.getString(sharedPrefKey, null);

            int msSinceUpdate = Long.valueOf(System.currentTimeMillis()).intValue() - mPrefs.getInt("ntp_token_tracker_last_update", 0);
            // cache, refresh after 1 day
            if (msSinceUpdate <= 3600000 && cachedData != null && !cachedData.equals("") && !cachedData.trim().isEmpty()) {
                processTokenTrackerData(cachedData);
            } else {
              String[] defaultValues = {"bitcoin,BTC,https://assets.coingecko.com/coins/images/1/large/bitcoin.png?1696501400", "ethereum,ETH,https://assets.coingecko.com/coins/images/279/large/ethereum.png?1696501628", "carbon-browser,CSIX,https://assets.coingecko.com/coins/images/28905/large/csix.png?1696527881"};
              TokenTrackerObj[] tokenTrackerObjects = new TokenTrackerObj[defaultValues.length];

              for (int i = 0; i < defaultValues.length; i++) {
                  String sharedPrefKeyPos = "ntp_token_tracker_pos" + i;
                  String pos = mPrefs.getString(sharedPrefKeyPos, defaultValues[i]);
                  String[] posValues = pos.split(",");
                  tokenTrackerObjects[i] = new TokenTrackerObj(posValues[0], "", posValues[1], i);
              }

              getTokenTrackerInfo("https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=" + tokenTrackerObjects[0].id + "%2C" + tokenTrackerObjects[1].id + "%2C" + tokenTrackerObjects[2].id + "&order=market_cap_desc&per_page=100&page=1&sparkline=false&locale=en", TokenTrackerEnum.CHART_DATA);
            }
        } catch (Exception ignore) {

        }
    }

    private void processTokenTrackerData(String result) {
        final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();

        String[] defaultValues = {"bitcoin,BTC,https://assets.coingecko.com/coins/images/1/large/bitcoin.png?1696501400", "ethereum,ETH,https://assets.coingecko.com/coins/images/279/large/ethereum.png?1696501628", "carbon-browser,CSIX,https://assets.coingecko.com/coins/images/28905/large/csix.png?1696527881"};
        TokenTrackerObj[] tokenTrackerObjects = new TokenTrackerObj[defaultValues.length];

        try {
            for (int i = 0; i < defaultValues.length; i++) {
                String sharedPrefKey = "ntp_token_tracker_pos" + i;
                String pos = mPrefs.getString(sharedPrefKey, defaultValues[i]);
                String[] posValues = pos.split(",");
                tokenTrackerObjects[i] = new TokenTrackerObj(posValues[0], "", posValues[1], i);
            }
        } catch (Exception e) {
            // Handle the exception or log it
        }

        try {
            JSONArray jsonArray = new JSONArray(result);
            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObj = jsonArray.getJSONObject(i);
                String id = jsonObj.getString("id");
                for (TokenTrackerObj token : tokenTrackerObjects) {
                    if (id.equals(token.id)) {
                        final String externalUrl = "https://www.coingecko.com/en/coins/" + id;

                        final ImageView logo;
                        TextView priceTextView = null;
                        TextView capTextView = null;
                        TextView tickerTextView = null;
                        LinearLayout tokenLayout = null;
                        if (token.position == 0) {
                            tokenLayout = mTokenTrackerContainer.findViewById(R.id.token_traker_1);
                            priceTextView = (TextView) tokenLayout.findViewById(R.id.token_price);
                            capTextView = (TextView) tokenLayout.findViewById(R.id.token_cap);
                            tickerTextView = (TextView) tokenLayout.findViewById(R.id.token_ticker);
                            logo = tokenLayout.findViewById(R.id.logo);
                        } else if (token.position == 1) {
                            tokenLayout = mTokenTrackerContainer.findViewById(R.id.token_traker_2);
                            priceTextView = (TextView) tokenLayout.findViewById(R.id.token_price);
                            capTextView = (TextView) tokenLayout.findViewById(R.id.token_cap);
                            tickerTextView = (TextView) tokenLayout.findViewById(R.id.token_ticker);
                            logo = tokenLayout.findViewById(R.id.logo);
                        } else {
                            tokenLayout = mTokenTrackerContainer.findViewById(R.id.token_traker_3);
                            priceTextView = (TextView) tokenLayout.findViewById(R.id.token_price);
                            capTextView = (TextView) tokenLayout.findViewById(R.id.token_cap);
                            tickerTextView = (TextView) tokenLayout.findViewById(R.id.token_ticker);
                            logo = tokenLayout.findViewById(R.id.logo);
                        }

                        tokenLayout.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                loadUrl(externalUrl);
                            }
                        });

                        Glide.with(logo)
                            .load(jsonObj.getString("image"))
                            .thumbnail(0.1f)
                            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                            .into(new CustomTarget<Drawable>() {
                                @Override
                                public void onResourceReady(@NonNull Drawable resource, @Nullable Transition<? super Drawable> transition) {
                                    logo.setImageDrawable(resource);
                                }

                                @Override
                                public void onLoadCleared(@Nullable Drawable placeholder) {

                                }
                            });

                        tickerTextView.setText(token.symbol + "/USDT");

                        DecimalFormat dfPrice = new DecimalFormat("#.###");
                        priceTextView.setText("$"+dfPrice.format(jsonObj.getDouble("current_price")));

                        double priceChange = jsonObj.getDouble("price_change_24h");
                        DecimalFormat df = new DecimalFormat("#.#");
                        int chartColor = Color.parseColor("#74d875");
                        String capString = df.format(jsonObj.getDouble("price_change_percentage_24h")) + "%";
                        if (priceChange < 0) {
                            int color = Color.parseColor("#b85550");
                            capTextView.setTextColor(color);
                            capString = capString;
                        } else {
                            capString = "+" + capString;
                        }
                        capTextView.setText(capString);
                        break; // breaks the inner loop as the condition is met
                    }
                }
            }
        } catch (Exception ignore) {

        }
    }

    private void processTokenListData(String result) {
        try {
            JSONArray jsonArray = new JSONArray(result);

            List<TokenTrackerObj> tokenList = new ArrayList<>();

            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);

                String id = jsonObject.getString("id");
                String symbol = jsonObject.getString("symbol");
                String name = jsonObject.getString("name");

                TokenTrackerObj token = new TokenTrackerObj(id, name, symbol, i);
                tokenList.add(token);
            }

            EditText editTextSearch = mTokenDialog.findViewById(R.id.editText_search);

            RecyclerView recyclerView = mTokenDialog.findViewById(R.id.recycler_view);
            recyclerView.setLayoutManager(new LinearLayoutManager(mTokenDialog.getContext()));

            ProgressBar spinner = mTokenDialog.findViewById(R.id.api_loader);

            recyclerView.setVisibility(View.VISIBLE);
            spinner.setVisibility(View.GONE);

            View closeButton = mTokenDialog.findViewById(R.id.close);
            closeButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mTokenDialog != null) mTokenDialog.dismiss();
                }
            });

            TokenTrackerAdapter adapter = new TokenTrackerAdapter(tokenList, new TokenTrackerAdapter.OnItemClickListener() {
                @Override
                public void onItemClick(final int position, final String info) {
                    // Handle the click event here
                    try {
                        final AlertDialog secondDialogBuilder = new AlertDialog.Builder(mActivity).create();

                        View container = mActivity.getLayoutInflater().inflate(R.layout.token_tracker_selector_layout, null);

                        View position1View = container.findViewById(R.id.item_position_1);
                        View position2View = container.findViewById(R.id.item_position_2);
                        View position3View = container.findViewById(R.id.item_position_3);

                        View.OnClickListener onClickListener = new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                try {
                                    final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();

                                    Toast.makeText(view.getContext(), "Tracking new token", Toast.LENGTH_SHORT).show();

                                    int pos = Integer.parseInt((String)view.getTag());

                                    mPrefs.edit().putInt("ntp_token_tracker_last_update", 0).apply();

                                    String sharedPrefKey = "ntp_token_tracker_pos" + pos;
                                    mPrefs.edit().putString(sharedPrefKey, info).apply();

                                    fetchTokenTrackerData();

                                    secondDialogBuilder.dismiss();
                                    secondDialogBuilder.cancel();

                                    mTokenDialog.dismiss();
                                    mTokenDialog = null;
                                } catch (Exception ignore) {}
                            }
                        };

                        secondDialogBuilder.show();

                        secondDialogBuilder.setContentView(container);

                        position1View.setOnClickListener(onClickListener);
                        position2View.setOnClickListener(onClickListener);
                        position3View.setOnClickListener(onClickListener);
                    } catch (Exception ignore) {}
                }
            });
            recyclerView.setAdapter(adapter);

            editTextSearch.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    // Before text changed
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    // As the user types, filter the list or perform the search
                    adapter.getFilter().filter(s);
                }

                @Override
                public void afterTextChanged(Editable s) {
                    // After text changed
                }
            });
        } catch (Exception e) {

        }
    }

    private void getTokenTrackerInfo(String url, TokenTrackerEnum type) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(url);

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setDoOutput(false);
                    conn.setConnectTimeout(20000);
                    conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("GET");
                    conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

                    // handle the response
                    int status = conn.getResponseCode();
                    if (status != 200) {
                        throw new IOException("Post failed with error code " + status);
                    } else {
                        BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                        String inputLine;
                        while ((inputLine = in.readLine()) != null) {
                            response.append(inputLine);
                        }
                        in.close();
                    }
                } catch (SocketTimeoutException timeout) {
                    // Time out, don't set a background - lazy
                } catch (Exception e) {

                } finally {
                    if (conn != null) conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
                if (type == TokenTrackerEnum.CHART_DATA) {
                    mPrefs.edit().putString("ntp_token_tracker_data", result).apply();
                    mPrefs.edit().putInt("ntp_token_tracker_last_update", Long.valueOf(System.currentTimeMillis()).intValue()).apply();
                    processTokenTrackerData(result);
                } else if (type == TokenTrackerEnum.TOKEN_LIST) {
                    mPrefs.edit().putInt("ntp_token_list_last_update", Long.valueOf(System.currentTimeMillis()).intValue()).apply();
                    mPrefs.edit().putString("cached_token_list", result).apply();
                    processTokenListData(result);
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private String formatNumber(long number) {
        if (number >= 1_000_000_000) {
            return String.format("%.1fb", number / 1_000_000_000.0);
        } else if (number >= 1_000_000) {
            return String.format("%.1fm", number / 1_000_000.0);
        } else {
            return String.valueOf(number);
        }
    }

    private boolean isCarbonDefault() {
        Intent browserIntent = new Intent("android.intent.action.VIEW", Uri.parse("http://"));
        ResolveInfo resolveInfo = getContext().getPackageManager().resolveActivity(browserIntent,PackageManager.MATCH_DEFAULT_ONLY);

        // This is the default browser's packageName
        String packageName = resolveInfo.activityInfo.packageName;
        if (packageName.equals("com.browser.tssomas")) return true;

        return false;
    }

    private void initialiseWeb3Features() {
        String textColor = isDarkMode ? "#ffffff" : "#000000";

        // TextView mComingSoonTitle = findViewById(R.id.coming_soon_textview);
        // mComingSoonTitle.setTextColor(Color.parseColor(textColor));

        TextView mFeaturedDappsTitle = findViewById(R.id.featured_daps_textview);
        mFeaturedDappsTitle.setTextColor(Color.parseColor(textColor));

        TextView mSpeedDialTitle = findViewById(R.id.speed_dial_title);
        mSpeedDialTitle.setTextColor(Color.parseColor(textColor));

        // Coming soon
        View walletTile = findViewById(R.id.coming_soon_tile1);
        View stakingTile = findViewById(R.id.coming_soon_tile2);
        View comingSoonTile3 = findViewById(R.id.coming_soon_tile3);
        View comingSoonTile4 = findViewById(R.id.coming_soon_tile4);

        walletTile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent();
                intent.setClass(view.getContext(), WalletActivity.class);

                try {
                    ((ChromeActivity)getContext()).startActivityForResult(intent, 12345);
                } catch (Exception ignore) {}

                // IntentUtils.safeStartActivity(view.getContext(), intent);
            }
        });

        stakingTile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                loadUrl("https://stake.carbon.website/");
            }
        });

        comingSoonTile3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                loadUrl("https://www.ldx.fi/");
            }
        });

        comingSoonTile4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent();
                intent.setClass(view.getContext(), MiradaActivity.class);

                try {
                    ((ChromeActivity)getContext()).startActivityForResult(intent, 123456);
                } catch (Exception ignore) { }
            }
        });

        TextView mWalletTextView = walletTile.findViewById(R.id.speed_dial_tile_textview);
        TextView mStakingTextView = stakingTile.findViewById(R.id.speed_dial_tile_textview);
        TextView mSwapTextView = comingSoonTile3.findViewById(R.id.speed_dial_tile_textview);
        TextView mBridgeTextView = comingSoonTile4.findViewById(R.id.speed_dial_tile_textview);
        mWalletTextView.setText("Wallet");
        mStakingTextView.setText("Staking");
        mSwapTextView.setText("Swap");
        mBridgeTextView.setText("AI");
        mWalletTextView.setTextColor(Color.parseColor(textColor));
        mStakingTextView.setTextColor(Color.parseColor(textColor));
        mSwapTextView.setTextColor(Color.parseColor(textColor));
        mBridgeTextView.setTextColor(Color.parseColor(textColor));

        FrameLayout walletBackground = walletTile.findViewById(R.id.speed_dial_tile_view_icon_background);
        FrameLayout stakingTileBackground = stakingTile.findViewById(R.id.speed_dial_tile_view_icon_background);
        FrameLayout swapBackground = comingSoonTile3.findViewById(R.id.speed_dial_tile_view_icon_background);
        FrameLayout bridgeBackground = comingSoonTile4.findViewById(R.id.speed_dial_tile_view_icon_background);

        walletBackground.setBackground(getResources().getDrawable(R.drawable.speed_dial_icon_background_dark_round));
        stakingTileBackground.setBackground(getResources().getDrawable(R.drawable.speed_dial_icon_background_dark_round));
        swapBackground.setBackground(getResources().getDrawable(R.drawable.speed_dial_icon_background_dark_round));
        bridgeBackground.setBackground(getResources().getDrawable(R.drawable.speed_dial_icon_background_dark_round));

        ImageView walletTileImage = walletTile.findViewById(R.id.speed_dial_tile_view_icon);
        ImageView stakingTileImage = stakingTile.findViewById(R.id.speed_dial_tile_view_icon);
        ImageView swapTileImage = comingSoonTile3.findViewById(R.id.speed_dial_tile_view_icon);
        ImageView bridgeTileImage = comingSoonTile4.findViewById(R.id.speed_dial_tile_view_icon);

        walletTileImage.setBackground(getResources().getDrawable(R.drawable.ic_wallet));
        stakingTileImage.setBackground(getResources().getDrawable(R.drawable.ic_staking));
        swapTileImage.setBackground(getResources().getDrawable(R.drawable.ic_swap));
        bridgeTileImage.setBackground(getResources().getDrawable(R.drawable.ic_ai));
    }

    private void loadUrl(String url) {
      try {
        if (getContext() instanceof ChromeActivity) {
            LoadUrlParams loadUrlParams = new LoadUrlParams(url);
            ((ChromeActivity)getContext()).getActivityTab().loadUrl(loadUrlParams);
        }
      } catch (Exception ignore) {}
    }

    private void getNewsArticles() {
        mNewsRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        mNewsRecyclerView.setAdapter(new NTPNewsRecyclerAdapter(getContext(), isDarkMode));
    }

    /**
     * Initializes the NewTabPageLayout. This must be called immediately after inflation, before
     * this object is used in any other way.
     * @param manager NewTabPageManager used to perform various actions when the user interacts
     *                with the page.
     * @param activity The activity that currently owns the new tab page
     * @param tileGroupDelegate Delegate for {@link TileGroup}.
     * @param searchProviderHasLogo Whether the search provider has a logo.
     * @param searchProviderIsGoogle Whether the search provider is Google.
     * @param scrollDelegate The delegate used to obtain information about scroll state.
     * @param touchEnabledDelegate The {@link TouchEnabledDelegate} for handling whether touch
     *         events are allowed.
     * @param uiConfig UiConfig that provides display information about this view.
     * @param lifecycleDispatcher Activity lifecycle dispatcher.
     * @param uma {@link NewTabPageUma} object recording user metrics.
     * @param isIncognito Whether the new tab page is in incognito mode.
     * @param windowAndroid An instance of a {@link WindowAndroid}
     * @param isNtpAsHomeSurfaceOnTablet {@code true} if the NTP is showing as the home surface on
     *         tablets.
     * @param isSurfacePolishEnabled {@code true} if the NTP surface is polished.
     */
    public void initialize(
            NewTabPageManager manager,
            Activity activity,
            Delegate tileGroupDelegate,
            boolean searchProviderHasLogo,
            boolean searchProviderIsGoogle,
            FeedSurfaceScrollDelegate scrollDelegate,
            TouchEnabledDelegate touchEnabledDelegate,
            UiConfig uiConfig,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            NewTabPageUma uma,
            boolean isIncognito,
            WindowAndroid windowAndroid,
            boolean isNtpAsHomeSurfaceOnTablet,
            boolean isSurfacePolishEnabled,
            boolean isSurfacePolishOmniboxColorEnabled,
            boolean isTablet) {
        TraceEvent.begin(TAG + ".initialize()");
        mScrollDelegate = scrollDelegate;
        mManager = manager;
        mActivity = activity;
        mUiConfig = uiConfig;
        mNewTabPageUma = uma;
        mIsIncognito = isIncognito;
        mWindowAndroid = windowAndroid;
        mIsNtpAsHomeSurfaceOnTablet = isNtpAsHomeSurfaceOnTablet;
        mIsSurfacePolishEnabled = isSurfacePolishEnabled;
        mIsSurfacePolishOmniboxColorEnabled = isSurfacePolishOmniboxColorEnabled;
        Profile profile = Profile.getLastUsedRegularProfile();
        mIsTablet = isTablet;

        if (mIsTablet) {
            mDisplayStyleObserver = this::onDisplayStyleChanged;
            mUiConfig.addObserver(mDisplayStyleObserver);
        } else {
            // On first run, the NewTabPageLayout is initialized behind the First Run Experience,
            // meaning the UiConfig will pickup the screen layout then. However
            // onConfigurationChanged is not called on orientation changes until the FRE is
            // completed. This means that if a user starts the FRE in one orientation, changes an
            // orientation and then leaves the FRE the UiConfig will have the wrong orientation.
            // https://crbug.com/683886.
            mUiConfig.updateDisplayStyle();
        }

        mSearchBoxCoordinator = new SearchBoxCoordinator(getContext(), this);
        mSearchBoxCoordinator.initialize(lifecycleDispatcher, mIsIncognito, mWindowAndroid);
        if (isSurfacePolishEnabled) {
            int searchBoxHeightPolish =
                    getResources().getDimensionPixelSize(R.dimen.ntp_search_box_height_polish);
            mSearchBoxCoordinator.getView().getLayoutParams().height = searchBoxHeightPolish;
            mSearchBoxBoundsVerticalInset =
                    (searchBoxHeightPolish
                                    - getResources()
                                            .getDimensionPixelSize(
                                                    R.dimen.toolbar_height_no_shadow))
                            / 2;
        } else if (!DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity)) {
            mSearchBoxBoundsVerticalInset =
                    getResources()
                            .getDimensionPixelSize(
                                    R.dimen.ntp_search_box_bounds_vertical_inset_modern);
        }
        mTransitionLengthOffset =
                mIsSurfacePolishEnabled
                                && !DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity)
                        ? getResources()
                                .getDimensionPixelSize(
                                        R.dimen.ntp_search_box_transition_length_polish_offset)
                        : 0;

        if (mIsNtpAsHomeSurfaceOnTablet && !mIsSurfacePolishEnabled) {
            // We add extra side margins to the fake search box when multiple column Feeds are
            // shown. There is only one exception that we don't shorten the width of the fake search
            // box: one row of MV tiles in portrait mode.
            mSearchBoxTwoSideMargin =
                    getResources().getDimensionPixelSize(R.dimen.ntp_search_box_start_margin) * 2;
        } else if (mIsSurfacePolishEnabled) {
            updateSearchBoxWidthForPolish();
        }
        initializeLogoCoordinator(searchProviderHasLogo, searchProviderIsGoogle);
        initializeMostVisitedTilesCoordinator(
                profile,
                lifecycleDispatcher,
                tileGroupDelegate,
                touchEnabledDelegate,
                isScrollableMvtEnabled(),
                searchProviderIsGoogle);
        initializeSearchBoxBackground();
        initializeSearchBoxTextView();
        initializeVoiceSearchButton();
        initializeLensButton();
        initializeLayoutChangeListener();

        if (searchProviderIsGoogle && QueryTileUtils.isQueryTilesEnabledOnNtp()) {
            mQueryTileSection =
                    new QueryTileSection(
                            findViewById(R.id.query_tiles), profile, mManager::performSearchQuery);
        }

        manager.addDestructionObserver(NewTabPageLayout.this::onDestroy);
        mInitialized = true;

        TraceEvent.end(TAG + ".initialize()");

        if (mIsSurfacePolishEnabled) {
            setBackground(
                    AppCompatResources.getDrawable(mContext, R.drawable.home_surface_background));
        }
    }

    /**
     * @return The {@link FeedSurfaceScrollDelegate} for this class.
     */
    FeedSurfaceScrollDelegate getScrollDelegate() {
        return mScrollDelegate;
    }

    /** Sets up the search box background or background tint. */
    private void initializeSearchBoxBackground() {
        if (mIsSurfacePolishOmniboxColorEnabled) {
            findViewById(R.id.search_box)
                    .setBackground(
                            AppCompatResources.getDrawable(
                                    mContext,
                                    R.drawable.home_surface_search_box_background_colorful));
            return;
        }

        if (mIsSurfacePolishEnabled) {
            findViewById(R.id.search_box)
                    .setBackground(
                            AppCompatResources.getDrawable(
                                    mContext,
                                    R.drawable.home_surface_search_box_background_neutral));
            return;
        }

        final int elevationDimenId =
                ChromeFeatureList.sBaselineGm3SurfaceColors.isEnabled()
                        ? R.dimen.default_elevation_4
                        : R.dimen.toolbar_text_box_elevation;
        final int searchBoxColor = ChromeColors.getSurfaceColor(getContext(), elevationDimenId);
        final ColorStateList colorStateList = ColorStateList.valueOf(searchBoxColor);
        findViewById(R.id.search_box).setBackgroundTintList(colorStateList);
    }

    /** Sets up the hint text and event handlers for the search box text view. */
    private void initializeSearchBoxTextView() {
        TraceEvent.begin(TAG + ".initializeSearchBoxTextView()");

        mSearchBoxCoordinator.setSearchBoxClickListener(v -> mManager.focusSearchBox(false, null));
        mSearchBoxCoordinator.setSearchBoxTextWatcher(
                new EmptyTextWatcher() {
                    @Override
                    public void afterTextChanged(Editable s) {
                        if (s.length() == 0) return;
                        mManager.focusSearchBox(false, s.toString());
                        mSearchBoxCoordinator.setSearchText("");
                    }
                });
        TraceEvent.end(TAG + ".initializeSearchBoxTextView()");
    }

    private void initializeVoiceSearchButton() {
        TraceEvent.begin(TAG + ".initializeVoiceSearchButton()");
        mSearchBoxCoordinator.addVoiceSearchButtonClickListener(
                v -> mManager.focusSearchBox(true, null));
        updateActionButtonVisibility();
        TraceEvent.end(TAG + ".initializeVoiceSearchButton()");
    }

    private void initializeLensButton() {
        TraceEvent.begin(TAG + ".initializeLensButton()");
        // TODO(b/181067692): Report user action for this click.
        mSearchBoxCoordinator.addLensButtonClickListener(
                v -> {
                    LensMetrics.recordClicked(LensEntryPoint.NEW_TAB_PAGE);
                    mSearchBoxCoordinator.startLens(LensEntryPoint.NEW_TAB_PAGE);
                });
        updateActionButtonVisibility();
        TraceEvent.end(TAG + ".initializeLensButton()");
    }

    private void initializeLayoutChangeListener() {
        TraceEvent.begin(TAG + ".initializeLayoutChangeListener()");
        addOnLayoutChangeListener(
                (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
                    int oldHeight = oldBottom - oldTop;
                    int newHeight = bottom - top;

                    if (oldHeight == newHeight && !mTileCountChanged) return;
                    mTileCountChanged = false;

                    // Re-apply the url focus change amount after a rotation to ensure the views are
                    // correctly placed with their new layout configurations.
                    onUrlFocusAnimationChanged();
                    updateSearchBoxOnScroll();

                    // The positioning of elements may have been changed (since the elements expand
                    // to fill the available vertical space), so adjust the scroll.
                    mScrollDelegate.snapScroll();
                });
        TraceEvent.end(TAG + ".initializeLayoutChangeListener()");
    }

    private void initializeLogoCoordinator(
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle) {
        Callback<LoadUrlParams> logoClickedCallback =
                mCallbackController.makeCancelable(
                        (urlParams) -> {
                            mManager.getNativePageHost()
                                    .loadUrl(urlParams, /* isIncognito= */ false);
                            BrowserUiUtils.recordModuleClickHistogram(
                                    HostSurface.NEW_TAB_PAGE, ModuleTypeOnStartAndNtp.DOODLE);
                        });
        Callback<Logo> onLogoAvailableCallback =
                mCallbackController.makeCancelable(
                        (logo) -> {
                            mSnapshotTileGridChanged = true;
                            mShowingNonStandardLogo = logo != null;
                            maybeKickOffCryptidRendering();
                        });
        Runnable onCachedLogoRevalidatedRunnable =
                mCallbackController.makeCancelable(this::maybeKickOffCryptidRendering);

        // If pull up Feed position is enabled, doodle is not supported since there is not enough
        // room, we don't need to fetch logo image.
        boolean shouldFetchDoodle = !FeedPositionUtils.isFeedPullUpEnabled();
        LogoView logoView = findViewById(R.id.search_provider_logo);
        if (mIsSurfacePolishEnabled) {
            LogoUtils.setLogoViewLayoutParams(
                    logoView,
                    getResources(),
                    DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext()),
                    StartSurfaceConfiguration.SURFACE_POLISH_LESS_BRAND_SPACE.getValue());
        } else if (mIsNtpAsHomeSurfaceOnTablet) {
            logoView.getLayoutParams().height =
                    mContext.getResources().getDimensionPixelSize(R.dimen.ntp_logo_height_shrink);
        }

        mLogoCoordinator =
                new LogoCoordinator(
                        mContext,
                        logoClickedCallback,
                        logoView,
                        shouldFetchDoodle,
                        onLogoAvailableCallback,
                        onCachedLogoRevalidatedRunnable,
                        /* isParentSurfaceShown= */ true,
                        /* visibilityObserver= */ null);
        mLogoCoordinator.initWithNative();
        setSearchProviderInfo(searchProviderHasLogo, searchProviderIsGoogle);
    }

    private void initializeMostVisitedTilesCoordinator(
            Profile profile,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            TileGroup.Delegate tileGroupDelegate,
            TouchEnabledDelegate touchEnabledDelegate,
            boolean isScrollableMvtEnabled,
            boolean searchProviderIsGoogle) {
        // assert mMvTilesContainerLayout != null;
        //
        // int maxRows = 2;
        // if (searchProviderIsGoogle && QueryTileUtils.isQueryTilesEnabledOnNtp()) {
        //     maxRows = QueryTileSection.getMaxRowsForMostVisitedTiles(getContext());
        // }
        //
        // mMostVisitedTilesCoordinator =
        //         new MostVisitedTilesCoordinator(
        //                 mActivity,
        //                 activityLifecycleDispatcher,
        //                 mMvTilesContainerLayout,
        //                 mWindowAndroid,
        //                 /* shouldShowSkeletonUIPreNative= */ false,
        //                 isScrollableMvtEnabled,
        //                 maxRows,
        //                 () -> mSnapshotTileGridChanged = true,
        //                 () -> {
        //                     if (mUrlFocusChangePercent == 1f) mTileCountChanged = true;
        //                 });
        //
        // mMostVisitedTilesCoordinator.initWithNative(
        //         mManager, tileGroupDelegate, touchEnabledDelegate);
    }

    /** Updates the search box when the parent view's scroll position is changed. */
    void updateSearchBoxOnScroll() {
        if (mDisableUrlFocusChangeAnimations || mIsViewMoving) return;

        // When the page changes (tab switching or new page loading), it is possible that events
        // (e.g. delayed view change notifications) trigger calls to these methods after
        // the current page changes. We check it again to make sure we don't attempt to update the
        // wrong page.
        if (!mManager.isCurrentPage()) return;

        if (mSearchBoxScrollListener != null) {
            mSearchBoxScrollListener.onNtpScrollChanged(getToolbarTransitionPercentage());
        }
    }

    /**
     * Calculates the percentage (between 0 and 1) of the transition from the search box to the
     * omnibox at the top of the New Tab Page, which is determined by the amount of scrolling and
     * the position of the search box.
     *
     * @return the transition percentage
     */
    float getToolbarTransitionPercentage() {
        // During startup the view may not be fully initialized.
        if (!mScrollDelegate.isScrollViewInitialized()) return 0f;

        if (isSearchBoxOffscreen()) {
            // getVerticalScrollOffset is valid only for the scroll view if the first item is
            // visible. If the search box view is offscreen, we must have scrolled quite far and we
            // know the toolbar transition should be 100%. This might be the initial scroll position
            // due to the scroll restore feature, so the search box will not have been laid out yet.
            return 1f;
        }

        // During startup the view may not be fully initialized, so we only calculate the current
        // percentage if some basic view properties (position of the search box) are sane.
        int searchBoxTop = getSearchBoxView().getTop();
        if (searchBoxTop == 0) return 0f;

        // For all other calculations, add the search box padding, because it defines where the
        // visible "border" of the search box is.
        searchBoxTop += getSearchBoxView().getPaddingTop();

        final int scrollY = mScrollDelegate.getVerticalScrollOffset();
        // Use int pixel size instead of float dimension to avoid precision error on the percentage.
        final float transitionLength =
                getResources().getDimensionPixelSize(R.dimen.ntp_search_box_transition_length)
                        + mTransitionLengthOffset;
        // Tab strip height is zero on phones, nonzero on tablets.
        int tabStripHeight = getResources().getDimensionPixelSize(R.dimen.tab_strip_height);

        // |scrollY - searchBoxTop + tabStripHeight| gives the distance the search bar is from the
        // top of the tab.
        return MathUtils.clamp(
                (scrollY
                                - (searchBoxTop + mTransitionLengthOffset)
                                + tabStripHeight
                                + transitionLength)
                        / transitionLength,
                0f,
                1f);
    }

    private void insertSiteSectionView() {
        // int insertionPoint = indexOfChild(mMiddleSpacer) + 1;
        //
        // if (ChromeFeatureList.sSurfacePolish.isEnabled()) {
        //     mMvTilesContainerLayout =
        //             (ViewGroup)
        //                     LayoutInflater.from(getContext())
        //                             .inflate(R.layout.mv_tiles_container_polish, this, false);
        // } else {
        //     mMvTilesContainerLayout =
        //             (ViewGroup)
        //                     LayoutInflater.from(getContext())
        //                             .inflate(R.layout.mv_tiles_container, this, false);
        // }
        // addView(mMvTilesContainerLayout, insertionPoint);
        // mMvTilesContainerLayout.setVisibility(View.GONE);
        // The page contents are initially hidden; otherwise they'll be drawn centered on the
        // page before the tiles are available and then jump upwards to make space once the
        // tiles are available.
        if (getVisibility() != View.VISIBLE) setVisibility(View.VISIBLE);
    }

    /**
     * @return The fake search box view.
     */
    public View getSearchBoxView() {
        return mSearchBoxCoordinator.getView();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mIsNtpAsHomeSurfaceOnTablet && isScrollableMvtEnabled()) {
            if (mIsSurfacePolishEnabled) {
                calculateTabletMvtWidth(MeasureSpec.getSize(widthMeasureSpec));
            } else {
                calculateTabletMvtMargin(MeasureSpec.getSize(widthMeasureSpec));
            }
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        unifyElementWidths();
    }

    /** Updates the width of the MV tiles container when used in NTP on the tablet. */
    private void calculateTabletMvtWidth(int widthMeasureSpec) {
        // if (mMvTilesContainerLayout.getVisibility() == GONE) return;
        //
        // if (mInitialTileNum == null) {
        //     mInitialTileNum = ((ViewGroup) findViewById(R.id.mv_tiles_layout)).getChildCount();
        // }
        //
        // int currentOrientation = getResources().getConfiguration().orientation;
        // if ((currentOrientation == Configuration.ORIENTATION_LANDSCAPE
        //                 && mIsMvtAllFilledLandscape == null)
        //         || (currentOrientation == Configuration.ORIENTATION_PORTRAIT
        //                 && mIsMvtAllFilledPortrait == null)) {
        //     boolean isAllFilled =
        //             mInitialTileNum * mTileViewWidth
        //                             + (mInitialTileNum - 1)
        //                                     * mTileViewIntervalPaddingTabletForPolish
        //                             + 2 * mTileViewEdgePaddingTabletForPolish
        //                     <= widthMeasureSpec;
        //     if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE) {
        //         mIsMvtAllFilledLandscape = isAllFilled;
        //     } else {
        //         mIsMvtAllFilledPortrait = isAllFilled;
        //     }
        //     updateMvtOnTabletForPolish();
        // }
    }

    /**
     * Update the right margin for the MV tiles container if needed to have half tile element
     * in the end of the MV tiles when used in NTP on the tablet.
     */
    private void calculateTabletMvtMargin(int widthMeasureSpec) {
        // if (mMvTilesContainerLayout.getVisibility() == GONE) return;
        //
        // if (mInitialTileNum == null) {
        //     mInitialTileNum = ((ViewGroup) findViewById(R.id.mv_tiles_layout)).getChildCount();
        // }
        //
        // int currentOrientation = getResources().getConfiguration().orientation;
        // if ((currentOrientation == Configuration.ORIENTATION_LANDSCAPE
        //                 && mIsHalfMvtLandscape == null)
        //         || (currentOrientation == Configuration.ORIENTATION_PORTRAIT
        //                 && mIsHalfMvtPortrait == null)) {
        //     MarginLayoutParams marginLayoutParams =
        //             (MarginLayoutParams) mMvTilesContainerLayout.getLayoutParams();
        //     int mvtContainerWidth =
        //             widthMeasureSpec
        //                     - marginLayoutParams.leftMargin
        //                     - marginLayoutParams.rightMargin;
        //     boolean isHalfMvt =
        //             mInitialTileNum * mTileViewWidth
        //                             + (mInitialTileNum - 1) * mTileViewMinIntervalPaddingTablet
        //                     > mvtContainerWidth;
        //     if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE) {
        //         mIsHalfMvtLandscape = isHalfMvt;
        //     } else {
        //         mIsHalfMvtPortrait = isHalfMvt;
        //     }
        //     updateTilesLayoutLeftAndRightMarginsOnTablet(marginLayoutParams);
        // }
    }

    public void onSwitchToForeground() {
        if (mMostVisitedTilesCoordinator != null) {
            mMostVisitedTilesCoordinator.onSwitchToForeground();
        }
    }

    /**
     * Should be called every time one of the flags used to track initialization progress changes.
     * Finalizes initialization once all the preliminary steps are complete.
     *
     * @see #mHasShownView
     * @see #mTilesLoaded
     */
    private void onInitializationProgressChanged() {
        if (!hasLoadCompleted()) return;

        mManager.onLoadingComplete();

        // Load the logo after everything else is finished, since it's lower priority.
        mLogoCoordinator.loadSearchProviderLogoWithAnimation();
    }

    /**
     * To be called to notify that the tiles have finished loading. Will do nothing if a load was
     * previously completed.
     */
    public void onTilesLoaded() {
        if (mTilesLoaded) return;
        mTilesLoaded = true;

        onInitializationProgressChanged();
    }

    /**
     * Changes the layout depending on whether the selected search provider (e.g. Google, Bing)
     * has a logo.
     * @param hasLogo Whether the search provider has a logo.
     * @param isGoogle Whether the search provider is Google.
     */
    public void setSearchProviderInfo(boolean hasLogo, boolean isGoogle) {
        if (hasLogo == mSearchProviderHasLogo
                && isGoogle == mSearchProviderIsGoogle
                && mInitialized) {
            return;
        }
        mSearchProviderHasLogo = hasLogo;
        mSearchProviderIsGoogle = isGoogle;

        updateTilesLayoutMargins();

        // Hide or show the views above the tile grid as needed, including search box, and
        // spacers. The visibility of Logo is handled by LogoCoordinator.
        mSearchBoxCoordinator.setVisibility(mSearchProviderHasLogo);

        onUrlFocusAnimationChanged();

        mSnapshotTileGridChanged = true;
    }

    /** Updates the margins for the tile grid based on what is shown above it. */
    private void updateTilesLayoutMargins() {
        // MarginLayoutParams marginLayoutParams =
        //         (MarginLayoutParams) mMvTilesContainerLayout.getLayoutParams();
        //
        // if (mIsSurfacePolishEnabled) {
        //     if (mIsNtpAsHomeSurfaceOnTablet) {
        //         if (isScrollableMvtEnabled()) {
        //             marginLayoutParams.topMargin =
        //                     getResources()
        //                             .getDimensionPixelSize(
        //                                     shouldShowLogo()
        //                                             ? R.dimen.mvt_container_top_margin_polish
        //                                             : R.dimen.tile_grid_layout_no_logo_top_margin);
        //         } else {
        //             // Set a bit more top padding on the tile grid if there is no logo.
        //             ViewGroup.LayoutParams layoutParams = mMvTilesContainerLayout.getLayoutParams();
        //             layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        //             marginLayoutParams.topMargin = getGridMvtTopMargin();
        //         }
        //     }
        //     return;
        // }
        //
        // if (isScrollableMvtEnabled()) {
        //     // Let mMvTilesContainerLayout attached to the edge of the screen.
        //     // setClipToPadding(false);
        //     // if (mIsNtpAsHomeSurfaceOnTablet) {
        //     //     updateTilesLayoutLeftAndRightMarginsOnTablet(marginLayoutParams);
        //     // } else {
        //     //     int lateralPaddingsForNtp =
        //     //             -getResources()
        //     //                     .getDimensionPixelSize(R.dimen.ntp_header_lateral_paddings_v2);
        //     //     marginLayoutParams.leftMargin = lateralPaddingsForNtp;
        //     //     marginLayoutParams.rightMargin = lateralPaddingsForNtp;
        //     // }
        //     // marginLayoutParams.topMargin =
        //     //         getResources()
        //     //                 .getDimensionPixelSize(
        //     //                         shouldShowLogo()
        //     //                                 ? R.dimen.tile_grid_layout_top_margin
        //     //                                 : R.dimen.tile_grid_layout_no_logo_top_margin);
        //     // marginLayoutParams.bottomMargin =
        //     //         getResources()
        //     //                 .getDimensionPixelSize(R.dimen.tile_carousel_layout_bottom_margin);
        // } else {
        //     // Set a bit more top padding on the tile grid if there is no logo.
        //     ViewGroup.LayoutParams layoutParams = mMvTilesContainerLayout.getLayoutParams();
        //     layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        //     marginLayoutParams.topMargin = getGridMvtTopMargin();
        //     marginLayoutParams.bottomMargin = getGridMvtBottomMargin();
        // }
        //
        // if (mIsNtpAsHomeSurfaceOnTablet) {
        //     marginLayoutParams.bottomMargin =
        //             getResources()
        //                     .getDimensionPixelSize(R.dimen.mvt_container_bottom_margin_tablet);
        // }
    }

    /**
     * Updates whether the NewTabPage should animate on URL focus changes.
     * @param disable Whether to disable the animations.
     */
    void setUrlFocusAnimationsDisabled(boolean disable) {
        if (disable == mDisableUrlFocusChangeAnimations) return;
        mDisableUrlFocusChangeAnimations = disable;
        if (!disable) onUrlFocusAnimationChanged();
    }

    /**
     * @return Whether URL focus animations are currently disabled.
     */
    boolean urlFocusAnimationsDisabled() {
        return mDisableUrlFocusChangeAnimations;
    }

    /**
     * Specifies the percentage the URL is focused during an animation.  1.0 specifies that the URL
     * bar has focus and has completed the focus animation.  0 is when the URL bar is does not have
     * any focus.
     *
     * @param percent The percentage of the URL bar focus animation.
     */
    void setUrlFocusChangeAnimationPercent(float percent) {
        mUrlFocusChangePercent = percent;
        onUrlFocusAnimationChanged();
    }

    /**
     * @return The percentage that the URL bar is focused during an animation.
     */
    @VisibleForTesting
    float getUrlFocusChangeAnimationPercent() {
        return mUrlFocusChangePercent;
    }

    void onUrlFocusAnimationChanged() {
        if (mDisableUrlFocusChangeAnimations || mIsViewMoving) return;

        // Translate so that the search box is at the top, but only upwards.
        float percent = mSearchProviderHasLogo ? mUrlFocusChangePercent : 0;
        int basePosition = mScrollDelegate.getVerticalScrollOffset() + getPaddingTop();
        int target =
                Math.max(
                        basePosition,
                        getSearchBoxView().getBottom()
                                - getSearchBoxView().getPaddingBottom()
                                - mSearchBoxBoundsVerticalInset);

        setTranslationY(percent * (basePosition - target));
        if (mQueryTileSection != null) mQueryTileSection.onUrlFocusAnimationChanged(percent);
    }

    void onLoadUrl(boolean isNtpUrl) {
        if (isNtpUrl && mQueryTileSection != null) mQueryTileSection.reloadTiles();
    }

    /**
     * Sets whether this view is currently moving within its parent view. When the view is moving
     * certain animations will be disabled or prevented.
     * @param isViewMoving Whether this view is currently moving.
     */
    void setIsViewMoving(boolean isViewMoving) {
        mIsViewMoving = isViewMoving;
    }

    /**
     * Updates the opacity of the search box when scrolling.
     *
     * @param alpha opacity (alpha) value to use.
     */
    public void setSearchBoxAlpha(float alpha) {
        mSearchBoxCoordinator.setAlpha(alpha);
    }

    /**
     * Updates the opacity of the search provider logo when scrolling.
     *
     * @param alpha opacity (alpha) value to use.
     */
    public void setSearchProviderLogoAlpha(float alpha) {
        mLogoCoordinator.setAlpha(alpha);
    }

    /**
     * Set the search box background drawable.
     *
     * @param drawable The search box background.
     */
    public void setSearchBoxBackground(Drawable drawable) {
        mSearchBoxCoordinator.setBackground(drawable);
    }

    /**
     * Get the bounds of the search box in relation to the top level {@code parentView}.
     *
     * @param bounds The current drawing location of the search box.
     * @param translation The translation applied to the search box by the parent view hierarchy up
     *                    to the {@code parentView}.
     * @param parentView The top level parent view used to translate search box bounds.
     */
    void getSearchBoxBounds(Rect bounds, Point translation, View parentView) {
        int searchBoxX = (int) getSearchBoxView().getX();
        int searchBoxY = (int) getSearchBoxView().getY();
        bounds.set(
                searchBoxX,
                searchBoxY,
                searchBoxX + getSearchBoxView().getWidth(),
                searchBoxY + getSearchBoxView().getHeight());

        translation.set(0, 0);

        if (isSearchBoxOffscreen()) {
            translation.y = Integer.MIN_VALUE;
        } else {
            View view = getSearchBoxView();
            while (true) {
                view = (View) view.getParent();
                if (view == null) {
                    // The |mSearchBoxView| is not a child of this view. This can happen if the
                    // RecyclerView detaches the NewTabPageLayout after it has been scrolled out of
                    // view. Set the translation to the minimum Y value as an approximation.
                    translation.y = Integer.MIN_VALUE;
                    break;
                }
                translation.offset(-view.getScrollX(), -view.getScrollY());
                if (view == parentView) break;
                translation.offset((int) view.getX(), (int) view.getY());
            }
        }

        bounds.offset(translation.x, translation.y);
        if (translation.y != Integer.MIN_VALUE) {
            bounds.inset(0, mSearchBoxBoundsVerticalInset);
        }
    }

    void setSearchProviderTopMargin(int topMargin) {
        mLogoCoordinator.setTopMargin(topMargin);
    }

    void setSearchProviderBottomMargin(int bottomMargin) {
        mLogoCoordinator.setBottomMargin(bottomMargin);
    }

    /**
     * @return Whether the search box view is scrolled off the screen.
     */
    private boolean isSearchBoxOffscreen() {
        return !mScrollDelegate.isChildVisibleAtPosition(0)
                || mScrollDelegate.getVerticalScrollOffset()
                        > getSearchBoxView().getTop() + mTransitionLengthOffset;
    }

    /**
     * Sets the listener for search box scroll changes.
     * @param listener The listener to be notified on changes.
     */
    void setSearchBoxScrollListener(OnSearchBoxScrollListener listener) {
        mSearchBoxScrollListener = listener;
        if (mSearchBoxScrollListener != null) updateSearchBoxOnScroll();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        assert mManager != null;

        if (!mHasShownView) {
            mHasShownView = true;
            onInitializationProgressChanged();
            TraceEvent.instant("NewTabPageSearchAvailable)");
        }
    }

    /** Update the visibility of the action buttons. */
    void updateActionButtonVisibility() {
        mSearchBoxCoordinator.setVoiceSearchButtonVisibility(mManager.isVoiceSearchEnabled());
        boolean shouldShowLensButton =
                mSearchBoxCoordinator.isLensEnabled(LensEntryPoint.NEW_TAB_PAGE);
        LensMetrics.recordShown(LensEntryPoint.NEW_TAB_PAGE, shouldShowLensButton);
        mSearchBoxCoordinator.setLensButtonVisibility(shouldShowLensButton);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);

        if (visibility == VISIBLE) {
            updateActionButtonVisibility();
        }
    }

    /**
     * @see InvalidationAwareThumbnailProvider#shouldCaptureThumbnail()
     */
    public boolean shouldCaptureThumbnail() {
        return mSnapshotTileGridChanged;
    }

    /**
     * Should be called before a thumbnail of the parent view is captured.
     * @see InvalidationAwareThumbnailProvider#captureThumbnail(Canvas)
     */
    public void onPreCaptureThumbnail() {
        mLogoCoordinator.endFadeAnimation();
        mSnapshotTileGridChanged = false;
    }

    private boolean shouldShowLogo() {
        return mSearchProviderHasLogo;
    }

    private boolean hasLoadCompleted() {
        return mHasShownView && mTilesLoaded;
    }

    @Override
    public void onTakeoverReceived(String json) {
        try {
            if (bgController == null) bgController = new BackgroundController();

            LinearLayout mEarnedLinearLayout = findViewById(R.id.earned_linearlayout);
            LinearLayout mSavedLinearLayout = findViewById(R.id.savings_linearlayout);
            LinearLayout mComingSoonLinearLayout = findViewById(R.id.coming_soon_linearlayout);
            LinearLayout mDappsLinearLayout = findViewById(R.id.featured_dapps_linearlayout);

            int backgroundDrawable = R.drawable.ntp_rounded_background_translucent;
            if (!isDarkMode) {
                backgroundDrawable = R.drawable.ntp_rounded_dark_background_translucent;
            }

            mEarnedLinearLayout.setBackground(getResources().getDrawable(backgroundDrawable));
            mSavedLinearLayout.setBackground(getResources().getDrawable(backgroundDrawable));
            mComingSoonLinearLayout.setBackground(getResources().getDrawable(backgroundDrawable));
            mDappsLinearLayout.setBackground(getResources().getDrawable(backgroundDrawable));

            DisplayMetrics displayMetrics = mContext.getResources().getDisplayMetrics();
            int screenHeight = displayMetrics.heightPixels;
            int screenWidth = displayMetrics.widthPixels;

            RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(screenWidth, screenHeight);
            bgImageView.setLayoutParams(lp);

            JSONObject jsonObject = new JSONObject(json);
            bgController.setBackground(bgImageView, jsonObject.getString("background_url"));

            View fade = findViewById(R.id.bg_image_fade);
            fade.setVisibility(View.VISIBLE);
            fade.setBackground(isDarkMode ? getResources().getDrawable(R.drawable.fade_bottom_gradient_dark) : getResources().getDrawable(R.drawable.fade_bottom_gradient_light));

            ImageView ctaView = new ImageView(mContext); // Make sure to pass a valid Context here
            ctaView.setId(R.id.cta_view);
            // Setting the layout parameters to match parent width, wrap content height
            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, // Width
                    LinearLayout.LayoutParams.WRAP_CONTENT  // Height
            );
            ctaView.setLayoutParams(layoutParams);

            // Setting scale type to center
            ctaView.setScaleType(ImageView.ScaleType.CENTER);

            // Add ctaView to your layout if not already added
            // Assuming you have a layout named 'parentLayout' where you want to add ctaView
            mMainLayout.addView(ctaView, 2);

            Glide.with(ctaView)
                .load(jsonObject.getString("cta_image"))
                .thumbnail(0.1f)
                .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                .into(new CustomTarget<Drawable>() {
                    @Override
                    public void onResourceReady(@NonNull Drawable resource, @Nullable Transition<? super Drawable> transition) {
                        ctaView.setImageDrawable(resource);
                    }

                    @Override
                    public void onLoadCleared(@Nullable Drawable placeholder) {

                    }
                });

            ctaView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                   try {
                       loadUrl(jsonObject.getString("cta_url"));
                   } catch (Exception e) {}
                }
            });
        } catch (Exception e) { }
    }

    @Override
    public void onDappReceived(String json) {
        String textColor = isDarkMode ? "#ffffff" : "#000000";
        final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
        String userGeo = mPrefs.getString("u_geo", "");

        try {
           JSONArray mDappsArray = new JSONArray(json);

           JSONObject dapp1Obj = mDappsArray.getJSONObject(0);
           JSONObject dapp2Obj = mDappsArray.getJSONObject(1);
           JSONObject dapp3Obj = mDappsArray.getJSONObject(2);
           JSONObject dapp4Obj = mDappsArray.getJSONObject(3);
           JSONObject dapp5Obj = mDappsArray.getJSONObject(4);
           JSONObject dapp6Obj = mDappsArray.getJSONObject(5);
           JSONObject dapp7Obj = mDappsArray.getJSONObject(6);
           JSONObject dapp8Obj = mDappsArray.getJSONObject(7);
           if (dapp1Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp1Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp1Obj = dapp1Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp2Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp2Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp2Obj = dapp2Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp3Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp3Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp3Obj = dapp3Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp4Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp4Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp4Obj = dapp4Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp5Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp5Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp5Obj = dapp5Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp6Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp6Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp6Obj = dapp6Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp7Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp7Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp7Obj = dapp7Obj.getJSONObject("geoTargeting");
             }
           }
           if (dapp8Obj.getJSONObject("geoTargeting").getBoolean("enabled")) {
             if (jsonArrayContainString(userGeo, dapp8Obj.getJSONObject("geoTargeting").getJSONArray("geos"))) {
               dapp8Obj = dapp8Obj.getJSONObject("geoTargeting");
             }
           }

           // DApps Links
           View featuredDappTile1 = findViewById(R.id.featured_daps1);
           View featuredDappTile2 = findViewById(R.id.featured_daps2);
           View featuredDappTile3 = findViewById(R.id.featured_daps3);
           View featuredDappTile4 = findViewById(R.id.featured_daps4);
           View featuredDappTile5 = findViewById(R.id.featured_daps5);
           View featuredDappTile6 = findViewById(R.id.featured_daps6);
           View featuredDappTile7 = findViewById(R.id.featured_daps7);
           View featuredDappTile8 = findViewById(R.id.featured_daps8);

           TextView mTextView1 = featuredDappTile1.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView2 = featuredDappTile2.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView3 = featuredDappTile3.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView4 = featuredDappTile4.findViewById(R.id.speed_dial_tile_textview);
           mTextView1.setText(dapp1Obj.getString("title"));
           mTextView2.setText(dapp2Obj.getString("title"));
           mTextView3.setText(dapp3Obj.getString("title"));
           mTextView4.setText(dapp4Obj.getString("title"));
           mTextView1.setTextColor(Color.parseColor(textColor));
           mTextView2.setTextColor(Color.parseColor(textColor));
           mTextView3.setTextColor(Color.parseColor(textColor));
           mTextView4.setTextColor(Color.parseColor(textColor));

           ((FrameLayout) featuredDappTile1.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile2.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile3.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile4.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);

           ((ImageView) featuredDappTile1.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile2.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile3.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile4.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile5.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile6.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile7.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);
           ((ImageView) featuredDappTile8.findViewById(R.id.speed_dial_tile_view_icon)).setImageTintList(null);

           remoteDappsHelper.setBackground(((ImageView) featuredDappTile1.findViewById(R.id.speed_dial_tile_view_icon)), dapp1Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile2.findViewById(R.id.speed_dial_tile_view_icon)), dapp2Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile3.findViewById(R.id.speed_dial_tile_view_icon)), dapp3Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile4.findViewById(R.id.speed_dial_tile_view_icon)), dapp4Obj.getString("imgUrl"));

           TextView mTextView5 = featuredDappTile5.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView6 = featuredDappTile6.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView7 = featuredDappTile7.findViewById(R.id.speed_dial_tile_textview);
           TextView mTextView8 = featuredDappTile8.findViewById(R.id.speed_dial_tile_textview);
           mTextView5.setText(dapp5Obj.getString("title"));
           mTextView6.setText(dapp6Obj.getString("title"));
           mTextView7.setText(dapp7Obj.getString("title"));
           mTextView8.setText(dapp8Obj.getString("title"));
           mTextView5.setTextColor(Color.parseColor(textColor));
           mTextView6.setTextColor(Color.parseColor(textColor));
           mTextView7.setTextColor(Color.parseColor(textColor));
           mTextView8.setTextColor(Color.parseColor(textColor));

           ((FrameLayout) featuredDappTile5.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile6.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile7.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);
           ((FrameLayout) featuredDappTile8.findViewById(R.id.speed_dial_tile_view_icon_background)).setBackground(null);

           remoteDappsHelper.setBackground(((ImageView) featuredDappTile5.findViewById(R.id.speed_dial_tile_view_icon)), dapp5Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile6.findViewById(R.id.speed_dial_tile_view_icon)), dapp6Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile7.findViewById(R.id.speed_dial_tile_view_icon)), dapp7Obj.getString("imgUrl"));
           remoteDappsHelper.setBackground(((ImageView) featuredDappTile8.findViewById(R.id.speed_dial_tile_view_icon)), dapp8Obj.getString("imgUrl"));

           final String dapp1Url = dapp1Obj.getString("url");
           final String dapp2Url = dapp2Obj.getString("url");
           final String dapp3Url = dapp3Obj.getString("url");
           final String dapp4Url = dapp4Obj.getString("url");
           final String dapp5Url = dapp5Obj.getString("url");
           final String dapp6Url = dapp6Obj.getString("url");
           final String dapp7Url = dapp7Obj.getString("url");
           final String dapp8Url = dapp8Obj.getString("url");

           featuredDappTile1.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                  try {
                      loadUrl(dapp1Url);
                  } catch (Exception e) {}
               }
           });

           featuredDappTile2.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp2Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile3.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp3Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile4.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp4Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile5.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp5Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile6.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp6Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile7.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp7Url);
                 } catch (Exception e) {}
               }
           });

           featuredDappTile8.setOnClickListener(new View.OnClickListener() {
               @Override
               public void onClick(View view) {
                 try {
                     loadUrl(dapp8Url);
                 } catch (Exception e) {}
               }
           });
        } catch (Exception e) {
        }

        View mViewMoreDappsBtn = findViewById(R.id.view_more_dapps_btn);
        mViewMoreDappsBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                loadUrl("https://carbon.website/app-store/");
            }
        });
    }

    private boolean jsonArrayContainString(String geo, JSONArray arr) {
        try {
          boolean found = false;
          // Iterate through the JSONArray to find the string
          for (int i = 0; i < arr.length(); i++) {
              if (arr.getString(i).equals(geo)) {
                  found = true;
                  break; // Exit the loop as we found the string
              }
          }

          return found;
        } catch (Exception e) {
          return false;
        }
    }

    @Override
    public void onBackgroundReceived(String credit, String creditUrl, String imgUrl) {
        if (bgController == null) bgController = new BackgroundController();
        try {
            bgController.setBackground(bgImageView, imgUrl);
            // set text color to white so it shows on the background
            // mSpeedDialView.updateTileTextTint();
            // mMostVisitedTitle.setTextColor(Color.WHITE);
            // if (mSiteSectionViewHolder instanceof SiteSectionViewHolder)
            // ((TileGridViewHolder)mSiteSectionViewHolder).updateTileTextTint();
            // set the rest of the stuff
            mPhotoCredit.setText("Photo by "+credit);
            mPhotoCredit.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    loadUrl(creditUrl);
                }
            });
        } catch (Exception ignore) { }
    }

    private void maybeKickOffCryptidRendering() {
        if (!mSearchProviderIsGoogle || mShowingNonStandardLogo) {
            // Cryptid rendering is disabled when the logo is not the standard Google logo.
            return;
        }

        ProbabilisticCryptidRenderer renderer = ProbabilisticCryptidRenderer.getInstance();
        renderer.getCryptidForLogo(
                Profile.getLastUsedRegularProfile(),
                mCallbackController.makeCancelable(
                        (drawable) -> {
                            if (drawable == null || mCryptidHolder != null) {
                                return;
                            }
                            ViewStub stub =
                                    findViewById(R.id.logo_holder)
                                            .findViewById(R.id.cryptid_holder);
                            ImageView view = (ImageView) stub.inflate();
                            view.setImageDrawable(drawable);
                            mCryptidHolder = view;
                            renderer.recordRenderEvent();
                        }));
    }

    private void onDestroy() {
        if (mCallbackController != null) {
            mCallbackController.destroy();
            mCallbackController = null;
        }

        if (mLogoCoordinator != null) {
            mLogoCoordinator.destroy();
            mLogoCoordinator = null;
        }

        mSearchBoxCoordinator.destroy();

        if (mMostVisitedTilesCoordinator != null) {
            mMostVisitedTilesCoordinator.destroyMvtiles();
            mMostVisitedTilesCoordinator = null;
        }

        if (mIsTablet) {
            mUiConfig.removeObserver(mDisplayStyleObserver);
            mDisplayStyleObserver = null;
        }

        if (mTokenDialog != null) {
            mTokenDialog = null;
        }

        if (bgController != null) {
            bgController = null;
        }

        if (remoteDappsHelper != null) {
          remoteDappsHelper = null;
        }

        if (bgImageView != null) {
            bgImageView = null;
        }

        if (mMainLayout != null) {
            mMainLayout = null;
        }
    }

    MostVisitedTilesCoordinator getMostVisitedTilesCoordinatorForTesting() {
        return mMostVisitedTilesCoordinator;
    }

    void maybeShowFeatureNotificationVoiceSearchIPH() {
        IPHCommandBuilder iphCommandBuilder =
                createIPHCommandBuilder(
                        mActivity.getResources(),
                        R.string.feature_notification_guide_tooltip_message_voice_search,
                        R.string.feature_notification_guide_tooltip_message_voice_search,
                        mSearchBoxCoordinator.getVoiceSearchButton(),
                        true);
        UserEducationHelper userEducationHelper = new UserEducationHelper(mActivity, new Handler());
        userEducationHelper.requestShowIPH(iphCommandBuilder.build());
    }

    private static IPHCommandBuilder createIPHCommandBuilder(
            Resources resources,
            @StringRes int stringId,
            @StringRes int accessibilityStringId,
            View anchorView,
            boolean showHighlight) {
        IPHCommandBuilder iphCommandBuilder =
                new IPHCommandBuilder(
                        resources,
                        FeatureConstants
                                .FEATURE_NOTIFICATION_GUIDE_VOICE_SEARCH_HELP_BUBBLE_FEATURE,
                        stringId,
                        accessibilityStringId);
        iphCommandBuilder.setAnchorView(anchorView);
        int yInsetPx = resources.getDimensionPixelOffset(R.dimen.ntp_iph_searchbox_y_inset);
        iphCommandBuilder.setInsetRect(new Rect(0, 0, 0, -yInsetPx));
        if (showHighlight) {
            iphCommandBuilder.setOnShowCallback(
                    () ->
                            ViewHighlighter.turnOnHighlight(
                                    anchorView, new HighlightParams(HighlightShape.CIRCLE)));
            iphCommandBuilder.setOnDismissCallback(
                    () ->
                            new Handler()
                                    .postDelayed(
                                            () -> {
                                                ViewHighlighter.turnOffHighlight(anchorView);
                                            },
                                            ViewHighlighter.IPH_MIN_DELAY_BETWEEN_TWO_HIGHLIGHTS));
        }

        return iphCommandBuilder;
    }

    /** Makes the Search Box and Logo as wide as Most Visited. */
    private void unifyElementWidths() {
        View searchBoxView = getSearchBoxView();
        // if (mMvTilesContainerLayout.getVisibility() != GONE) {
        //     final int width =
        //             getMeasuredWidth() - (mIsSurfacePolishEnabled ? 0 : mTileGridLayoutBleed);
        //     if (!isScrollableMvtEnabled()) {
        //         measureExactly(
        //                 searchBoxView,
        //                 width - mSearchBoxTwoSideMargin,
        //                 searchBoxView.getMeasuredHeight());
        //         mLogoCoordinator.measureExactlyLogoView(width);
        //     } else {
        //         // We reset the extra margins of the fake search box if the scrollable MV tiles are
        //         // showing in the portrait mode with multiple column Feeds.
        //         int searchBoxTwoSideMargin = mSearchBoxTwoSideMargin;
        //         if (mSearchBoxTwoSideMargin != 0
        //                 && getResources().getConfiguration().orientation
        //                         == Configuration.ORIENTATION_PORTRAIT
        //                 && !mIsSurfacePolishEnabled) {
        //             searchBoxTwoSideMargin = 0;
        //         }
        //
        //         measureExactly(
        //                 searchBoxView,
        //                 width - searchBoxTwoSideMargin,
        //                 searchBoxView.getMeasuredHeight());
        //         mLogoCoordinator.measureExactlyLogoView(width);
        //     }
        // }
    }

    private boolean isScrollableMvtEnabled() {
        return NewTabPage.isScrollableMvtEnabled(mContext);
    }

    // TODO(crbug.com/1329288): Remove this method when the Feed position experiment is cleaned up.
    private int getGridMvtTopMargin() {
        if (!shouldShowLogo()) {
            return getResources()
                    .getDimensionPixelSize(R.dimen.tile_grid_layout_no_logo_top_margin);
        }

        int resourcesId =
                mIsSurfacePolishEnabled
                        ? R.dimen.mvt_container_top_margin_polish
                        : R.dimen.tile_grid_layout_top_margin;

        if (FeedPositionUtils.isFeedPushDownLargeEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_top_margin_push_down_large;
        } else if (FeedPositionUtils.isFeedPushDownSmallEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_top_margin_push_down_small;
        } else if (FeedPositionUtils.isFeedPullUpEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_top_margin_pull_up;
        }

        return getResources().getDimensionPixelSize(resourcesId);
    }

    // TODO(crbug.com/1329288): Remove this method when the Feed position experiment is cleaned up.
    private int getGridMvtBottomMargin() {
        int resourcesId = R.dimen.tile_grid_layout_bottom_margin;

        if (!shouldShowLogo()) return getResources().getDimensionPixelSize(resourcesId);

        if (FeedPositionUtils.isFeedPushDownLargeEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_bottom_margin_push_down_large;
        } else if (FeedPositionUtils.isFeedPushDownSmallEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_bottom_margin_push_down_small;
        } else if (FeedPositionUtils.isFeedPullUpEnabled()) {
            resourcesId = R.dimen.tile_grid_layout_bottom_margin_pull_up;
        }

        return getResources().getDimensionPixelSize(resourcesId);
    }

    /**
     * Convenience method to call measure() on the given View with MeasureSpecs converted from the
     * given dimensions (in pixels) with MeasureSpec.EXACTLY.
     */
    private static void measureExactly(View view, int widthPx, int heightPx) {
        view.measure(
                MeasureSpec.makeMeasureSpec(widthPx, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(heightPx, MeasureSpec.EXACTLY));
    }

    LogoCoordinator getLogoCoordinatorForTesting() {
        return mLogoCoordinator;
    }

    private void onDisplayStyleChanged(UiConfig.DisplayStyle newDisplayStyle) {
        if (!mIsTablet) return;

        if (mIsSurfacePolishEnabled) {
            updateMvtOnTabletForPolish();
            updateSearchBoxWidthForPolish();
        } else if (mIsNtpAsHomeSurfaceOnTablet && isScrollableMvtEnabled()) {
            // MarginLayoutParams marginLayoutParams =
            //         (MarginLayoutParams) mMvTilesContainerLayout.getLayoutParams();
            // updateTilesLayoutLeftAndRightMarginsOnTablet(marginLayoutParams);
        }
    }

    /**
     * Updates the margins for the MV tiles container when used in NTP on the tablet.
     * @param marginLayoutParams The {@link MarginLayoutParams} of the MV tiles container.
     */
    private void updateTilesLayoutLeftAndRightMarginsOnTablet(
            MarginLayoutParams marginLayoutParams) {
        ((LayoutParams) marginLayoutParams).gravity = Gravity.CENTER_HORIZONTAL;
        int leftMarginForNtp = mTileGridLayoutBleed / 2;
        int rightMarginForNtp = leftMarginForNtp;
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            leftMarginForNtp = leftMarginForNtp + mMvtLandscapeLateralMarginTablet;
            rightMarginForNtp = rightMarginForNtp + mMvtLandscapeLateralMarginTablet;
            if (mIsHalfMvtLandscape != null && mIsHalfMvtLandscape) {
                ((LayoutParams) marginLayoutParams).gravity = Gravity.START;
                rightMarginForNtp = rightMarginForNtp + mMvtExtraRightMarginTablet;
            }
        } else if (mIsHalfMvtPortrait != null && mIsHalfMvtPortrait) {
            ((LayoutParams) marginLayoutParams).gravity = Gravity.START;
            rightMarginForNtp = rightMarginForNtp + mMvtExtraRightMarginTablet;
        }
        marginLayoutParams.leftMargin = leftMarginForNtp;
        marginLayoutParams.rightMargin = rightMarginForNtp;
    }

    /**
     * Updates whether the MV tiles layout stays in the center of the container when used in NTP on
     * the tablet by changing the width of its container. Also updates the lateral margins.
     */
    private void updateMvtOnTabletForPolish() {
        // MarginLayoutParams marginLayoutParams =
        //         (MarginLayoutParams) mMvTilesContainerLayout.getLayoutParams();
        // marginLayoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT;
        // if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
        //     if (mIsMvtAllFilledLandscape != null && mIsMvtAllFilledLandscape) {
        //         marginLayoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        //     }
        // } else if (mIsMvtAllFilledPortrait != null && mIsMvtAllFilledPortrait) {
        //     marginLayoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        // }
        //
        // int lateralPaddingId =
        //         isInNarrowWindowOnTablet(mIsTablet, mUiConfig)
        //                 ? R.dimen.search_box_lateral_margin_polish
        //                 : R.dimen.mvt_container_lateral_margin_polish;
        // int lateralPaddingsForNtp = getResources().getDimensionPixelSize(lateralPaddingId);
        // marginLayoutParams.leftMargin = lateralPaddingsForNtp;
        // marginLayoutParams.rightMargin = lateralPaddingsForNtp;
    }

    private void updateSearchBoxWidthForPolish() {
        if (isInNarrowWindowOnTablet(mIsTablet, mUiConfig)) {
            mSearchBoxTwoSideMargin =
                    getResources().getDimensionPixelSize(R.dimen.search_box_lateral_margin_polish)
                            * 2;
            // Invalidates |mIsHalfMvtLandscape| or |mIsHalfMvtPortrait| to recalculate them in
            // onMeasure().
            if (getResources().getConfiguration().orientation
                    == Configuration.ORIENTATION_LANDSCAPE) {
                mIsHalfMvtLandscape = null;
            } else {
                mIsHalfMvtPortrait = null;
            }
        } else if (mIsNtpAsHomeSurfaceOnTablet) {
            mSearchBoxTwoSideMargin =
                    getResources()
                                    .getDimensionPixelSize(
                                            R.dimen.ntp_search_box_lateral_margin_tablet_polish)
                            * 2;
        } else {
            mSearchBoxTwoSideMargin =
                    getResources()
                                    .getDimensionPixelSize(
                                            R.dimen.mvt_container_lateral_margin_polish)
                            * 2;
        }
    }

    /** Returns whether the current window is a narrow one on tablet. */
    @VisibleForTesting
    public static boolean isInNarrowWindowOnTablet(boolean isTablet, UiConfig uiConfig) {
        return isTablet
                && uiConfig.getCurrentDisplayStyle().horizontal < HorizontalDisplayStyle.WIDE;
    }
}

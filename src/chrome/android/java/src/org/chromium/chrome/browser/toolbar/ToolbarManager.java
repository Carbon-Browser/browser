// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.ComponentCallbacks;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.View.OnClickListener;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewGroup;
import android.view.ViewStub;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.appbar.AppBarLayout;

import org.chromium.base.Callback;
import org.chromium.base.CallbackController;
import org.chromium.base.TraceEvent;
import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.app.tab_activity_glue.TabReparentingController;
import org.chromium.chrome.browser.app.tabmodel.TabWindowManagerSingleton;
import org.chromium.chrome.browser.autofill_assistant.AutofillAssistantPreferenceFragment;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.Invalidator;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanelManager.OverlayPanelManagerObserver;
import org.chromium.chrome.browser.compositor.bottombar.ephemeraltab.EphemeralTabCoordinator;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.crash.ChromePureJavaExceptionReporter;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbar;
import org.chromium.chrome.browser.dom_distiller.DomDistillerTabUtils;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.findinpage.FindToolbarObserver;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.fullscreen.FullscreenOptions;
import org.chromium.chrome.browser.history.HistoryManagerUtils;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.homepage.HomepagePolicyManager;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.merchant_viewer.MerchantTrustSignalsCoordinator;
import org.chromium.chrome.browser.ntp.IncognitoNewTabPage;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.omaha.UpdateMenuItemHelper;
import org.chromium.chrome.browser.omnibox.BackKeyBehaviorDelegate;
import org.chromium.chrome.browser.omnibox.LocationBar;
import org.chromium.chrome.browser.omnibox.LocationBarCoordinator;
import org.chromium.chrome.browser.omnibox.NewTabPageDelegate;
import org.chromium.chrome.browser.omnibox.OmniboxFocusReason;
import org.chromium.chrome.browser.omnibox.OmniboxStub;
import org.chromium.chrome.browser.omnibox.OverrideUrlLoadingDelegate;
import org.chromium.chrome.browser.omnibox.SearchEngineLogoUtils;
import org.chromium.chrome.browser.omnibox.UrlFocusChangeListener;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxPedalDelegate;
import org.chromium.chrome.browser.omnibox.voice.VoiceRecognitionHandler;
import org.chromium.chrome.browser.page_info.ChromePageInfo;
import org.chromium.chrome.browser.partnercustomizations.PartnerBrowserCustomizations;
import org.chromium.chrome.browser.price_tracking.PriceTrackingFeatures;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.SadTab;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.tasks.ReturnToChromeUtil;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupUi;
import org.chromium.chrome.browser.tasks.tab_management.TabManagementModuleProvider;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.ScrollingBottomViewResourceFrameLayout;
import org.chromium.chrome.browser.toolbar.load_progress.LoadProgressCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonState;
import org.chromium.chrome.browser.toolbar.top.ActionModeController;
import org.chromium.chrome.browser.toolbar.top.ActionModeController.ActionBarDelegate;
import org.chromium.chrome.browser.toolbar.top.HomeButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.ToggleTabStackButton;
import org.chromium.chrome.browser.toolbar.top.ToggleTabStackButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.toolbar.top.ToolbarPhone;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet;
import org.chromium.chrome.browser.toolbar.top.TopToolbarCoordinator;
import org.chromium.chrome.browser.toolbar.top.TopToolbarInteractabilityManager;
import org.chromium.chrome.browser.toolbar.top.ViewShiftingActionBarDelegate;
import org.chromium.chrome.browser.ui.TabObscuringHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.MenuButtonDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePage;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.chrome.browser.user_education.IPHCommandBuilder;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.browser.util.ChromeAccessibilityUtil;
import org.chromium.chrome.browser.vr.VrModuleProvider;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.chrome.features.start_surface.StartSurfaceState;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.components.browser_ui.widget.gesture.BackPressHandler;
import org.chromium.components.browser_ui.widget.highlight.ViewHighlighter.HighlightParams;
import org.chromium.components.browser_ui.widget.highlight.ViewHighlighter.HighlightShape;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.page_info.PageInfoController.OpenedFromSource;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.components.search_engines.TemplateUrlService.TemplateUrlServiceObserver;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationEntry;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.net.NetError;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.WindowDelegate;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.util.TokenHolder;
import org.chromium.ui.vr.VrModeObserver;
import org.chromium.url.GURL;

import org.chromium.base.ApiCompatibilityUtils;
import android.view.WindowManager;
import android.os.Build;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.CompoundButton;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.rewards.RewardsAPIBridge;
import org.chromium.chrome.browser.rewards.RewardsCommunicator;
import org.chromium.ui.widget.ChromeImageButton;
import androidx.recyclerview.widget.LinearLayoutManager;
import android.widget.ImageView;
import com.bumptech.glide.request.target.DrawableImageViewTarget;
import com.bumptech.glide.Glide;
import android.net.Uri;
import java.net.URL;
import org.chromium.components.adblock.AdblockController;
import android.widget.Switch;
import androidx.appcompat.widget.AppCompatImageButton;
import java.io.File;
import android.content.ContentResolver;
import com.bumptech.glide.load.engine.DiskCacheStrategy;

import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarCoordinator;
// import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialController;
// import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialGridView;
// import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialInteraction;
import android.content.DialogInterface;
import android.view.Gravity;
import android.view.ViewGroup;
import android.app.AlertDialog;
import android.widget.FrameLayout;
import android.graphics.Color;

import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarThemeCommunicator;

import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;

import java.util.List;

import org.chromium.chrome.browser.rewards.RewardsBottomSheetCoordinator;

import android.content.res.Resources;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import org.chromium.chrome.browser.wallet.dapp.DAppMethod;
import org.json.JSONObject;
import java.util.Base64;
import java.nio.charset.StandardCharsets;
import android.content.Context;
import java.io.OutputStream;


import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;
import org.chromium.chrome.browser.pininput.PinCodeFragment;
import android.content.SharedPreferences;
import android.view.Window;
import android.widget.Button;
import org.chromium.ui.widget.Toast;
import android.os.Looper;
import wallet.core.jni.CoinType;
import org.chromium.chrome.browser.wallet.WalletDataObj;
import java.util.ArrayList;
import java.lang.Integer;
import java.math.BigInteger;
import com.google.protobuf.ByteString;

import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import java.math.BigInteger;
import java.math.BigDecimal;
import wallet.core.jni.CoinType;
import wallet.core.jni.HDWallet;
import wallet.core.jni.PrivateKey;
import wallet.core.jni.proto.Ethereum;
import wallet.core.java.AnySigner;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.SocketTimeoutException;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.wallet.TokenDatabase;

import java.security.NoSuchAlgorithmException;
import org.chromium.chrome.browser.wallet.CryptoAPIHelper;
import org.bouncycastle.jcajce.provider.digest.Keccak;

import 	android.widget.ProgressBar;
import android.content.ClipData;
import android.content.ClipboardManager;
import java.math.RoundingMode;

import android.os.VibrationEffect;
import android.os.Vibrator;

/**
 * Contains logic for managing the toolbar visual component.  This class manages the interactions
 * with the rest of the application to ensure the toolbar is always visually up to date.
 */
public class ToolbarManager implements UrlFocusChangeListener, ThemeColorObserver, TintObserver,
                                       MenuButtonDelegate, ChromeAccessibilityUtil.Observer, BottomToolbarCoordinator,
                                       RewardsCommunicator {
    private final IncognitoStateProvider mIncognitoStateProvider;
    private final TabCountProvider mTabCountProvider;
    private final TopUiThemeColorProvider mTopUiThemeColorProvider;
    private final Supplier<EphemeralTabCoordinator> mEphemeralTabCoordinatorSupplier;
    private AppThemeColorProvider mAppThemeColorProvider;
    private SettableThemeColorProvider mCustomTabThemeColorProvider;
    private final TopToolbarCoordinator mToolbar;
    private final ToolbarControlContainer mControlContainer;
    private final BrowserControlsStateProvider.Observer mBrowserControlsObserver;
    private final FullscreenManager.Observer mFullscreenObserver;
    private final ObservableSupplierImpl<Boolean> mHomepageEnabledSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<Boolean> mHomepageManagedByPolicySupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<Boolean> mStartSurfaceAsHomepageSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<Boolean> mIdentityDiscStateSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplier<Boolean> mOmniboxFocusStateSupplier;

    private BottomControlsCoordinator mBottomControlsCoordinator;

    private ObservableSupplierImpl<BottomControlsCoordinator> mBottomControlsCoordinatorSupplier =
            new ObservableSupplierImpl<>();
    private TabModelSelector mTabModelSelector;
    private TabModelSelectorObserver mTabModelSelectorObserver;
    private ObservableSupplier<TabModelSelector> mTabModelSelectorSupplier;
    private ActivityTabProvider.ActivityTabTabObserver mActivityTabTabObserver;
    private final ActivityTabProvider mActivityTabProvider;
    private final LocationBarModel mLocationBarModel;
    private ObservableSupplier<BookmarkBridge> mBookmarkBridgeSupplier;
    private final Callback<BookmarkBridge> mBookmarkBridgeSupplierObserver;
    private TemplateUrlServiceObserver mTemplateUrlObserver;
    private LocationBar mLocationBar;
    private FindToolbarManager mFindToolbarManager;

    private LayoutManagerImpl mLayoutManager;

    private TabObserver mTabObserver;
    private BookmarkBridge.BookmarkModelObserver mBookmarksObserver;
    private FindToolbarObserver mFindToolbarObserver;

    private @StartSurfaceState int mStartSurfaceState = StartSurfaceState.NOT_SHOWN;
    private boolean mIsStartSurfaceEnabled;

    private LayoutStateProvider mLayoutStateProvider;
    private LayoutStateProvider.LayoutStateObserver mLayoutStateObserver;
    private OneshotSupplier<LayoutStateProvider> mLayoutStateProviderSupplier;
    private CallbackController mCallbackController = new CallbackController();

    private final ActionBarDelegate mActionBarDelegate;
    private ActionModeController mActionModeController;
    private final Callback<Boolean> mUrlFocusChangedCallback;
    private final Handler mHandler = new Handler();
    private final AppCompatActivity mActivity;
    private final WindowAndroid mWindowAndroid;
    private final AppMenuDelegate mAppMenuDelegate;
    private final CompositorViewHolder mCompositorViewHolder;
    private final BrowserControlsSizer mBrowserControlsSizer;
    private final FullscreenManager mFullscreenManager;
    private LocationBarFocusScrimHandler mLocationBarFocusHandler;
    private ComponentCallbacks mComponentCallbacks;
    private final LoadProgressCoordinator mProgressBarCoordinator;
    private final ToolbarTabControllerImpl mToolbarTabController;
    private MenuButtonCoordinator mMenuButtonCoordinator;
    private MenuButtonCoordinator mOverviewModeMenuButtonCoordinator;
    private HomepageManager.HomepageStateListener mHomepageStateListener;
    private StatusBarColorController mStatusBarColorController;
    private final Supplier<ShareDelegate> mShareDelegateSupplier;
    private final ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    private final BottomSheetController mBottomSheetController;
    private final Supplier<Boolean> mIsWarmOnResumeSupplier;
    private final TabContentManager mTabContentManager;
    private final TabCreatorManager mTabCreatorManager;
    private final SnackbarManager mSnackbarManager;
    private final OneshotSupplier<TabReparentingController> mTabReparentingControllerSupplier;

    private RewardsBottomSheetCoordinator mRewardsBottomSheetCoordinator;

    private HomeButtonCoordinator mHomeButtonCoordinator;
    private ToggleTabStackButtonCoordinator mToggleTabStackButtonCoordinator;

    private BrowserStateBrowserControlsVisibilityDelegate mControlsVisibilityDelegate;
    private int mFullscreenFocusToken = TokenHolder.INVALID_TOKEN;
    private int mFullscreenFindInPageToken = TokenHolder.INVALID_TOKEN;
    private int mFullscreenMenuToken = TokenHolder.INVALID_TOKEN;
    private int mFullscreenHighlightToken = TokenHolder.INVALID_TOKEN;

    private boolean mTabRestoreCompleted;

    private ColorStateList mTempColorStateList;
    private @BrandedColorScheme int mTempBrandedColorScheme;

    private int mTempColor;

    private boolean mInitializedWithNative;
    private Runnable mOnInitializedRunnable;
    private Runnable mMenuStateObserver;
    private Runnable mStartSurfaceMenuStateObserver;

    private boolean mShouldUpdateToolbarPrimaryColor = true;
    private int mCurrentThemeColor;

    private int mCurrentOrientation;

    private final Supplier<Boolean> mCanAnimateNativeBrowserControls;

    /**
     * Runnable for the home and search accelerator button when Start Surface home page is enabled.
     */
    private Supplier<Boolean> mShowStartSurfaceSupplier;
    private final ScrimCoordinator mScrimCoordinator;

    private StartSurface mStartSurface;
    private StartSurface.StateObserver mStartSurfaceStateObserver;
    private AppBarLayout.OnOffsetChangedListener mStartSurfaceHeaderOffsetChangeListener;

    private OneshotSupplier<ToolbarIntentMetadata> mIntentMetadataOneshotSupplier;
    private OneshotSupplier<Boolean> mPromoShownOneshotSupplier;
    private OverlayPanelManagerObserver mOverlayPanelManagerObserver;
    private ObservableSupplierImpl<Boolean> mOverlayPanelVisibilitySupplier =
            new ObservableSupplierImpl<>();

    private TabGroupUi mTabGroupUi;

    private final VrModeObserver mVrModeObserver;
    private ObservableSupplierImpl<Boolean> mIsProgressBarVisibleSupplier =
            new ObservableSupplierImpl<>();

    private boolean mIsDestroyed;

    // private RewardsAPIBridge mRewardsBridge;

    private OnClickListener mTabSwitcherClickHandler;

    // private AlertDialog mSpeedDialDialog;

    private String authorizedHost = "";
    private ArrayList<WalletDataObj> walletAddresses = new ArrayList<>();
    private String inputPinCode = "";
    private View mDot1;
    private View mDot2;
    private View mDot3;
    private View mDot4;
    private View mDot5;
    private View mDot6;

    private WalletInteractionEnum pendingWalletInteractionType;
    private String pendingWalletInteractionNetwork;
    private long pendingWalletInteractionId;
    private JavaScriptCallback pendingWalletInteractionCallback;
    private String pendingWalletInteractionChainIdHex;
    private int pendingWalletInteractionChainId;

    private String pendingWalletInteractionData;
    private String pendingWalletInteractionFrom;
    private String pendingWalletInteractionGas;
    private String pendingWalletInteractionPrice;
    private String pendingWalletInteractionTo;
    private String pendingWalletInteractionValue;

    private boolean shouldResetPendingInteraction = true;

    private AlertDialog mPinDialog;
    private int overrideChainId = -1;

    private boolean isLoadingGas;

    private View transactionConfirmationContainer;

    private static class TabObscuringCallback implements Callback<Boolean> {
        private final TabObscuringHandler mTabObscuringHandler;
        /** A token held while the toolbar/omnibox is obscuring all visible tabs. */
        private int mTabObscuringToken = TokenHolder.INVALID_TOKEN;
        public TabObscuringCallback(TabObscuringHandler handler) {
            mTabObscuringHandler = handler;
        }

        @Override
        public void onResult(Boolean visible) {
            if (visible) {
                // It's possible for the scrim to unfocus and refocus without the
                // visibility actually changing. In this case we have to make sure we
                // unregister the previous token before acquiring a new one.
                int oldToken = mTabObscuringToken;
                mTabObscuringToken = mTabObscuringHandler.obscureAllTabs();
                if (oldToken != TokenHolder.INVALID_TOKEN) {
                    mTabObscuringHandler.unobscureAllTabs(oldToken);
                }
            } else {
                mTabObscuringHandler.unobscureAllTabs(mTabObscuringToken);
                mTabObscuringToken = TokenHolder.INVALID_TOKEN;
            }
        }
    };

    private enum WalletInteractionEnum {
       WALLET_UNLOCK,
       WALLET_SWITCH_CHAIN,
       WALLET_SEND_TRX,
    }

    /**
     * Creates a ToolbarManager object.
     *
     * @param activity The Android activity.
     * @param controlsSizer The {@link BrowserControlsSizer} for the activity.
     * @param fullscreenManager The {@link FullscreenManager} for the activity.
     * @param controlContainer The container of the toolbar.
     * @param compositorViewHolder Class that holds a {@link CompositorView}.
     * @param urlFocusChangedCallback The callback to be notified when the URL focus changes.
     * @param topUiThemeColorProvider The ThemeColorProvider object for top UI.
     * @param tabObscuringHandler Delegate object handling obscuring views.
     * @param shareDelegateSupplier Supplier for ShareDelegate.
     * @param identityDiscController The controller that coordinates the state of the identity disc
     * @param buttonDataProviders The list of button data providers for the optional toolbar button
     *         in the browsing mode toolbar, given in precedence order.
     * @param tabProvider The {@link ActivityTabProvider} for accessing current activity tab.
     * @param scrimCoordinator A means of showing the scrim.
     * @param toolbarActionModeCallback Callback that communicates changes in the conceptual mode
     *                                  of toolbar interaction.
     * @param findToolbarManager The manager for the find in page function.
     * @param profileSupplier Supplier of the currently applicable profile.
     * @param bookmarkBridgeSupplier Supplier of the bookmark bridge for the current profile.
     * TODO(https://crbug.com/1084528): Use OneShotSupplier once it is ready.
     * @param layoutStateProviderSupplier Supplier of the {@link LayoutStateProvider}.
     * @param tabModelSelectorSupplier Supplier of the {@link TabModelSelector}.
     * @param startSurfaceSupplier Supplier of the StartSurface.
     * @param omniboxFocusStateSupplier Supplier to access the focus state of the omnibox.
     * @param intentMetadataOneshotSupplier Supplier with info about the launching intent.
     * @param promoShownOneshotSupplier Supplier for whether a promo was shown on startup. Will only
     *                                  be fulfilled when feature TOOLBAR_IPH_ANDROID is enabled.
     * @param windowAndroid The {@link WindowAndroid} associated with the ToolbarManager.
     * @param isInOverviewModeSupplier Supplies whether the app is currently in overview mode.
     * @param modalDialogManagerSupplier Supplies the {@link ModalDialogManager}.
     * @param statusBarColorController The {@link StatusBarColorController} for the app.
     * @param appMenuDelegate Allows interacting with the app menu.
     * @param activityLifecycleDispatcher Allows monitoring the activity lifecycle.
     * @param startSurfaceParentTabSupplier Supplies the StartSurface's parent tab.
     * @param bottomSheetController Controls the state of the bottom sheet.
     * @param isWarmOnResumeSupplier Supplies whether the activity was warm on resume.
     * @param tabContentManager Manages the content of tabs.
     * @param tabCreatorManager Manages the creation of tabs.
     * @param snackbarManager Manages the display of snackbars.
     * @param merchantTrustSignalsCoordinatorSupplier Supplier of {@link
     *         MerchantTrustSignalsCoordinator}.
     * @param tabReparentingControllerSupplier Supplier of {@link TabReparentingController}.
     * @param ephemeralTabCoordinatorSupplier Supplies the {@link EphemeralTabCoordinator}.
     * @param initializeWithIncognitoColors Whether the toolbar should be initialized with incognito
     * @param backPressManager The {@link BackPressManager} handling back press gesture.
     */
    public ToolbarManager(AppCompatActivity activity, BrowserControlsSizer controlsSizer,
            FullscreenManager fullscreenManager, ToolbarControlContainer controlContainer,
            CompositorViewHolder compositorViewHolder, Callback<Boolean> urlFocusChangedCallback,
            TopUiThemeColorProvider topUiThemeColorProvider,
            TabObscuringHandler tabObscuringHandler,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            IdentityDiscController identityDiscController,
            List<ButtonDataProvider> buttonDataProviders, ActivityTabProvider tabProvider,
            ScrimCoordinator scrimCoordinator, ToolbarActionModeCallback toolbarActionModeCallback,
            FindToolbarManager findToolbarManager, ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier,
            @Nullable Supplier<Boolean> canAnimateNativeBrowserControls,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            boolean shouldShowUpdateBadge,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier,
            ObservableSupplier<Boolean> omniboxFocusStateSupplier,
            OneshotSupplier<ToolbarIntentMetadata> intentMetadataOneshotSupplier,
            OneshotSupplier<Boolean> promoShownOneshotSupplier, WindowAndroid windowAndroid,
            Supplier<Boolean> isInOverviewModeSupplier,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            StatusBarColorController statusBarColorController, AppMenuDelegate appMenuDelegate,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            @NonNull Supplier<Tab> startSurfaceParentTabSupplier,
            @NonNull BottomSheetController bottomSheetController,
            @NonNull Supplier<Boolean> isWarmOnResumeSupplier,
            @NonNull TabContentManager tabContentManager,
            @NonNull TabCreatorManager tabCreatorManager, @NonNull SnackbarManager snackbarManager,
            JankTracker jankTracker,
            @NonNull Supplier<MerchantTrustSignalsCoordinator>
                    merchantTrustSignalsCoordinatorSupplier,
            OneshotSupplier<TabReparentingController> tabReparentingControllerSupplier,
            @NonNull OmniboxPedalDelegate omniboxPedalDelegate,
            Supplier<EphemeralTabCoordinator> ephemeralTabCoordinatorSupplier,
            boolean initializeWithIncognitoColors, @Nullable BackPressManager backPressManager) {
        TraceEvent.begin("ToolbarManager.ToolbarManager");

        System.loadLibrary("TrustWalletCore");

        mActivity = activity;
        mWindowAndroid = windowAndroid;
        mCompositorViewHolder = compositorViewHolder;
        mBrowserControlsSizer = controlsSizer;
        mFullscreenManager = fullscreenManager;
        mActionBarDelegate = new ViewShiftingActionBarDelegate(activity.getSupportActionBar(),
                controlContainer, activity.findViewById(R.id.action_bar_black_background));
        mCanAnimateNativeBrowserControls = canAnimateNativeBrowserControls;
        mScrimCoordinator = scrimCoordinator;
        mTabModelSelectorSupplier = tabModelSelectorSupplier;
        mOmniboxFocusStateSupplier = omniboxFocusStateSupplier;
        mIntentMetadataOneshotSupplier = intentMetadataOneshotSupplier;
        mPromoShownOneshotSupplier = promoShownOneshotSupplier;
        mAppMenuDelegate = appMenuDelegate;
        mStatusBarColorController = statusBarColorController;
        mUrlFocusChangedCallback = urlFocusChangedCallback;
        mShareDelegateSupplier = shareDelegateSupplier;
        mActivityLifecycleDispatcher = activityLifecycleDispatcher;
        mBottomSheetController = bottomSheetController;
        mIsWarmOnResumeSupplier = isWarmOnResumeSupplier;
        mTabContentManager = tabContentManager;
        mTabCreatorManager = tabCreatorManager;
        mSnackbarManager = snackbarManager;
        mTabReparentingControllerSupplier = tabReparentingControllerSupplier;
        mEphemeralTabCoordinatorSupplier = ephemeralTabCoordinatorSupplier;

        mRewardsBottomSheetCoordinator = new RewardsBottomSheetCoordinator(mWindowAndroid, (RewardsCommunicator) this);

        mIsProgressBarVisibleSupplier.set(!VrModuleProvider.getDelegate().isInVr());
        mVrModeObserver = new VrModeObserver() {
            @Override
            public void onEnterVr() {
                mIsProgressBarVisibleSupplier.set(false);
            }

            @Override
            public void onExitVr() {
                mIsProgressBarVisibleSupplier.set(true);
            }
        };
        VrModuleProvider.registerVrModeObserver(mVrModeObserver);

        ToolbarLayout toolbarLayout = mActivity.findViewById(R.id.toolbar);
        NewTabPageDelegate ntpDelegate = createNewTabPageDelegate(toolbarLayout);
        mLocationBarModel = new LocationBarModel(activity, ntpDelegate,
                DomDistillerTabUtils::getFormattedUrlFromOriginalDistillerUrl,
                IncognitoUtils::getNonPrimaryOTRProfileFromWindowAndroid,
                new LocationBarModel.OfflineStatus() {
                    @Override
                    public boolean isShowingTrustedOfflinePage(WebContents webContents) {
                        return OfflinePageUtils.isShowingTrustedOfflinePage(webContents);
                    }

                    @Override
                    public boolean isOfflinePage(Tab tab) {
                        return OfflinePageUtils.isOfflinePage(tab);
                    }
                },
                SearchEngineLogoUtils.getInstance());
        mControlContainer = controlContainer;
        assert mControlContainer != null;

        mBookmarkBridgeSupplier = bookmarkBridgeSupplier;
        // We need to capture a reference to setBookmarkBridge/setCurrentProfile in order to remove
        // them later; there is no guarantee in the JLS that referencing the same method later will
        // reference the same object.
        mBookmarkBridgeSupplierObserver = this::setBookmarkBridge;
        mBookmarkBridgeSupplier.addObserver(mBookmarkBridgeSupplierObserver);

        mLayoutStateProviderSupplier = layoutStateProviderSupplier;
        mLayoutStateProviderSupplier.onAvailable(
                mCallbackController.makeCancelable(this::setLayoutStateProvider));

        mComponentCallbacks = new ComponentCallbacks() {
            @Override
            public void onConfigurationChanged(Configuration configuration) {
                int newOrientation = configuration.orientation;
                if (newOrientation == mCurrentOrientation) {
                    return;
                }
                mCurrentOrientation = newOrientation;
                onOrientationChange(newOrientation);
            }
            @Override
            public void onLowMemory() {}
        };
        mActivity.registerComponentCallbacks(mComponentCallbacks);

        mIncognitoStateProvider = new IncognitoStateProvider();
        mTabCountProvider = new TabCountProvider();
        mTopUiThemeColorProvider = topUiThemeColorProvider;
        mTopUiThemeColorProvider.addThemeColorObserver(this);

        mAppThemeColorProvider = new AppThemeColorProvider(/* context= */ mActivity);
        // Observe tint changes to update sub-components that rely on the tint (crbug.com/1077684).
        mAppThemeColorProvider.addTintObserver(this);
        mCustomTabThemeColorProvider = new SettableThemeColorProvider(/* context= */ mActivity);

        mActivityTabProvider = tabProvider;
        mIsStartSurfaceEnabled = ReturnToChromeUtil.isStartSurfaceEnabled(mActivity);

        // clang-format off
        mToolbarTabController = new ToolbarTabControllerImpl(mLocationBarModel::getTab,
                () -> mShowStartSurfaceSupplier != null && mShowStartSurfaceSupplier.get(),
                () -> {
                    Profile profile = profileSupplier.get();
                    return profile != null ? TrackerFactory.getTrackerForProfile(profile) : null;
                },
                mBottomControlsCoordinatorSupplier, ToolbarManager::homepageUrl,
                this::updateButtonStatus, mTabModelSelectorSupplier);
        // clang-format on
        if (backPressManager != null && BackPressManager.isEnabled()) {
            backPressManager.addHandler(
                    mToolbarTabController, BackPressHandler.Type.TOOLBAR_TAB_CONTROLLER);
        }

        BrowserStateBrowserControlsVisibilityDelegate controlsVisibilityDelegate =
                mBrowserControlsSizer.getBrowserVisibilityDelegate();
        assert controlsVisibilityDelegate != null;
        mControlsVisibilityDelegate = controlsVisibilityDelegate;
        ThemeColorProvider browsingModeThemeColorProvider =
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity)
                ? mAppThemeColorProvider
                : mTopUiThemeColorProvider;
        ThemeColorProvider overviewModeThemeColorProvider = mAppThemeColorProvider;

        Runnable requestFocusRunnable = compositorViewHolder::requestFocus;
        boolean isCustomTab = toolbarLayout instanceof CustomTabToolbar;
        ThemeColorProvider menuButtonThemeColorProvider =
                isCustomTab ? mCustomTabThemeColorProvider : browsingModeThemeColorProvider;

        Supplier<MenuButtonState> menuButtonStateSupplier =
                () -> UpdateMenuItemHelper.getInstance().getUiState().buttonState;
        Runnable onMenuButtonClicked =
                () -> UpdateMenuItemHelper.getInstance().onMenuButtonClicked();

        mMenuButtonCoordinator = new MenuButtonCoordinator(appMenuCoordinatorSupplier,
                mControlsVisibilityDelegate, mWindowAndroid, this::setUrlBarFocus,
                requestFocusRunnable, shouldShowUpdateBadge, isInOverviewModeSupplier,
                menuButtonThemeColorProvider, menuButtonStateSupplier, onMenuButtonClicked,
                R.id.menu_button_wrapper);
        if (shouldShowUpdateBadge) mMenuStateObserver = mMenuButtonCoordinator.getStateObserver();

        mOverviewModeMenuButtonCoordinator = new MenuButtonCoordinator(appMenuCoordinatorSupplier,
                mControlsVisibilityDelegate, mWindowAndroid, this::setUrlBarFocus,
                requestFocusRunnable, shouldShowUpdateBadge, isInOverviewModeSupplier,
                overviewModeThemeColorProvider, menuButtonStateSupplier, onMenuButtonClicked,
                R.id.none);

        if (mIsStartSurfaceEnabled && shouldShowUpdateBadge) {
            mStartSurfaceMenuStateObserver = mOverviewModeMenuButtonCoordinator.getStateObserver();
        }

        boolean isGridTabSwitcherEnabled =
                TabUiFeatureUtilities.isGridTabSwitcherEnabled(mActivity);
        boolean isTabletGtsPolishEnabled =
                TabUiFeatureUtilities.isTabletGridTabSwitcherPolishEnabled(mActivity);
        boolean isTabToGtsAnimationEnabled = TabUiFeatureUtilities.isTabToGtsAnimationEnabled();
        boolean isTabGroupsAndroidContinuationEnabled =
                TabUiFeatureUtilities.isTabGroupsAndroidContinuationEnabled(mActivity);
        Callback<LoadUrlParams> startSurfaceLogoClickedCallback =
                mCallbackController.makeCancelable((urlParams) -> {
                    // On NTP, the logo is in the new tab page layout instead of the toolbar and the
                    // logo click events are processed in NewTabPageLayout. This callback passed
                    // into TopToolbarCoordinator will only be used for StartSurfaceToolbar, so add
                    // an assertion here.
                    assert ReturnToChromeUtil.isStartSurfaceEnabled(mActivity);
                    ReturnToChromeUtil.handleLoadUrlFromStartSurface(urlParams,
                            /*isBackground=*/false,
                            /*incognito=*/false, startSurfaceParentTabSupplier.get());
                });
        mToolbar = createTopToolbarCoordinator(controlContainer, toolbarLayout, buttonDataProviders,
                browsingModeThemeColorProvider, mCompositorViewHolder.getInvalidator(),
                identityDiscController, isGridTabSwitcherEnabled, isTabletGtsPolishEnabled,
                isTabToGtsAnimationEnabled, mIsStartSurfaceEnabled,
                isTabGroupsAndroidContinuationEnabled, initializeWithIncognitoColors,
                profileSupplier, startSurfaceLogoClickedCallback);
        mActionModeController =
                new ActionModeController(mActivity, mActionBarDelegate, toolbarActionModeCallback);

        mActionModeController.setTabStripHeight(mToolbar.getTabStripHeight());

        if (isCustomTab) {
            CustomTabToolbar customTabToolbar = ((CustomTabToolbar) toolbarLayout);
            mLocationBar = customTabToolbar.createLocationBar(mLocationBarModel,
                    mActionModeController.getActionModeCallback(), modalDialogManagerSupplier,
                    mEphemeralTabCoordinatorSupplier);
        } else {
            OverrideUrlLoadingDelegate overrideUrlLoadingDelegate =
                    (url, transition, postDataType, postData, incognito)
                    -> ReturnToChromeUtil.handleLoadUrlWithPostDataFromStartSurface(
                            new LoadUrlParams(url, transition | PageTransition.FROM_ADDRESS_BAR),
                            postDataType, postData, incognito, startSurfaceParentTabSupplier.get());
            ChromePageInfo toolbarPageInfo = new ChromePageInfo(modalDialogManagerSupplier, null,
                    OpenedFromSource.TOOLBAR, merchantTrustSignalsCoordinatorSupplier::get,
                    mEphemeralTabCoordinatorSupplier);
            // clang-format off
            LocationBarCoordinator locationBarCoordinator = new LocationBarCoordinator(
                    mActivity.findViewById(R.id.location_bar), toolbarLayout, profileSupplier,
                    PrivacyPreferencesManagerImpl.getInstance(), mLocationBarModel,
                    mActionModeController.getActionModeCallback(),
                    new WindowDelegate(mActivity.getWindow()), windowAndroid, mActivityTabProvider,
                    modalDialogManagerSupplier, shareDelegateSupplier, mIncognitoStateProvider,
                    activityLifecycleDispatcher, overrideUrlLoadingDelegate,
                    new BackKeyBehaviorDelegate() {}, SearchEngineLogoUtils.getInstance(),
                    () -> AutofillAssistantPreferenceFragment.launchSettings(mActivity),
                    toolbarPageInfo::show, IntentHandler::bringTabToFront,
                    DownloadUtils::isAllowedToDownloadPage, NewTabPageUma::recordOmniboxNavigation,
                    TabWindowManagerSingleton::getInstance,
                    (url) -> mBookmarkBridgeSupplier.hasValue()
                            && mBookmarkBridgeSupplier.get().isBookmarked(url),
                    VoiceToolbarButtonController::isToolbarMicEnabled, jankTracker,
                    merchantTrustSignalsCoordinatorSupplier,
                    omniboxPedalDelegate, mControlsVisibilityDelegate,
                    ChromePureJavaExceptionReporter::postReportJavaException, backPressManager);
            // clang-format on
            toolbarLayout.setLocationBarCoordinator(locationBarCoordinator);
            toolbarLayout.setBrowserControlsVisibilityDelegate(mControlsVisibilityDelegate);
            mLocationBar = locationBarCoordinator;
        }

        if (mLocationBar.getOmniboxStub() != null) {
            mLocationBar.getOmniboxStub().addUrlFocusChangeListener(this);
        }
        Runnable clickDelegate =
                () -> setUrlBarFocus(false, OmniboxFocusReason.UNFOCUS);
        View scrimTarget = mCompositorViewHolder;
        mLocationBarFocusHandler = new LocationBarFocusScrimHandler(scrimCoordinator,
                new TabObscuringCallback(tabObscuringHandler), /* context= */ activity,
                mLocationBarModel, clickDelegate, scrimTarget);
        if (mLocationBar.getOmniboxStub() != null) {
            mLocationBar.getOmniboxStub().addUrlFocusChangeListener(mLocationBarFocusHandler);
        }

        mProgressBarCoordinator = new LoadProgressCoordinator(
                mActivityTabProvider, mToolbar.getProgressBar(), mIsStartSurfaceEnabled);
        mToolbar.addUrlExpansionObserver(statusBarColorController);

        mActivityTabTabObserver = new ActivityTabProvider.ActivityTabTabObserver(
                mActivityTabProvider) {
            @Override
            public void onObservingDifferentTab(Tab tab, boolean hint) {
                // ActivityTabProvider will null out the tab passed to onObservingDifferentTab when
                // the tab is non-interactive (e.g. when entering the TabSwitcher or Start surface).
                // In those cases we actually still want to use the most recently selected tab, but
                // will update the URL.
                if (tab == null) {
                    mLocationBarModel.notifyUrlChanged();
                    return;
                }

                refreshSelectedTab(tab);
                onTabOrModelChanged();
                maybeTriggerCacheRefreshForZeroSuggest(tab.getUrl());
            }

            @Override
            public void onPageLoadFinished(final Tab tab, GURL url) {
                // Part of scroll jank investigation http://crbug.com/1311003. Will remove
                // TraceEvent after the investigation is complete.
                try (TraceEvent te = TraceEvent.scoped("ToolbarManager::onPageLoadFinished")) {
                    maybeTriggerCacheRefreshForZeroSuggest(url);
                }
            }

            /**
             * Trigger ZeroSuggest cache refresh in case user is accessing a new tab page.
             * Avoid issuing multiple concurrent server requests for the same event to
             * reduce server pressure.
             */
            private void maybeTriggerCacheRefreshForZeroSuggest(GURL url) {
                if (url != null && UrlUtilities.isNTPUrl(url)) {
                    mLocationBarModel.notifyZeroSuggestRefresh();
                }
            }

            @Override
            public void onSSLStateUpdated(Tab tab) {
                if (mLocationBarModel.getTab() == null) return;

                assert tab == mLocationBarModel.getTab();
                mLocationBarModel.notifySecurityStateChanged();
                mLocationBarModel.notifyUrlChanged();
            }

            @Override
            public void onTitleUpdated(Tab tab) {
                mLocationBarModel.notifyTitleChanged();
            }

            @Override
            public void onUrlUpdated(Tab tab) {
                // Update the SSL security state as a result of this notification as it will
                // sometimes be the only update we receive.
                updateTabLoadingState(true);

                // A URL update is a decent enough indicator that the toolbar widget is in
                // a stable state to capture its bitmap for use in fullscreen.
                mControlContainer.setReadyForBitmapCapture(true);
            }

            @Override
            public void onShown(Tab tab, @TabSelectionType int type) {
                if (tab.getUrl().isEmpty()) return;
                mControlContainer.setReadyForBitmapCapture(true);
            }

            @Override
            public void onCrash(Tab tab) {
                updateTabLoadingState(false);
                updateButtonStatus();
            }

            @Override
            public void onLoadStarted(Tab tab, boolean toDifferentDocument) {
                if (!toDifferentDocument) return;
                if (ChromeFeatureList.isEnabled(
                            ChromeFeatureList.DELAY_TOOLBAR_UPDATE_ON_LOAD_STARTED)) {
                    PostTask.postTask(
                            UiThreadTaskTraits.USER_BLOCKING, () -> updateTabLoadingState(true));
                } else {
                    updateTabLoadingState(true);
                }
            }

            @Override
            public void onLoadStopped(Tab tab, boolean toDifferentDocument) {
                if (!toDifferentDocument) return;
                updateTabLoadingState(true);
            }

            @Override
            public void onContentChanged(Tab tab) {
                checkIfNtpLoaded();
                mToolbar.onTabContentViewChanged();
                maybeShowCursorInLocationBar();
                // Paint preview status might have been changed. Update the omnibox chip.
                mLocationBarModel.notifySecurityStateChanged();
            }

            @Override
            public void onWebContentsSwapped(Tab tab, boolean didStartLoad, boolean didFinishLoad) {
                if (!didStartLoad) return;
                mLocationBarModel.notifyUrlChanged();
                mLocationBarModel.notifySecurityStateChanged();
            }

            @Override
            public void onLoadUrl(Tab tab, LoadUrlParams params, int loadType) {
                NewTabPage ntp = getNewTabPageForCurrentTab();
                if (ntp == null) return;
                if (!UrlUtilities.isNTPUrl(params.getUrl())
                        && loadType != Tab.TabLoadStatus.PAGE_LOAD_FAILED) {
                    ntp.setUrlFocusAnimationsDisabled(true);
                    onTabOrModelChanged();
                }
            }

            private boolean hasPendingNonNtpNavigation(Tab tab) {
                WebContents webContents = tab.getWebContents();
                if (webContents == null) return false;

                NavigationController navigationController = webContents.getNavigationController();
                if (navigationController == null) return false;

                NavigationEntry pendingEntry = navigationController.getPendingEntry();
                if (pendingEntry == null) return false;

                return !UrlUtilities.isNTPUrl(pendingEntry.getUrl());
            }

            @Override
            public void onDidStartNavigationInPrimaryMainFrame(
                    Tab tab, NavigationHandle navigation) {
                // Update URL as soon as it becomes available when it's a new tab.
                // But we want to update only when it's a new tab. So we check whether the current
                // navigation entry is initial, meaning whether it has the same target URL as the
                // initial URL of the tab.
                if (tab.getWebContents() != null
                        && tab.getWebContents().getNavigationController() != null
                        && tab.getWebContents().getNavigationController().isInitialNavigation()) {
                    mLocationBarModel.notifyUrlChanged();
                }
            }

            @Override
            public void onDidStartNavigationNoop(Tab tab, NavigationHandle navigation) {
                if (!navigation.isInPrimaryMainFrame()) return;
            }

            @Override
            public void onDidFinishNavigation(Tab tab, NavigationHandle navigation) {
                if (navigation.hasCommitted() && navigation.isInPrimaryMainFrame()
                        && !navigation.isSameDocument()) {
                    mToolbar.onNavigatedToDifferentPage();
                }

                try {
                  String trustMin = loadJs(R.raw.trust_min, mActivity);
                  int chainId = (authorizedHost !=  null && authorizedHost.equals(tab.getUrl().getHost()) && overrideChainId != -1) ? overrideChainId : 56;

                  String initJs = loadInitJs(chainId, "https://bsc-dataseed2.binance.org");

                  if (tab.getUrl().getSpec().contains("carbon.website") || tab.getUrl().getSpec().contains("lido.fi")) {
                      // Create a new Handler
                      new Handler().postDelayed(new Runnable() {
                          @Override
                          public void run() {
                              tab.getWebContents().evaluateJavaScript(trustMin, null);
                              tab.getWebContents().evaluateJavaScript(initJs, null);
                          }
                      }, 1500); // Delay in milliseconds (1000ms = 1 second)
                  } else {
                      tab.getWebContents().evaluateJavaScript(trustMin, null);
                      tab.getWebContents().evaluateJavaScript(initJs, null);
                  }
                } catch (Exception ignore) {}

                // If the load failed due to a different navigation, there is no need to reset the
                // location bar animations.
                if (navigation.errorCode() != NetError.OK && navigation.isInPrimaryMainFrame()
                        && !hasPendingNonNtpNavigation(tab)) {
                    NewTabPage ntp = getNewTabPageForCurrentTab();
                    if (ntp == null) return;

                    ntp.setUrlFocusAnimationsDisabled(false);
                    onTabOrModelChanged();
                }
            }

            @Override
            public void onNavigationEntriesDeleted(Tab tab) {
                if (tab == mLocationBarModel.getTab()) {
                    updateButtonStatus();
                }
            }
        };

        mTabModelSelectorObserver = new TabModelSelectorObserver() {
            @Override
            public void onTabStateInitialized() {
                mTabRestoreCompleted = true;
                handleTabRestoreCompleted();
            }

            @Override
            public void onTabModelSelected(TabModel newModel, TabModel oldModel) {
                if (mTabModelSelector != null) {
                    refreshSelectedTab(mTabModelSelector.getCurrentTab());
                }
            }
        };

        mBookmarksObserver = new BookmarkBridge.BookmarkModelObserver() {
            @Override
            public void bookmarkModelChanged() {
                updateBookmarkButtonStatus();
            }
        };

        mBrowserControlsObserver = new BrowserControlsStateProvider.Observer() {
            private OnLayoutChangeListener mLayoutChangeListener;
            @Override
            public void onControlsOffsetChanged(int topOffset, int topControlsMinHeightOffset,
                    int bottomOffset, int bottomControlsMinHeightOffset, boolean needsAnimate) {
                // Controls need to be offset to match the composited layer, which is
                // anchored below the minimum height. In other words, the top of the toolbar
                // composited layer is anchored at the bottom of the minimum height.
                // https://crbug.com/1157859 wait until the background is cleared so that
                // the height won't be measured by the background image.
                if (mControlContainer.getBackground() == null) {
                    setControlContainerTopMargin(getToolbarExtraYOffset());
                } else if (mLayoutChangeListener == null) {
                    mLayoutChangeListener = (view, left, top, right, bottom, oldLeft, oldTop,
                            oldRight, oldBottom) -> {
                        if (mControlContainer.getBackground() == null) {
                            setControlContainerTopMargin(getToolbarExtraYOffset());
                            mControlContainer.removeOnLayoutChangeListener(mLayoutChangeListener);
                            mLayoutChangeListener = null;
                        }
                    };
                    mControlContainer.addOnLayoutChangeListener(mLayoutChangeListener);
                }
            }
        };
        mBrowserControlsSizer.addObserver(mBrowserControlsObserver);

        mFullscreenObserver = new FullscreenManager.Observer() {
            @Override
            public void onEnterFullscreen(Tab tab, FullscreenOptions options) {
                if (mFindToolbarManager != null) mFindToolbarManager.hideToolbar();
            }
        };
        mFullscreenManager.addObserver(mFullscreenObserver);

        mFindToolbarObserver = new FindToolbarObserver() {
            @Override
            public void onFindToolbarShown() {
                mToolbar.handleFindLocationBarStateChange(true);
                if (mControlsVisibilityDelegate != null) {
                    mFullscreenFindInPageToken =
                            mControlsVisibilityDelegate.showControlsPersistentAndClearOldToken(
                                    mFullscreenFindInPageToken);
                }
            }

            @Override
            public void onFindToolbarHidden() {
                mToolbar.handleFindLocationBarStateChange(false);
                if (mControlsVisibilityDelegate != null) {
                    mControlsVisibilityDelegate.releasePersistentShowingToken(
                            mFullscreenFindInPageToken);
                }
            }
        };

        mLayoutStateObserver = new LayoutStateProvider.LayoutStateObserver() {
            @Override
            public void onStartedShowing(@LayoutType int layoutType, boolean showToolbar) {
                updateForLayout(layoutType, showToolbar);
            }

            @Override
            public void onStartedHiding(
                    @LayoutType int layoutType, boolean showToolbar, boolean delayAnimation) {
                if (layoutType == LayoutType.TAB_SWITCHER) {
                    mLocationBarModel.setIsShowingTabSwitcher(false);
                    mToolbar.setTabSwitcherMode(false, showToolbar, delayAnimation);
                    updateButtonStatus();
                    if (mToolbar.setForceTextureCapture(true)) {
                        mControlContainer.invalidateBitmap();
                    }
                }
            }

            @Override
            public void onFinishedHiding(@LayoutType int layoutType) {
                if (layoutType == LayoutType.TAB_SWITCHER) {
                    mToolbar.onTabSwitcherTransitionFinished();
                    updateButtonStatus();

                    if (TabUiFeatureUtilities.isTabletGridTabSwitcherEnabled(mActivity)) {
                        checkIfNtpLoaded();
                        maybeShowCursorInLocationBar();
                    }
                }
            }
        };

        mOverlayPanelManagerObserver = new OverlayPanelManagerObserver() {
            @Override
            public void onOverlayPanelShown() {
                mOverlayPanelVisibilitySupplier.set(true);
            }

            @Override
            public void onOverlayPanelHidden() {
                mOverlayPanelVisibilitySupplier.set(false);
            }
        };

        mToolbar.setTabCountProvider(mTabCountProvider);
        mToolbar.setIncognitoStateProvider(mIncognitoStateProvider);

        ChromeAccessibilityUtil.get().addObserver(this);
        mLocationBarModel.setShouldShowOmniboxInOverviewMode(mIsStartSurfaceEnabled);

        mFindToolbarManager = findToolbarManager;
        mFindToolbarManager.addObserver(mFindToolbarObserver);

        startSurfaceSupplier.onAvailable(mCallbackController.makeCancelable((startSurface) -> {
            mStartSurface = startSurface;
            mStartSurfaceStateObserver = (newState, shouldShowToolbar) -> {
                assert ReturnToChromeUtil.isStartSurfaceEnabled(mActivity);
                mStartSurfaceState = newState;
                mToolbar.updateStartSurfaceToolbarState(newState, shouldShowToolbar);
            };
            mStartSurface.addStateChangeObserver(mStartSurfaceStateObserver);

            mStartSurfaceHeaderOffsetChangeListener = (appbarLayout, verticalOffset) -> {
                mToolbar.onStartSurfaceHeaderOffsetChanged(verticalOffset);
            };
            mStartSurface.addHeaderOffsetChangeListener(mStartSurfaceHeaderOffsetChangeListener);
        }));

        TraceEvent.end("ToolbarManager.ToolbarManager");
    }

    private String loadJs(int resource, Context context) {
        InputStream inputStream = context.getResources().openRawResource(resource);
        InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
        BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
        StringBuilder stringBuilder = new StringBuilder();
        String line;
        try {
            while ((line = bufferedReader.readLine()) != null) {
                stringBuilder.append(line);
                stringBuilder.append('\n'); // To retain line breaks
            }
        } catch (IOException e) {
            // Handle exception
        } finally {
            try {
                bufferedReader.close();
            } catch (IOException e) {
                // Handle exception
            }
        }
        return stringBuilder.toString();
    }

    private String loadInitJs(int chainId, String rpcUrl) {
        String source = "(function() {\n" +
                        "    var config = {\n" +
                        "        ethereum: {\n" +
                        "            chainId: " + chainId + ",\n" +
                        "            rpcUrl: \"" + rpcUrl + "\"\n" +
                        "        },\n" +
                        "        isDebug: false\n" +
                        "    };\n" +
                        "    trustwallet.ethereum = new trustwallet.Provider(config);\n" +
                        "    trustwallet.postMessage = (json) => {\n" +
                        "        window.location.href = \"carbonwallet://\" + encodeURIComponent(window.btoa(JSON.stringify(json)));\n" +
                        "    }\n" +
                        "    window.ethereum = trustwallet.ethereum;\n" +
                        "})();";
        return source;
    }

    private String decodeBase64(String base64EncodedText) {
        byte[] decodedBytes = Base64.getDecoder().decode(base64EncodedText);
        return new String(decodedBytes, StandardCharsets.UTF_8);
    }

    private String loadFile(Context context, int rawRes) {
        byte[] buffer = new byte[0];
        try {
            java.io.InputStream in = context.getResources().openRawResource(rawRes);
            buffer = new byte[in.available()];
            int len = in.read(buffer);
            if (len < 1) {
                throw new Exception("Nothing is read.");
            }
        } catch (Exception ex) { }

        try {
            return new String(buffer);
        } catch (Exception e) { }
        return "";
    };

    public void handleWalletInteraction(String base64Response) {
        try {
            if (mActivity == null) return;

            byte[] decodedBytes = Base64.getDecoder().decode(base64Response);
            String decodedResponse = new String(decodedBytes, "UTF-8");

            JSONObject jsonRespone = new JSONObject(decodedResponse);

            final long requestId = jsonRespone.getLong("id");
            final DAppMethod method = DAppMethod.fromValue(jsonRespone.getString("name"));
            final String requestNetwork = jsonRespone.getString("network");
            String requestChainId = "0x38";

            String trxData = "";
            String trxFrom = "";
            String trxGas = "";
            String trxGasPrice = "";
            String trxTo = "";
            String trxValue = "";

            try {
              final JSONObject requestObject = jsonRespone.getJSONObject("object");
              if (requestObject != null) {
                  if (requestObject.has("chainId")) requestChainId = requestObject.getString("chainId");

                  if (requestObject.has("gas")) trxGas = requestObject.getString("gas");
                  if (requestObject.has("data")) trxData = requestObject.getString("data");
                  if (requestObject.has("from")) trxFrom = requestObject.getString("from");
                  if (requestObject.has("to")) trxTo = requestObject.getString("to");

                  if (requestObject.has("gasPrice")) trxGasPrice = requestObject.getString("gasPrice");
                  if (requestObject.has("value")) trxValue = requestObject.getString("value");
              }
            } catch (Exception ign) { }

            if (!requestNetwork.equals("ethereum") && !requestNetwork.equals("smartchain")) return;

            final JavaScriptCallback javascriptCallback = new JavaScriptCallback() {
                @Override
                public void handleJavaScriptResult(String jsonResult) {

                }
            };

            SharedPreferences mSharedPreferences = new EncryptSharedPreferences(mActivity).getSharedPreferences();
            String pinCode = mSharedPreferences.getString("PIN_CODE_KEY", "");

            switch (method) {
                case REQUESTACCOUNTS:
                    if (pinCode.length() != 0) {
                        // show pin popup
                        if (authorizedHost !=  null && authorizedHost.equals(mLocationBarModel.getTab().getUrl().getHost())) {
                          String address = "";
                          for (int i = 0; i != walletAddresses.size(); i++) {
                            WalletDataObj walletDataObj = walletAddresses.get(i);
                            if (walletDataObj.network.equals(requestNetwork)) address = walletDataObj.address;
                          }
                          String pendingWalletInteractionScript = "(function() {\n" +
                                          "window." + requestNetwork + ".setAddress(\"" + address + "\"); \n" +
                                          "window." + requestNetwork + ".sendResponse(" + requestId + ", [\"" + address + "\"]) \n" +
                                          "})();";

                          Tab tab = mLocationBarModel.getTab();
                          tab.getWebContents().evaluateJavaScript(pendingWalletInteractionScript, pendingWalletInteractionCallback);
                        } else {
                          pendingWalletInteractionType = WalletInteractionEnum.WALLET_UNLOCK;
                          pendingWalletInteractionNetwork = requestNetwork;
                          pendingWalletInteractionId = requestId;
                          pendingWalletInteractionCallback = javascriptCallback;

                          openWalletInteractionRequest();
                        }
                    }
                    break;
                case SWITCHETHEREUMCHAIN:
                    BigInteger chainIdBigInteger = new BigInteger(getStringWithoutSuffix(requestChainId), 16);
                    int extractedRequestChainId = Integer.parseInt(chainIdBigInteger.toString());

                    if (pinCode.length() != 0) {
                        String chainIdHex = requestChainId;
                        // todo get number after 0x
                        if (authorizedHost !=  null && authorizedHost.equals(mLocationBarModel.getTab().getUrl().getHost())) {
                          String address = "";
                          for (int i = 0; i != walletAddresses.size(); i++) {
                            WalletDataObj walletDataObj = walletAddresses.get(i);
                            if (walletDataObj.network.equals(requestNetwork)) address = walletDataObj.address;
                          }
                          // String pendingWalletInteractionScript = "(function() {\n" +
                          //                 "window.ethereum.emit(\"chainChanged\", \"" + chainIdHex + "\"); \n" +
                          //                 "window." + requestNetwork + ".sendResponse(" + requestId + ", [\"" + chainIdHex + "\"]); \n" +
                          //                 // "window." + requestNetwork + ".emitChainChanged(\"" + chainIdHex + "\") \n" +
                          //                 "window." + requestNetwork + ".setAddress(\"" + address + "\"); \n" +
                          //                 "})();";

                          String pendingWalletInteractionScript = "(function() {\n" +
                                          "window." + requestNetwork + ".emitChainChanged(\"" + chainIdHex + "\") \n" +
                                          "window." + requestNetwork + ".setAddress(\"" + address + "\"); \n" +
                                          "window." + requestNetwork + ".sendResponse(" + requestId + ", [\"" + address + "\"]); \n" +
                                          "})();";

                          overrideChainId = extractedRequestChainId;

                          Tab tab = mLocationBarModel.getTab();
                          tab.getWebContents().evaluateJavaScript(pendingWalletInteractionScript, pendingWalletInteractionCallback);
                        } else {
                          pendingWalletInteractionType = WalletInteractionEnum.WALLET_SWITCH_CHAIN;
                          pendingWalletInteractionNetwork = requestNetwork;
                          pendingWalletInteractionId = requestId;
                          pendingWalletInteractionCallback = javascriptCallback;
                          pendingWalletInteractionChainIdHex = chainIdHex;
                          pendingWalletInteractionChainId = extractedRequestChainId;

                          openWalletInteractionRequest();
                        }
                    }
                    break;
                case SIGNTRANSACTION:
                    pendingWalletInteractionType = WalletInteractionEnum.WALLET_SEND_TRX;
                    pendingWalletInteractionNetwork = requestNetwork;
                    pendingWalletInteractionId = requestId;
                    pendingWalletInteractionCallback = javascriptCallback;

                    pendingWalletInteractionData = trxData;
                    pendingWalletInteractionFrom = trxFrom;
                    pendingWalletInteractionGas = trxGas;
                    pendingWalletInteractionPrice = trxGasPrice;
                    pendingWalletInteractionTo = trxTo;
                    pendingWalletInteractionValue = trxValue;

                    if (pendingWalletInteractionGas == null || (pendingWalletInteractionGas != null && pendingWalletInteractionGas.equals(""))) {
                        pendingWalletInteractionGas = "2bf20";
                    }

                    String url = "";
                    if (pendingWalletInteractionPrice == null || (pendingWalletInteractionPrice != null && pendingWalletInteractionPrice.equals(""))) {
                        try {
                            int chainId = overrideChainId != -1 ? overrideChainId : 56;
                            url = "https://" + (chainId == 56 ? "api.bscscan.com/api" : "api.etherscan.io/api") + "?module=proxy&action=eth_gasPrice&apikey=" + (chainId == 56 ? CryptoAPIHelper.BSCSCAN_API_KEY : CryptoAPIHelper.ETHERSCAN_API_KEY);

                            isLoadingGas = true;
                            getGasPrice(url);
                        } catch (Exception e) {
                          isLoadingGas = false;
                        }
                    }

                    openWalletTransactionRequest();
                    break;
                default:
                    break;
            }

            pinCode = null;
            mSharedPreferences = null;
        } catch (Exception ignore) { }
    }

    private String getStringWithoutSuffix(String str) {
        if (!str.startsWith("0x")) return str;
        return str.substring(str.indexOf("0x") + 2);
    }

    public void openWalletPinVerification() {
        if (mActivity == null) return;

        mPinDialog = new AlertDialog.Builder(mActivity, R.style.WalletDialogAnimation).create();
        mPinDialog.setCanceledOnTouchOutside(true);

        View container = mActivity.getLayoutInflater().inflate(R.layout.wallet_pin_popup, null);

        mPinDialog.show();
        mPinDialog.setContentView(container);

        mDot1 = container.findViewById(R.id.dot1);
        mDot2 = container.findViewById(R.id.dot2);
        mDot3 = container.findViewById(R.id.dot3);
        mDot4 = container.findViewById(R.id.dot4);
        mDot5 = container.findViewById(R.id.dot5);
        mDot6 = container.findViewById(R.id.dot6);

        View mDismissButton = container.findViewById(R.id.wallet_back_button);
        mDismissButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mPinDialog.dismiss();
                }
            });

        SharedPreferences mSharedPreferences = new EncryptSharedPreferences(mActivity).getSharedPreferences();
        String pinCode = mSharedPreferences.getString("PIN_CODE_KEY", "");

        Window dialogWindow = mPinDialog.getWindow();
        if (dialogWindow != null) {
            dialogWindow.setGravity(Gravity.BOTTOM);

            // Set the attributes to match parent width
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialogWindow.getAttributes());
            layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            dialogWindow.setAttributes(layoutParams);
        }

        mPinDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialogInterface) {
                    if (mDot1 != null) mDot1 = null;
                    if (mDot2 != null) mDot2 = null;
                    if (mDot3 != null) mDot3 = null;
                    if (mDot4 != null) mDot4 = null;
                    if (mDot5 != null) mDot5 = null;
                    if (mDot6 != null) mDot6 = null;
                    if (mPinDialog != null) mPinDialog = null;
                }
            });
    }

    // tits
    public void openWalletTransactionRequest() {
        if (mActivity == null || mLocationBarModel == null) return;

        AlertDialog mWalletInteractionDialog = new AlertDialog.Builder(mActivity, R.style.WalletDialogAnimation).create();
        mWalletInteractionDialog.setCanceledOnTouchOutside(true);

        transactionConfirmationContainer = mActivity.getLayoutInflater().inflate(R.layout.wallet_interaction_transaction, null);

        Tab tab = mLocationBarModel.getTab();

        int chainId = (authorizedHost !=  null && authorizedHost.equals(tab.getUrl().getHost()) && overrideChainId != -1) ? overrideChainId : 56;

        mWalletInteractionDialog.show();
        mWalletInteractionDialog.setContentView(transactionConfirmationContainer);

        String url = tab.getUrl().getHost();

        ProgressBar gasSpinner = transactionConfirmationContainer.findViewById(R.id.gas_value_loader);

        TextView gasPriceTextView = transactionConfirmationContainer.findViewById(R.id.gas_value);
        TextView dataTextView = transactionConfirmationContainer.findViewById(R.id.data_value);

        TextView chainNameTextView = transactionConfirmationContainer.findViewById(R.id.chain_name);
        chainNameTextView.setText(chainId == 56 ? "BNBChain Mainnet" : "Ethereum Mainnet");

        TextView siteUrl = transactionConfirmationContainer.findViewById(R.id.site_url);
        siteUrl.setText(url);

        TextView positiveButton = transactionConfirmationContainer.findViewById(R.id.button_positive);
        positiveButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                  openWalletPinVerification();
                  transactionConfirmationContainer = null;
                }
            });
        TextView negativeButton = transactionConfirmationContainer.findViewById(R.id.button_negative);
        negativeButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                  transactionConfirmationContainer = null;
                }
            });

        Button dataCopyButton = transactionConfirmationContainer.findViewById(R.id.data_button_copy);
        dataCopyButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
                    ClipData clip = ClipData.newPlainText("data", pendingWalletInteractionData);
                    clipboard.setPrimaryClip(clip);

                    Toast.makeText(mActivity, "Copied to clipboard" , Toast.LENGTH_SHORT).show();
                }
            });

        dataTextView.setText(pendingWalletInteractionData);

        LinearLayout gasEstimateContainer = transactionConfirmationContainer.findViewById(R.id.gas_price_container);

        // rounded_background_wallet_send_button
        if (!isLoadingGas) {
            positiveButton.setEnabled(true);
            positiveButton.setBackground(mActivity.getResources().getDrawable(R.drawable.rounded_background_wallet_send_button));
            positiveButton.setTextColor(Color.WHITE);

            gasSpinner.setVisibility(View.GONE);
            gasPriceTextView.setVisibility(View.VISIBLE);
            gasPriceTextView.setText(getFormattedGasPrice(pendingWalletInteractionPrice));

            gasEstimateContainer.setVisibility(View.VISIBLE);

            TextView gasEstimateTextView = gasEstimateContainer.findViewById(R.id.max_gas_value);
            gasEstimateTextView.setText(getSafeGasEstimate());
        }

        Window dialogWindow = mWalletInteractionDialog.getWindow();
        if (dialogWindow != null) {
            dialogWindow.setGravity(Gravity.BOTTOM);

            // Set the attributes to match parent width
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialogWindow.getAttributes());
            layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            dialogWindow.setAttributes(layoutParams);
        }
    }

    public void openWalletInteractionRequest() {
        if (mActivity == null || mLocationBarModel == null) return;

        AlertDialog mWalletInteractionDialog = new AlertDialog.Builder(mActivity, R.style.WalletDialogAnimation).create();
        mWalletInteractionDialog.setCanceledOnTouchOutside(true);

        View container = mActivity.getLayoutInflater().inflate(R.layout.wallet_interaction_account, null);

        mWalletInteractionDialog.show();
        mWalletInteractionDialog.setContentView(container);

        String url = mLocationBarModel.getTab().getUrl().getSpec();
        TextView siteUrl = container.findViewById(R.id.site_url);
        siteUrl.setText(url);

        TextView positiveButton = container.findViewById(R.id.button_positive);
        positiveButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                  openWalletPinVerification();
                }
            });
        TextView negativeButton = container.findViewById(R.id.button_negative);
        negativeButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                }
            });

        Window dialogWindow = mWalletInteractionDialog.getWindow();
        if (dialogWindow != null) {
            dialogWindow.setGravity(Gravity.BOTTOM);

            // Set the attributes to match parent width
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialogWindow.getAttributes());
            layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            dialogWindow.setAttributes(layoutParams);
        }
    }

    public void pinCodePressed(String code) {
        if (inputPinCode.length() == 6) return;

        inputPinCode = inputPinCode + code;
        onCodeChanged();
    }

    public void deletePinCodePressed() {
        if (inputPinCode.length() == 0) return;

        inputPinCode = inputPinCode.substring(0, inputPinCode.length() - 1);
        onCodeChanged();
    }

    private void doVibrate() {
       try {
         Vibrator vibrator = (Vibrator) mActivity.getSystemService(Context.VIBRATOR_SERVICE);

         if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
             // For newer versions
             VibrationEffect effect = VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE); // 1000 milliseconds = 1 second
             vibrator.vibrate(effect);
         } else {
             // For older versions
             vibrator.vibrate(100); // 1000 milliseconds = 1 second
         }
       } catch ( Exception e ) {}
    }

    private void onCodeChanged() {
      try {
          doVibrate();

         mDot1.setBackground(mActivity.getDrawable(inputPinCode.length() >= 1 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
         mDot2.setBackground(mActivity.getDrawable(inputPinCode.length() >= 2 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
         mDot3.setBackground(mActivity.getDrawable(inputPinCode.length() >= 3 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
         mDot4.setBackground(mActivity.getDrawable(inputPinCode.length() >= 4 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
         mDot5.setBackground(mActivity.getDrawable(inputPinCode.length() >= 5 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
         mDot6.setBackground(mActivity.getDrawable(inputPinCode.length() >= 6 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
      } catch (Exception ignore) { }

      try {
         if (inputPinCode.length() >=6) {
           final Handler handler = new Handler(Looper.getMainLooper());
           handler.postDelayed(new Runnable() {
               @Override
               public void run() {
                  processRequest();
               }
           }, 500);
         }
      } catch (Exception ignore) { }
    }

    private void getGasPrice(String url) {
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
                    conn.setRequestMethod("POST");
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
                  timeout.printStackTrace();
                    // Time out, don't set a background - lazy
                } catch (Exception e) {
                    isLoadingGas = false;
                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                isLoadingGas = false;
                try {
                    JSONObject jsonResult = new JSONObject(result);
                    pendingWalletInteractionPrice = jsonResult.getString("result");

                    TextView positiveButton = transactionConfirmationContainer.findViewById(R.id.button_positive);
                    ProgressBar gasSpinner = transactionConfirmationContainer.findViewById(R.id.gas_value_loader);
                    TextView gasPriceTextView = transactionConfirmationContainer.findViewById(R.id.gas_value);
                    LinearLayout gasEstimateContainer = transactionConfirmationContainer.findViewById(R.id.gas_price_container);

                    gasSpinner.setVisibility(View.GONE);
                    gasPriceTextView.setVisibility(View.VISIBLE);
                    gasPriceTextView.setText(getFormattedGasPrice(pendingWalletInteractionPrice));

                    gasEstimateContainer.setVisibility(View.VISIBLE);
                    TextView gasEstimateTextView = gasEstimateContainer.findViewById(R.id.max_gas_value);
                    gasEstimateTextView.setText(getSafeGasEstimate());

                    positiveButton.setEnabled(true);
                    positiveButton.setBackground(mActivity.getResources().getDrawable(R.drawable.rounded_background_wallet_send_button));
                    positiveButton.setTextColor(Color.WHITE);
                } catch (Exception e) {}
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private String getSafeGasEstimate() {
        try {
            BigInteger gasLimitBigInteger = new BigInteger(getStringWithoutSuffix(pendingWalletInteractionGas), 16);
            BigDecimal gasLimitBigDecimal = new BigDecimal(gasLimitBigInteger);

            BigDecimal gasPriceBigDecimal = new BigDecimal(getFormattedGasPrice(pendingWalletInteractionPrice));

            BigDecimal price = gasPriceBigDecimal.multiply(gasLimitBigDecimal);

            int chainId = overrideChainId != -1 ? overrideChainId : 56;

            String tokenPriceUsd = TokenDatabase.getInstance(mActivity).getTokenUSDValue(chainId != 56);
            BigDecimal maxUsdBigDecimal = new BigDecimal(tokenPriceUsd);
            maxUsdBigDecimal = maxUsdBigDecimal.multiply(price);
            maxUsdBigDecimal = maxUsdBigDecimal.setScale(5, RoundingMode.HALF_UP);

            return price.toPlainString() + " / $" + maxUsdBigDecimal.toPlainString();
        } catch (Exception e) {
          return "";
        }
    }

    private String getFormattedGasPrice(String amount) {
        String safeAmount = getStringWithoutSuffix(amount);

        BigDecimal wei = new BigDecimal("1000000000000000000");

        BigInteger amountBigInteger = new BigInteger(safeAmount, 16);

        BigDecimal amountBigDecimal = new BigDecimal(amountBigInteger);
        amountBigDecimal = amountBigDecimal.divide(wei);

        amountBigDecimal = amountBigDecimal.stripTrailingZeros();

        return amountBigDecimal.toPlainString();
    }

    private void processRequest() {
        //
        try {
           final SharedPreferences mSharedPreferences = new EncryptSharedPreferences(mActivity).getSharedPreferences();
           int attempts = mSharedPreferences.getInt("PIN_CODE_ATTEMPTS", 0);
           int timeLock = mSharedPreferences.getInt("PIN_CODE_LOCKED", 1);
           long timeSinceLastAttempt = System.currentTimeMillis() - mSharedPreferences.getLong("PIN_CODE_TIME_TRACK", 0);

           // lock for 30 mins, give 5 more attempts, double time, repeat
           long timeToLock = timeLock * 1800000 - timeSinceLastAttempt;

           boolean isLocked = attempts >= 5 && timeSinceLastAttempt <= timeToLock;

           String pinCode = mSharedPreferences.getString("PIN_CODE_KEY", "");

           if (inputPinCode.equals(pinCode) && !isLocked) {
               authorizedHost = mLocationBarModel.getTab().getUrl().getHost();

               mPinDialog.dismiss();


               String mnemonic = mSharedPreferences.getString("MNEMONIC_KEY", "");
               HDWallet mWallet = new HDWallet(mnemonic, "");

               String address = "";

               walletAddresses.clear();

               String ethereumAddress = mWallet.getAddressForCoin(CoinType.ETHEREUM);
               String smartchainAddress = mWallet.getAddressForCoin(CoinType.SMARTCHAIN);

               WalletDataObj ethObj = new WalletDataObj("ethereum", ethereumAddress);
               WalletDataObj bscObj = new WalletDataObj("smartchain", smartchainAddress);

               walletAddresses.add(ethObj);
               walletAddresses.add(bscObj);

               if (pendingWalletInteractionNetwork.equals("ethereum")) {
                 address = ethereumAddress;
               } else if (pendingWalletInteractionNetwork.equals("smartchain")) {
                 address = smartchainAddress;
               }

               Tab tab = mLocationBarModel.getTab();
               if (pendingWalletInteractionType == WalletInteractionEnum.WALLET_UNLOCK) {
                   String pendingWalletInteractionScript = "(function() {\n" +
                                   "window." + pendingWalletInteractionNetwork + ".setAddress(\"" + address + "\"); \n" +
                                   "window." + pendingWalletInteractionNetwork + ".sendResponse(" + pendingWalletInteractionId + ", [\"" + address + "\"]) \n" +
                                   "})();";

                   tab.getWebContents().evaluateJavaScript(pendingWalletInteractionScript, pendingWalletInteractionCallback);
               } else if (pendingWalletInteractionType == WalletInteractionEnum.WALLET_SWITCH_CHAIN) {
                   overrideChainId = pendingWalletInteractionChainId;

                   String pendingWalletInteractionScript = "(function() {\n" +
                                   "window." + pendingWalletInteractionNetwork + ".emitChainChanged(\"" + pendingWalletInteractionChainIdHex + "\") \n" +
                                   "window." + pendingWalletInteractionNetwork + ".setAddress(\"" + address + "\"); \n" +
                                   "window." + pendingWalletInteractionNetwork + ".sendResponse(" + pendingWalletInteractionId + ", [\"" + address + "\"]); \n" +
                                   "})();";

                   tab.getWebContents().evaluateJavaScript(pendingWalletInteractionScript, pendingWalletInteractionCallback);
               } else if (pendingWalletInteractionType == WalletInteractionEnum.WALLET_SEND_TRX) {
                   int chainId = overrideChainId != -1 ? overrideChainId : 56;
                   CoinType coinType = chainId == 56 ? CoinType.SMARTCHAIN : CoinType.ETHEREUM;

                   PrivateKey secretPrivateKey = mWallet.getKeyForCoin(coinType);
                   final String receiverAddress = pendingWalletInteractionTo;

                   final BigInteger gasLimitMax = new BigInteger(getStringWithoutSuffix(pendingWalletInteractionGas), 16);

                   Ethereum.SigningInput.Builder signerInputBuilder = Ethereum.SigningInput.newBuilder();

                   final String ticker = chainId == 56 ? "BSC" : "ETH";

                   String nonce = TokenDatabase.getInstance(mActivity).getTokenNonce(ticker);
                   BigInteger nonceToUse;
                   if (nonce.length() == 1 && Character.isDigit(nonce.charAt(0))) {
                       nonceToUse = new BigInteger(nonce);
                   } else {
                       nonceToUse = new BigInteger(nonce, 16);
                   }

                   BigInteger gasPriceBigInteger;
                   if (pendingWalletInteractionPrice == null) {
                       gasPriceBigInteger = new BigInteger("b2d05e00", 16);
                   } else {
                       gasPriceBigInteger = new BigInteger(getStringWithoutSuffix(pendingWalletInteractionPrice), 16);
                   }

                   signerInputBuilder.setChainId(ByteString.copyFrom((new BigInteger(chainId+"").toByteArray())));
                   signerInputBuilder.setGasPrice(toByteString(gasPriceBigInteger));
                   signerInputBuilder.setGasLimit(toByteString(gasLimitMax));
                   signerInputBuilder.setNonce(toByteString(nonceToUse));
                   signerInputBuilder.setPrivateKey(ByteString.copyFrom(secretPrivateKey.data()));

                   // BEP/ERC20
                   signerInputBuilder.setToAddress(pendingWalletInteractionTo); // contract address

                   Ethereum.Transaction.Builder ethTrxBuilder = Ethereum.Transaction.newBuilder();

                   Ethereum.Transaction.Transfer.Builder ethTrxTransferBuilder = Ethereum.Transaction.Transfer.newBuilder();
                   if (pendingWalletInteractionValue != null && !pendingWalletInteractionValue.equals("")) {
                      ethTrxTransferBuilder.setAmount(ByteString.copyFrom(new BigInteger(getStringWithoutSuffix(pendingWalletInteractionValue), 16).toByteArray()));
                   }
                   ethTrxTransferBuilder.setData(ByteString.copyFrom(hexStringToByteArray(pendingWalletInteractionData)));

                   ethTrxBuilder.setTransfer(ethTrxTransferBuilder.build());

                   signerInputBuilder.setTransaction(ethTrxBuilder.build());

                   Ethereum.SigningInput signerInput = signerInputBuilder.build();

                   Ethereum.SigningOutput signerOutput = (Ethereum.SigningOutput)AnySigner.sign((Message)signerInput, coinType, Ethereum.SigningOutput.parser());

                   secretPrivateKey = null;

                   String url = "";
                   if (chainId == 56) {
                       url = "https://go.getblock.io/5855ef8cc8c04dfd808817090d98dd7c";
                   } else {
                       url = "https://go.getblock.io/56d1ba70818d40a19f936b8a0bffcece";
                   }

                   String body = "{"
                       + "\"jsonrpc\": \"2.0\","
                       + "\"method\": \"eth_sendRawTransaction\","
                       + "\"params\": ["
                       + "\"" + toHexString(signerOutput.getEncoded().toByteArray(), true) + "\""
                       + "],"
                       + "\"id\": \"getblock.io\""
                       + "}";

                   shouldResetPendingInteraction = false;

                   sendTransaction(url, body);
               }

               mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();
               mSharedPreferences.edit().putLong("PIN_CODE_TIME_TRACK", 0).commit();
               mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();

               mnemonic = "";
               mWallet = null;
           } else {
               if (timeSinceLastAttempt > timeToLock || attempts < 4) {
                   if (timeSinceLastAttempt > timeToLock && attempts > 4) {
                     attempts = 0;
                     mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();
                   }

                   mSharedPreferences.edit().putLong("PIN_CODE_TIME_TRACK", System.currentTimeMillis()).commit();

                   Toast.makeText(mActivity, ("Incorrect pin, try again. " + (5 - (attempts + 1)) + " attempts remaining.") , Toast.LENGTH_SHORT).show();

                   mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", attempts + 1).commit();
               } else {
                   Toast.makeText(mActivity, ("Wallet locked. Try again in " + timeConversion(timeToLock)) , Toast.LENGTH_SHORT).show();
               }
           }

           pinCode = "";
        } catch (Exception ignore) { }

        inputPinCode = "";
        onCodeChanged();

        if (shouldResetPendingInteraction) {
            pendingWalletInteractionNetwork = null;
            pendingWalletInteractionId = -1;
            pendingWalletInteractionCallback = null;
            pendingWalletInteractionType = null;
            pendingWalletInteractionData = null;
            pendingWalletInteractionFrom = null;
            pendingWalletInteractionGas = null;
            pendingWalletInteractionPrice = null;
            pendingWalletInteractionTo = null;
            pendingWalletInteractionValue = null;
        }
    }

    private byte[] keccak256(byte[] data) {
        Keccak.Digest256 digest = new Keccak.Digest256();
        return digest.digest(data);
    }

    public String encodeTransferData(String address, String amount) throws NoSuchAlgorithmException {
        String functionSignature = "transfer(address,uint256)";
        byte[] signatureHash = keccak256(functionSignature.getBytes(StandardCharsets.UTF_8));

        String addressPadded = padLeft(address.replace("0x", ""), 64);
        String amountPadded = padLeft(amount, 64);

        return bytesToHex(signatureHash).substring(0, 8) + addressPadded + amountPadded;
    }

    public String padLeft(String s, int n) {
        return String.format("%1$" + n + "s", s).replace(' ', '0');
    }

    private String bytesToHex(byte[] bytes) {
        StringBuilder hexString = new StringBuilder(2 * bytes.length);
        for (byte b : bytes) {
            String hex = Integer.toHexString(0xff & b);
            if (hex.length() == 1) {
                hexString.append('0');
            }
            hexString.append(hex);
        }
        return hexString.toString();
    }

    public static byte[] hexStringToByteArray(String input) {
        String cleanInput = cleanHexPrefix(input);

        int len = cleanInput.length();

        if (len == 0) {
            return new byte[0];
        }

        byte[] data;
        int startIdx;
        if (len % 2 != 0) {
            data = new byte[len / 2 + 1];
            data[0] = (byte) Character.digit(cleanInput.charAt(0), 16);
            startIdx = 1;
        } else {
            data = new byte[len / 2];
            startIdx = 0;
        }

        for (int i = startIdx; i < len; i += 2) {
            data[(i + 1) / 2] = (byte) ((Character.digit(cleanInput.charAt(i), 16) << 4)
                                        + Character.digit(cleanInput.charAt(i + 1), 16));
        }
        return data;
    }

    public static boolean containsHexPrefix(String input) {
        return input.length() > 1 && input.charAt(0) == '0' && input.charAt(1) == 'x';
    }

    public static String cleanHexPrefix(String input) {
        if (containsHexPrefix(input)) {
            return input.substring(2);
        } else {
            return input;
        }
    }

    public void sendTransaction(String url, String body) {
      new AsyncTask<String>() {
          @Override
          protected String doInBackground() {
              HttpURLConnection conn = null;
              StringBuffer response = new StringBuffer();
              try {
                  URL mUrl = new URL(url);

                  conn = (HttpURLConnection) mUrl.openConnection();
                  conn.setDoOutput(true);
                  conn.setConnectTimeout(20000);
                  conn.setDoInput(true);
                  conn.setUseCaches(false);
                  conn.setRequestMethod("POST");
                  conn.setRequestProperty("Content-Type", "application/json");

                  byte[] outputInBytes = body.getBytes("UTF-8");
                  OutputStream os = conn.getOutputStream();
                  os.write( outputInBytes );
                  os.close();

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
                timeout.printStackTrace();
                  // Time out, don't set a background - lazy
              } catch (Exception e) {

              } finally {
                  if (conn != null)
                      conn.disconnect();
              }

              return response.toString();
          }

          @Override
          protected void onPostExecute(String result) {
              try {
                  shouldResetPendingInteraction = true;
                  Tab tab = mLocationBarModel.getTab();

                  JSONObject jsonResult = new JSONObject(result);

                  // String pendingWalletInteractionScript = "(function() {\n" +
                  //                 "window." + pendingWalletInteractionNetwork + ".sendResponse(" + pendingWalletInteractionId + ", [\"" + jsonResult.getString("result") + "\"]); \n" +
                  //                 "})();";
                  //
                  // tab.getWebContents().evaluateJavaScript(pendingWalletInteractionScript, pendingWalletInteractionCallback);

                  openTrxConfirmation(jsonResult.getString("result"));

                  tab.reload();

                  // update nonce
                  int chainId = (authorizedHost !=  null && authorizedHost.equals(tab.getUrl().getHost()) && overrideChainId != -1) ? overrideChainId : 56;
                  final String ticker = chainId == 56 ? "BSC" : "ETH";
                  String nonce = TokenDatabase.getInstance(mActivity).getTokenNonce(ticker);

                  boolean shouldStoreAsHex = false;
                  BigInteger nonceBigInteger;
                  if (nonce.length() == 1 && Character.isDigit(nonce.charAt(0))) {
                      nonceBigInteger = new BigInteger(nonce);
                      if (nonceBigInteger.compareTo(BigInteger.valueOf(9)) <= 0) {
                          shouldStoreAsHex = false;
                      } else {
                          shouldStoreAsHex = true;
                      }
                  } else {
                      nonceBigInteger = new BigInteger(nonce, 16);
                      shouldStoreAsHex = true;
                  }
                  nonceBigInteger = nonceBigInteger.add(BigInteger.ONE);
                  TokenDatabase.getInstance(mActivity).setTokenNonce(ticker, shouldStoreAsHex ? nonceBigInteger.toString(16) : nonceBigInteger.toString());
              } catch (Exception e) {
                showTransactionError(result);
              }

              pendingWalletInteractionNetwork = null;
              pendingWalletInteractionId = -1;
              pendingWalletInteractionCallback = null;
              pendingWalletInteractionType = null;
              pendingWalletInteractionData = null;
              pendingWalletInteractionFrom = null;
              pendingWalletInteractionGas = null;
              pendingWalletInteractionPrice = null;
              pendingWalletInteractionTo = null;
              pendingWalletInteractionValue = null;
          }
      }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void showTransactionError(String result) {
        if (mActivity == null) return;

        AlertDialog mWalletInteractionDialog = new AlertDialog.Builder(mActivity, R.style.WalletDialogAnimation).create();
        mWalletInteractionDialog.setCanceledOnTouchOutside(true);

        View container = mActivity.getLayoutInflater().inflate(R.layout.wallet_interaction_transaction_error, null);

        mWalletInteractionDialog.show();
        mWalletInteractionDialog.setContentView(container);

        TextView errorText = container.findViewById(R.id.error_text);
        errorText.setText(result);

        TextView positiveButton = container.findViewById(R.id.button_positive);
        positiveButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                }
            });

        Window dialogWindow = mWalletInteractionDialog.getWindow();
        if (dialogWindow != null) {
            dialogWindow.setGravity(Gravity.BOTTOM);

            // Set the attributes to match parent width
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialogWindow.getAttributes());
            layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            dialogWindow.setAttributes(layoutParams);
        }
    }

    private void openTrxConfirmation(String id) {
        if (mActivity == null || mLocationBarModel == null) return;

        Tab tab = mLocationBarModel.getTab();

        int chainId = (authorizedHost !=  null && authorizedHost.equals(tab.getUrl().getHost()) && overrideChainId != -1) ? overrideChainId : 56;

        AlertDialog mWalletInteractionDialog = new AlertDialog.Builder(mActivity, R.style.WalletDialogAnimation).create();
        mWalletInteractionDialog.setCanceledOnTouchOutside(true);

        View container = mActivity.getLayoutInflater().inflate(R.layout.wallet_interaction_transaction_confirmation, null);

        mWalletInteractionDialog.show();
        mWalletInteractionDialog.setContentView(container);

        String url = tab.getUrl().getHost();

        TextView positiveButton = container.findViewById(R.id.button_positive);
        positiveButton.setText("OPEN IN " +(chainId == 56 ? "BSCSCAN" : "ETHERSCAN"));
        positiveButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                  String url = chainId != 56 ? "https://etherscan.io/tx/" : "https://bscscan.com/tx/";
                  url = url + id;
                  loadUrl(url, v);
                }
            });
        TextView negativeButton = container.findViewById(R.id.button_negative);
        negativeButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  mWalletInteractionDialog.dismiss();
                }
            });

        Window dialogWindow = mWalletInteractionDialog.getWindow();
        if (dialogWindow != null) {
            dialogWindow.setGravity(Gravity.BOTTOM);

            // Set the attributes to match parent width
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialogWindow.getAttributes());
            layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            dialogWindow.setAttributes(layoutParams);
        }
    }

    public String toHexString(byte[] data, boolean withPrefix) {
        StringBuilder stringBuilder = new StringBuilder();
        if (withPrefix) {
            stringBuilder.append("0x");
        }
        for (byte element : data) {
            stringBuilder.append(String.format("%02x", element & 0xFF));
        }

        return stringBuilder.toString();
    }

    private ByteString toByteString(BigInteger bigInteger) {
        return ByteString.copyFrom(bigInteger.toByteArray());
    }

    public String timeConversion(Long millie) {
       if (millie != null) {
           long seconds = (millie / 1000);
           long sec = seconds % 60;
           long min = (seconds / 60) % 60;
           long hrs = (seconds / (60 * 60)) % 24;
           if (hrs > 0) {
               return String.format("%02d:%02d:%02d", hrs, min, sec);
           } else {
               return String.format("%02d:%02d", min, sec);
           }
       } else {
           return null;
       }
    }

    /**
     * Set container view on which GTS toolbar needs to inflate.
     * @param containerView view containing GTS fullscreen toolbar.
     */
    public void setTabSwitcherFullScreenView(ViewGroup containerView) {
        ViewStub toolbarStub =
                containerView.findViewById(R.id.fullscreen_tab_switcher_toolbar_stub);
        mToolbar.setFullScreenToolbarStub(toolbarStub);
    }

    /**
     * Handle a layout change event.
     * @param layoutType The layout being switched to.
     * @param showToolbar Whether the toolbar should be shown.
     */
    private void updateForLayout(@LayoutType int layoutType, boolean showToolbar) {
        if (layoutType == LayoutType.TAB_SWITCHER) {
            mLocationBarModel.setIsShowingTabSwitcher(true);
            mToolbar.setTabSwitcherMode(true, showToolbar, false);
            updateButtonStatus();
            if (mLocationBarModel.shouldShowLocationBarInOverviewMode()) {
                assert mLocationBar instanceof LocationBarCoordinator;
                ((LocationBarCoordinator) mLocationBar).startAutocompletePrefetch();
            }
        }
        mToolbar.setContentAttached(layoutType == LayoutType.BROWSING);
    }

    @Override
    public void loadRewardsUrl(String url, View view) {
        try {
          LoadUrlParams loadUrlParams = new LoadUrlParams(url);
          ChromeActivity activity = (ChromeActivity) view.getContext();
          activity.getActivityTab().loadUrl(loadUrlParams);
        } catch (Exception ignore) {}
    }

    private TopToolbarCoordinator createTopToolbarCoordinator(
            ToolbarControlContainer controlContainer, ToolbarLayout toolbarLayout,
            List<ButtonDataProvider> buttonDataProviders,
            ThemeColorProvider browsingModeThemeColorProvider, Invalidator invalidator,
            IdentityDiscController identityDiscController, boolean isGridTabSwitcherEnabled,
            boolean isTabletGtsPolishEnabled, boolean isTabToGtsAnimationEnabled,
            boolean isStartSurfaceEnabled, boolean isTabGroupsAndroidContinuationEnabled,
            boolean initializeWithIncognitoColors, ObservableSupplier<Profile> profileSupplier,
            Callback<LoadUrlParams> logoClickedCallback) {
        ViewStub tabSwitcherToolbarStub = mActivity.findViewById(R.id.tab_switcher_toolbar_stub);
        ViewStub tabSwitcherFullscreenToolbarStub = null;

        // clang-format off
        TopToolbarCoordinator toolbar = new TopToolbarCoordinator(controlContainer,
                tabSwitcherToolbarStub, tabSwitcherFullscreenToolbarStub, toolbarLayout,
                mLocationBarModel, mToolbarTabController,
                new UserEducationHelper(mActivity, mHandler), buttonDataProviders,
                mLayoutStateProviderSupplier, browsingModeThemeColorProvider,
                mAppThemeColorProvider, mMenuButtonCoordinator, mOverviewModeMenuButtonCoordinator,
                mMenuButtonCoordinator.getMenuButtonHelperSupplier(), mTabModelSelectorSupplier,
                mHomepageEnabledSupplier,
                mIdentityDiscStateSupplier, (client) -> {
                    if (invalidator != null) {
                        invalidator.invalidate(client);
                    } else {
                        client.run();
                    }
                }, () -> identityDiscController.getForStartSurface(mStartSurfaceState),
                mCompositorViewHolder::getResourceManager,
                mIsProgressBarVisibleSupplier, IncognitoUtils::isIncognitoModeEnabled,
                isGridTabSwitcherEnabled, isTabletGtsPolishEnabled, isTabToGtsAnimationEnabled,
                isStartSurfaceEnabled, isTabGroupsAndroidContinuationEnabled,
                HistoryManagerUtils::showHistoryManager,
                PartnerBrowserCustomizations.getInstance()::isHomepageProviderAvailableAndEnabled,
                DownloadUtils::downloadOfflinePage, initializeWithIncognitoColors, profileSupplier,
                logoClickedCallback);
        // clang-format on
        mHomepageStateListener = () -> {
            Boolean wasHomepageEnabled = mHomepageEnabledSupplier.get();
            boolean isHomepageEnabled = HomepageManager.isHomepageEnabled();
            mHomepageEnabledSupplier.set(isHomepageEnabled);

            // Whether to show start surface as homepage is affected by whether homepage URI is
            // customized. So we add a supplier to observe homepage URI change.
            Boolean wasStartSurfaceAsHomepage = mStartSurfaceAsHomepageSupplier.get();
            boolean isStartSurfaceAsHomepage =
                    ReturnToChromeUtil.shouldShowStartSurfaceAsTheHomePage(mActivity);
            mStartSurfaceAsHomepageSupplier.set(isStartSurfaceAsHomepage);
            mHomepageManagedByPolicySupplier.set(HomepagePolicyManager.isHomepageManagedByPolicy());

            // We need to skip recreating CTA the first time when mStartSurfaceAsHomepageSupplier
            // is set, i.e., mStartSurfaceAsHomepageSupplier.get() is changed from null to
            // true/false. Otherwise, it will cause infinite loop of calling recreate().
            if (wasStartSurfaceAsHomepage == null || wasHomepageEnabled == null) return;

            if (mIsStartSurfaceEnabled != ReturnToChromeUtil.isStartSurfaceEnabled(mActivity)) {
                // If the state of whether Start surface is enabled is changed due to the homepage
                // settings change, we need to recreate CTA to adopt this config change.
                recreateActivityWithTabReparenting();
            }
        };
        HomepageManager.getInstance().addListener(mHomepageStateListener);
        mHomepageStateListener.onHomepageStateUpdated();

        if (toolbarLayout instanceof ToolbarPhone
                && ReturnToChromeUtil.isStartSurfaceEnabled(mActivity)) {
            identityDiscController.addObserver(
                    (canShowHint) -> mIdentityDiscStateSupplier.set(canShowHint));
        }
        HomeButton homeButton = toolbarLayout.getHomeButton();
        if (homeButton != null) {
            homeButton.init(mHomepageEnabledSupplier, HomepageManager.getInstance()::onMenuClick,
                    mHomepageManagedByPolicySupplier);
        }
        return toolbar;
    }

    /**
     * Recreates the activity with Tab reparenting, allowing fast restart without reloading Tabs'
     * contents.
     */
    private void recreateActivityWithTabReparenting() {
        if (mTabReparentingControllerSupplier.get() != null) {
            mTabReparentingControllerSupplier.get().prepareTabsForReparenting();
        }
        mActivity.recreate();
    }

    // Base abstract implementation of NewTabPageDelegate for phone/table toolbar layout.
    private abstract class ToolbarNtpDelegate implements NewTabPageDelegate {
        protected NewTabPage mVisibleNtp;

        @Override
        public boolean wasShowingNtp() {
            return mVisibleNtp != null;
        }

        @Override
        public boolean isCurrentlyVisible() {
            return getNewTabPageForCurrentTab() != null;
        }

        @Override
        public boolean dispatchTouchEvent(MotionEvent ev) {
            assert mVisibleNtp != null;
            // No null check -- the toolbar should not be moved if we are not on an NTP.
            return mVisibleNtp.getView().dispatchTouchEvent(ev);
        }

        @Override
        public boolean isLocationBarShown() {
            // Without this check, ToolbarPhone#computeVisualState may return
            // VisualState.NEW_TAB_NORMAL even if it's in start surface homepage, which leads
            // ToolbarPhone#getToolbarColorForVisualState to return transparent color.
            if (ReturnToChromeUtil.isStartSurfaceEnabled(mActivity)
                    && mStartSurfaceState == StartSurfaceState.SHOWN_HOMEPAGE) {
                return false;
            }
            NewTabPage ntp = getNewTabPageForCurrentTab();
            return ntp != null && ntp.isLocationBarShownInNTP();
        }

        @Override
        public boolean transitioningAwayFromLocationBar() {
            return mVisibleNtp != null && mVisibleNtp.isLocationBarShownInNTP()
                    && !isLocationBarShown();
        }

        @Override
        public void setSearchBoxScrollListener(Callback<Float> scrollCallback) {
            NewTabPage newVisibleNtp = getNewTabPageForCurrentTab();
            if (mVisibleNtp != null) mVisibleNtp.setSearchBoxScrollListener(null);
            mVisibleNtp = newVisibleNtp;
            if (mVisibleNtp != null && shouldUpdateListener()) {
                mVisibleNtp.setSearchBoxScrollListener(
                        (fraction) -> scrollCallback.onResult(fraction));
            }
        }

        // Boolean predicate that tells if the NewTabPage.OnSearchBoxScrollListener
        // should be updated or not
        protected abstract boolean shouldUpdateListener();

        @Override
        public void getSearchBoxBounds(Rect bounds, Point translation) {
            assert getNewTabPageForCurrentTab() != null;
            getNewTabPageForCurrentTab().getSearchBoxBounds(bounds, translation);
        }

        @Override
        public void setSearchBoxBackground(Drawable drawable) {
            assert getNewTabPageForCurrentTab() != null;
            getNewTabPageForCurrentTab().setSearchBoxBackground(drawable);
        }

        @Override
        public void setSearchBoxAlpha(float alpha) {
            assert getNewTabPageForCurrentTab() != null;
            getNewTabPageForCurrentTab().setSearchBoxAlpha(alpha);
        }

        @Override
        public void setSearchProviderLogoAlpha(float alpha) {
            assert getNewTabPageForCurrentTab() != null;
            getNewTabPageForCurrentTab().setSearchProviderLogoAlpha(alpha);
        }

        @Override
        public void setUrlFocusChangeAnimationPercent(float fraction) {
            NewTabPage ntp = getNewTabPageForCurrentTab();
            if (ntp != null) ntp.setUrlFocusChangeAnimationPercent(fraction);
        }
    }

    private NewTabPageDelegate createNewTabPageDelegate(ToolbarLayout toolbarLayout) {
        if (toolbarLayout instanceof ToolbarPhone) {
            return new ToolbarNtpDelegate() {
                @Override
                protected boolean shouldUpdateListener() {
                    return mVisibleNtp.isLocationBarShownInNTP();
                }
            };
        } else if (toolbarLayout instanceof ToolbarTablet) {
            return new ToolbarNtpDelegate() {
                @Override
                public void setSearchBoxScrollListener(Callback<Float> scrollCallback) {
                    if (mVisibleNtp == getNewTabPageForCurrentTab()) return;
                    super.setSearchBoxScrollListener(scrollCallback);
                }

                @Override
                protected boolean shouldUpdateListener() {
                    return true;
                }
            };
        }
        return NewTabPageDelegate.EMPTY;
    }

    private NewTabPage getNewTabPageForCurrentTab() {
        if (mLocationBarModel.hasTab()) {
            NativePage nativePage = mLocationBarModel.getTab().getNativePage();
            if (nativePage instanceof NewTabPage) return (NewTabPage) nativePage;
        }
        return null;
    }

    /**
     * @return  Whether the UrlBar currently has focus.
     */
    public boolean isUrlBarFocused() {
        if (mLocationBar.getOmniboxStub() == null) {
            return false;
        }
        return mLocationBar.getOmniboxStub().isUrlBarFocused();
    }

    /**
     * Enable the bottom controls.
     */
    public void enableBottomControls() {
        View root = ((ViewStub) mActivity.findViewById(R.id.bottom_controls_stub)).inflate();
        root.setBackgroundColor(Color.TRANSPARENT);
        mTabGroupUi = TabManagementModuleProvider.getDelegate().createTabGroupUi(mActivity,
                root.findViewById(R.id.bottom_container_slot), mIncognitoStateProvider,
                mScrimCoordinator, mOmniboxFocusStateSupplier, mBottomSheetController,
                mActivityLifecycleDispatcher, mIsWarmOnResumeSupplier, mTabModelSelector,
                mTabContentManager, mCompositorViewHolder,
                mCompositorViewHolder::getDynamicResourceLoader, mTabCreatorManager,
                mShareDelegateSupplier, mLayoutStateProviderSupplier, mSnackbarManager);

        mBottomControlsCoordinator = new BottomControlsCoordinator(mActivity, mWindowAndroid, mLayoutManager,
                mCompositorViewHolder.getResourceManager(), mBrowserControlsSizer,
                mFullscreenManager, (ScrollingBottomViewResourceFrameLayout) root,
                mTabGroupUi, mOverlayPanelVisibilitySupplier,
                this, mTabSwitcherClickHandler, mTabModelSelector, mTabCountProvider);
        mBottomControlsCoordinatorSupplier.set(mBottomControlsCoordinator);

        onTintChanged(mTempColorStateList, mTempBrandedColorScheme);
        onThemeColorChanged(mTempColor, false);
    }

    /**
     * TODO(https://crbug.com/1164216): Remove this getter in favor of extracting tab group
     * feature details from ChromeTabbedActivity directly.
     * @return The coordinator for the tab group UI if it exists.
     */
    public TabGroupUi getTabGroupUi() {
        return mTabGroupUi;
    }

    /**
     * Initialize the manager with the components that had native initialization dependencies.
     * <p>
     * Calling this must occur after the native library have completely loaded.
     *
     * @param layoutManager A {@link LayoutManagerImpl} instance used to watch for scene
     *                      changes.
     * @param tabSwitcherClickHandler The {@link OnClickListener} for the tab switcher button.
     * @param newTabClickHandler The {@link OnClickListener} for the new tab button.
     * @param bookmarkClickHandler The {@link OnClickListener} for the bookmark button.
     * @param customTabsBackClickHandler The {@link OnClickListener} for the custom tabs back
     *         button.
     * @param showStartSurfaceSupplier Supplies if we should show the start surface.
     */
    public void initializeWithNative(LayoutManagerImpl layoutManager,
            OnClickListener tabSwitcherClickHandler, OnClickListener newTabClickHandler,
            OnClickListener bookmarkClickHandler, OnClickListener customTabsBackClickHandler,
            Supplier<Boolean> showStartSurfaceSupplier) {
        TraceEvent.begin("ToolbarManager.initializeWithNative");
        assert !mInitializedWithNative;
        assert mTabModelSelectorSupplier.get() != null;

        mTabModelSelector = mTabModelSelectorSupplier.get();
        mShowStartSurfaceSupplier = showStartSurfaceSupplier;

        mTabSwitcherClickHandler = tabSwitcherClickHandler;

        mToolbar.initializeWithNative(layoutManager::requestUpdate, tabSwitcherClickHandler,
                newTabClickHandler, bookmarkClickHandler, customTabsBackClickHandler,
                mAppMenuDelegate, layoutManager, mActivityTabProvider, mBrowserControlsSizer,
                mTopUiThemeColorProvider);

        mToolbar.addOnAttachStateChangeListener(new OnAttachStateChangeListener() {
            @Override
            public void onViewDetachedFromWindow(View v) {}

            @Override
            public void onViewAttachedToWindow(View v) {
                // As we have only just registered for notifications, any that were sent prior
                // to this may have been missed. Calling refreshSelectedTab in case we missed
                // the initial selection notification.
                refreshSelectedTab(mActivityTabProvider.get());
            }
        });

        mLocationBarModel.initializeWithNative();

        if (layoutManager != null) {
            mLayoutManager = layoutManager;
            mLayoutManager.getOverlayPanelManager().addObserver(mOverlayPanelManagerObserver);
        }

        if (mMenuStateObserver != null) {
            UpdateMenuItemHelper.getInstance().registerObserver(mMenuStateObserver);
        }

        if (mStartSurfaceMenuStateObserver != null) {
            UpdateMenuItemHelper.getInstance().registerObserver(mStartSurfaceMenuStateObserver);
        }

        TemplateUrlServiceFactory.get().runWhenLoaded(this::registerTemplateUrlObserver);
        mInitializedWithNative = true;
        mTabModelSelector.addObserver(mTabModelSelectorObserver);
        refreshSelectedTab(mActivityTabProvider.get());
        if (mTabModelSelector.isTabStateInitialized()) mTabRestoreCompleted = true;
        handleTabRestoreCompleted();
        mTabCountProvider.setTabModelSelector(mTabModelSelector);
        mIncognitoStateProvider.setTabModelSelector(mTabModelSelector);
        mAppThemeColorProvider.setIncognitoStateProvider(mIncognitoStateProvider);

        if (mOnInitializedRunnable != null) {
            mOnInitializedRunnable.run();
            mOnInitializedRunnable = null;
        }

        // Allow bitmap capturing once everything has been initialized.
        Tab currentTab = mTabModelSelector.getCurrentTab();
        if (currentTab != null && currentTab.getWebContents() != null
                && !currentTab.getUrl().isEmpty()) {
            mControlContainer.setReadyForBitmapCapture(true);
        }

        // mRewardsBridge = RewardsAPIBridge.getInstance();

        // ChromeImageButton mRewardsButton = mControlContainer.findViewById(R.id.rewards_button);
        // if (mRewardsButton != null) {
        //     mRewardsButton.setOnClickListener(new View.OnClickListener() {
        //         @Override
        //         public void onClick(View v) {
        //             showRewardsPopup(v);
        //         }
        //     });
        // }

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.TOOLBAR_IPH_ANDROID)) {
            UserEducationHelper userEducationHelper = new UserEducationHelper(mActivity, mHandler);
            View homeButton = mControlContainer.findViewById(R.id.home_button);
            mHomeButtonCoordinator = new HomeButtonCoordinator(mActivity, homeButton,
                    userEducationHelper, mIncognitoStateProvider::isIncognitoSelected,
                    mIntentMetadataOneshotSupplier, mPromoShownOneshotSupplier,
                    HomepageManager::isHomepageNonNtp, FeedFeatures::isFeedEnabled,
                    mActivityTabProvider);
            ToggleTabStackButton toggleTabStackButton =
                    mControlContainer.findViewById(R.id.tab_switcher_button);
            mToggleTabStackButtonCoordinator = new ToggleTabStackButtonCoordinator(mActivity,
                    toggleTabStackButton, userEducationHelper,
                    mIncognitoStateProvider::isIncognitoSelected, mIntentMetadataOneshotSupplier,
                    mPromoShownOneshotSupplier, mLayoutStateProviderSupplier,
                    mToolbar::setNewTabButtonHighlight, mActivityTabProvider);
        }
        TraceEvent.end("ToolbarManager.initializeWithNative");
    }

    // @Override
    // public void onSpeedDialClicked() {
    //     if (mSpeedDialDialog != null) {
    //         mSpeedDialDialog.cancel();
    //     }
    // }

    @Override
    public void loadUrl(String url, View v) {
        try {
          LoadUrlParams loadUrlParams = new LoadUrlParams(url);
          ChromeActivity activity = (ChromeActivity)v.getContext();
          activity.getActivityTab().loadUrl(loadUrlParams);
        } catch (Exception ignore) {}
    }

    @Override
    public void loadHomepage(View v) {
        try {
          LoadUrlParams loadUrlParams = new LoadUrlParams(homepageUrl());
          ChromeActivity activity = (ChromeActivity)v.getContext();
          activity.getActivityTab().loadUrl(loadUrlParams);
        } catch (Exception ignore) {}
    }

    @Override
    public void openSettings(View v) {
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        settingsLauncher.launchSettingsActivity(v.getContext());
    }

    @Override
    public void openSpeedDialPopup(View v) { }

    // @Override
    // public void openSpeedDialPopup(View v) {
    //     mSpeedDialDialog = new AlertDialog.Builder(v.getContext(), R.style.SpeedDialDialogAnimation).create();
    //     mSpeedDialDialog.setCanceledOnTouchOutside(true);
    //
    //     FrameLayout container = new FrameLayout(v.getContext());
    //
    //     mSpeedDialDialog.getWindow().setGravity(Gravity.BOTTOM);
    //
    //     mSpeedDialDialog.show();
    //
    //     mSpeedDialDialog.setContentView(container);
    //
    //     mSpeedDialDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    //
    //     container.setMinimumHeight(v.getResources().getDimensionPixelSize(R.dimen.min_popup_dialog_height));
    //
    //     container.setBackgroundDrawable(ApiCompatibilityUtils.getDrawable(
    //             v.getResources(), R.drawable.popup_bg_tinted_color));
    //
    //     SpeedDialGridView mSpeedDialView = SpeedDialController.inflateSpeedDial(container.getContext(), 4,
    //             ((SpeedDialInteraction)this), true);
    //
    //     if (mShouldForceTintRefresh) mSpeedDialView.setDark();
    //     if(mSpeedDialView.getParent() != null) ((ViewGroup)mSpeedDialView.getParent()).removeView(mSpeedDialView);
    //     container.addView(mSpeedDialView);
    //
    //     ViewGroup.LayoutParams layoutParams = mSpeedDialView.getLayoutParams();
    //     layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
    //     mSpeedDialView.setLayoutParams(layoutParams);
    //     //
    //
    //     // center
    //     FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)mSpeedDialView.getLayoutParams();
    //     lp.gravity = Gravity.CENTER;
    //     mSpeedDialView.setLayoutParams(lp);
    //
    //
    //     // free up alert dialog reference for garbage collector
    //     mSpeedDialDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
    //         @Override
    //         public void onDismiss(DialogInterface dialogInterface) {
    //             if (mSpeedDialDialog != null) mSpeedDialDialog = null;
    //         }
    //     });
    //
    //     mSpeedDialDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
    //         @Override
    //         public void onCancel(DialogInterface dialogInterface) {
    //             if (mSpeedDialDialog != null) mSpeedDialDialog = null;
    //         }
    //     });
    // }

    @Override
    public void openRewardsPopup(View v) {
        showRewardsPopup(v);
    }

    @Override
    public void focusUrlBar() {
        mHandler.post(() -> setUrlBarFocus(true, OmniboxFocusReason.OMNIBOX_TAP));
        if (shouldShowCursorInLocationBar()) {
            mLocationBar.showUrlBarCursorWithoutFocusAnimations();
        }
        updateButtonStatus();
    }

    private boolean shouldShowCursorInLocationBar() {
        Tab tab = mLocationBarModel.getTab();
        if (tab == null) return false;
        NativePage nativePage = tab.getNativePage();
        if (!(nativePage instanceof NewTabPage) && !(nativePage instanceof IncognitoNewTabPage)) {
            return false;
        }

        return DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity)
                && mActivity.getResources().getConfiguration().keyboard
                == Configuration.KEYBOARD_QWERTY;
    }

    private void showRewardsPopup(View v) {
        mRewardsBottomSheetCoordinator.show();
    }

    /**
     * @return The toolbar interface that this manager handles.
     */
    public Toolbar getToolbar() {
        return mToolbar;
    }

    @Override
    public @Nullable View getMenuButtonView() {
        return mMenuButtonCoordinator.getMenuButton().getImageButton();
    }

    /**
     * TODO(twellington): Try to remove this method. It's only used to return an in-product help
     *                    bubble anchor view... which should be moved out of tab and perhaps into
     *                    the status bar icon component.
     * @return The view containing the security icon.
     */
    public View getSecurityIconView() {
        return mLocationBar.getSecurityIconView();
    }

    /**
     * Adds a custom action button to the {@link Toolbar}, if it is supported.
     * @param drawable The {@link Drawable} to use as the background for the button.
     * @param description The content description for the custom action button.
     * @param listener The {@link OnClickListener} to use for clicks to the button.
     * @see #updateCustomActionButton
     */
    public void addCustomActionButton(
            Drawable drawable, String description, OnClickListener listener) {
        mToolbar.addCustomActionButton(drawable, description, listener);
    }

    /**
     * Updates the visual appearance of a custom action button in the {@link Toolbar},
     * if it is supported.
     * @param index The index of the button to update.
     * @param drawable The {@link Drawable} to use as the background for the button.
     * @param description The content description for the custom action button.
     * @see #addCustomActionButton
     */
    public void updateCustomActionButton(int index, Drawable drawable, String description) {
        mToolbar.updateCustomActionButton(index, drawable, description);
    }

    /**
     * Call to tear down all of the toolbar dependencies.
     */
    public void destroy() {
        mIsDestroyed = true;

        VrModuleProvider.unregisterVrModeObserver(mVrModeObserver);

        if (mInitializedWithNative) {
            mFindToolbarManager.removeObserver(mFindToolbarObserver);
        }
        if (mTabModelSelectorSupplier != null) {
            mTabModelSelectorSupplier = null;
        }
        if (mTabModelSelector != null) {
            mTabModelSelector.removeObserver(mTabModelSelectorObserver);
        }
        if (mBookmarkBridgeSupplier != null) {
            BookmarkBridge bridge = mBookmarkBridgeSupplier.get();
            if (bridge != null) bridge.removeObserver(mBookmarksObserver);

            mBookmarkBridgeSupplier.removeObserver(mBookmarkBridgeSupplierObserver);
            mBookmarkBridgeSupplier = null;
        }
        if (mTemplateUrlObserver != null) {
            TemplateUrlServiceFactory.get().removeObserver(mTemplateUrlObserver);
            mTemplateUrlObserver = null;
        }
        if (mLayoutStateProvider != null) {
            mLayoutStateProvider.removeObserver(mLayoutStateObserver);
            mLayoutStateProvider = null;
        }

        if (mLayoutStateProviderSupplier != null) {
            mLayoutStateProviderSupplier = null;
        }

        if (mLayoutManager != null) {
            mLayoutManager.getOverlayPanelManager().removeObserver(mOverlayPanelManagerObserver);
            mLayoutManager = null;
        }

        HomepageManager.getInstance().removeListener(mHomepageStateListener);

        if (mBottomControlsCoordinator != null) {
            mBottomControlsCoordinator = null;
        }

        if (mBottomControlsCoordinatorSupplier.get() != null) {
            mBottomControlsCoordinatorSupplier.get().destroy();
            mBottomControlsCoordinatorSupplier = null;
        }

        if (mLocationBar != null) {
            mLocationBar.destroy();
            mLocationBar = null;
        }

        mToolbarTabController.destroy();

        mToolbar.removeUrlExpansionObserver(mStatusBarColorController);
        mToolbar.destroy();

        if (mTabObserver != null) {
            Tab currentTab = mLocationBarModel.getTab();
            if (currentTab != null) currentTab.removeObserver(mTabObserver);
            mTabObserver = null;
        }

        mIncognitoStateProvider.destroy();
        mTabCountProvider.destroy();

        mLocationBarModel.destroy();
        mHandler.removeCallbacksAndMessages(null); // Cancel delayed tasks.
        mBrowserControlsSizer.removeObserver(mBrowserControlsObserver);
        mFullscreenManager.removeObserver(mFullscreenObserver);

        if (mTopUiThemeColorProvider != null) {
            mTopUiThemeColorProvider.removeThemeColorObserver(this);
        }

        if (mAppThemeColorProvider != null) {
            mAppThemeColorProvider.removeTintObserver(this);
            mAppThemeColorProvider.destroy();
            mAppThemeColorProvider = null;
        }

        if (mActivityTabTabObserver != null) {
            mActivityTabTabObserver.destroy();
            mActivityTabTabObserver = null;
        }

        if (mProgressBarCoordinator != null) mProgressBarCoordinator.destroy();

        if (mFindToolbarManager != null) {
            mFindToolbarManager.removeObserver(mFindToolbarObserver);
            mFindToolbarManager = null;
        }

        if (mMenuButtonCoordinator != null) {
            if (mMenuStateObserver != null) {
                UpdateMenuItemHelper.getInstance().unregisterObserver(mMenuStateObserver);
                mMenuStateObserver = null;
            }
            mMenuButtonCoordinator.destroy();
            mMenuButtonCoordinator = null;
        }

        if (mOverviewModeMenuButtonCoordinator != null) {
            if (mStartSurfaceMenuStateObserver != null) {
                UpdateMenuItemHelper.getInstance().unregisterObserver(
                        mStartSurfaceMenuStateObserver);
                mStartSurfaceMenuStateObserver = null;
            }
            mOverviewModeMenuButtonCoordinator.destroy();
            mOverviewModeMenuButtonCoordinator = null;
        }

        if (mHomeButtonCoordinator != null) {
            mHomeButtonCoordinator.destroy();
            mHomeButtonCoordinator = null;
        }
        if (mToggleTabStackButtonCoordinator != null) {
            mToggleTabStackButtonCoordinator.destroy();
            mToggleTabStackButtonCoordinator = null;
        }

        if (mCallbackController != null) {
            mCallbackController.destroy();
            mCallbackController = null;
        }

        if (mStartSurface != null) {
            mStartSurface.removeStateChangeObserver(mStartSurfaceStateObserver);
            mStartSurface.removeHeaderOffsetChangeListener(mStartSurfaceHeaderOffsetChangeListener);
            mStartSurface = null;
            mStartSurfaceStateObserver = null;
            mStartSurfaceHeaderOffsetChangeListener = null;
        }

        mActivity.unregisterComponentCallbacks(mComponentCallbacks);
        mComponentCallbacks = null;
        ChromeAccessibilityUtil.get().removeObserver(this);
    }

    /**
     * Called when the orientation of the activity has changed.
     */
    private void onOrientationChange(int newOrientation) {
        if (mActionModeController != null) mActionModeController.showControlsOnOrientationChange();
    }

    @Override
    public void onAccessibilityModeChanged(boolean enabled) {
        if (mIsStartSurfaceEnabled != ReturnToChromeUtil.isStartSurfaceEnabled(mActivity)) {
            // If Start surface is disabled or re-enabled due to the accessibility change, restarts
            // the activity to create the correct Toolbar from scratch.
            recreateActivityWithTabReparenting();
            return;
        }
        mToolbar.onAccessibilityStatusChanged(enabled);
    }

    @VisibleForTesting
    static String homepageUrl() {
        String homePageUrl = HomepageManager.getHomepageUri();
        if (TextUtils.isEmpty(homePageUrl)) homePageUrl = UrlConstants.NTP_URL;
        return homePageUrl;
    }

    private void registerTemplateUrlObserver() {
        final TemplateUrlService templateUrlService = TemplateUrlServiceFactory.get();
        assert mTemplateUrlObserver == null;
        mTemplateUrlObserver = new TemplateUrlServiceObserver() {
            private TemplateUrl mSearchEngine =
                    templateUrlService.getDefaultSearchEngineTemplateUrl();

            @Override
            public void onTemplateURLServiceChanged() {
                TemplateUrl searchEngine = templateUrlService.getDefaultSearchEngineTemplateUrl();
                if ((mSearchEngine == null && searchEngine == null)
                        || (mSearchEngine != null && mSearchEngine.equals(searchEngine))) {
                    return;
                }

                mSearchEngine = searchEngine;
                mToolbar.onDefaultSearchEngineChanged();
            }
        };
        templateUrlService.addObserver(mTemplateUrlObserver);
    }

    private void handleTabRestoreCompleted() {
        if (!mTabRestoreCompleted || !mInitializedWithNative) return;
        mToolbar.onStateRestored();
    }

    // TODO(https://crbug.com/865801): remove the below two methods if possible.
    public boolean back() {
        return mToolbarTabController.back();
    }

    public boolean forward() {
        return mToolbarTabController.forward();
    }

    /**
     * Triggered when the URL input field has gained or lost focus.
     * @param hasFocus Whether the URL field has gained focus.
     */
    @Override
    public void onUrlFocusChange(boolean hasFocus) {
        mToolbar.onUrlFocusChange(hasFocus);

        if (mFindToolbarManager != null && hasFocus) mFindToolbarManager.hideToolbar();

        if (mControlsVisibilityDelegate == null) return;
        if (hasFocus) {
            mFullscreenFocusToken =
                    mControlsVisibilityDelegate.showControlsPersistentAndClearOldToken(
                            mFullscreenFocusToken);
        } else {
            mControlsVisibilityDelegate.releasePersistentShowingToken(mFullscreenFocusToken);
        }

        mUrlFocusChangedCallback.onResult(hasFocus);
    }

    /**
     * Updates the primary color used by the model to the given color.
     * @param color The primary color for the current tab.
     * @param shouldAnimate Whether the change of color should be animated.
     */
    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
        if (mBottomControlsCoordinator != null) {
            ((BottomToolbarThemeCommunicator) mBottomControlsCoordinator).onThemeChanged(color);
        }

        mTempColor = color;

        if (!mShouldUpdateToolbarPrimaryColor) return;

        boolean colorChanged = mCurrentThemeColor != color;
        if (!colorChanged) return;

        mCurrentThemeColor = color;
        mLocationBarModel.setPrimaryColor(color);
        mToolbar.onPrimaryColorChanged(shouldAnimate);
        // TODO(https://crbug.com/865801, pnoland): Rationalize theme color logic
        // into a set of documented, self-contained providers that we can inject to the appropriate
        // sub-components. That will let us have every component handle its own coloring, and remove
        // onThemeColorChanged from ToolbarManager.
        mCustomTabThemeColorProvider.setPrimaryColor(color, shouldAnimate);
    }

    @Override
    public void onTintChanged(ColorStateList tint, @BrandedColorScheme int brandedColorScheme) {
        updateBookmarkButtonStatus();

        if (mBottomControlsCoordinator != null) {
            ((BottomToolbarThemeCommunicator) mBottomControlsCoordinator).onTintChanged(tint, brandedColorScheme);
        }

        mTempColorStateList = tint;
        mTempBrandedColorScheme = brandedColorScheme;

        if (mShouldUpdateToolbarPrimaryColor) {
            mCustomTabThemeColorProvider.setTint(tint, brandedColorScheme);
        }
    }

    /**
     * @param shouldUpdate Whether we should be updating the toolbar primary color based on updates
     *                     from the Tab.
     */
    public void setShouldUpdateToolbarPrimaryColor(boolean shouldUpdate) {
        mShouldUpdateToolbarPrimaryColor = shouldUpdate;
    }

    /**
     * @return The primary toolbar color.
     */
    public int getPrimaryColor() {
        return mLocationBarModel.getPrimaryColor();
    }

    /**
     * Sets the visibility of the Toolbar shadow.
     */
    public void setToolbarShadowVisibility(int visibility) {
        View toolbarShadow = mControlContainer.findViewById(R.id.toolbar_hairline);
        if (toolbarShadow != null) toolbarShadow.setVisibility(visibility);
    }

    /**
     * Force to hide toolbar shadow.
     * @param forceHideShadow Whether toolbar shadow should be hidden.
     */
    public void setForceHideShadow(boolean forceHideShadow) {
        mToolbar.setForceHideShadow(forceHideShadow);
    }

    /**
     * We use getTopControlOffset to position the top controls. However, the toolbar's height may
     * be less than the total top controls height. If that's the case, this method will return the
     * extra offset needed to align the toolbar at the bottom of the top controls.
     * @return The extra Y offset for the toolbar in pixels.
     */
    private int getToolbarExtraYOffset() {
        return mBrowserControlsSizer.getTopControlsMinHeight();
    }

    /**
     * Sets the drawable that the close button shows, or hides it if {@code drawable} is
     * {@code null}.
     */
    public void setCloseButtonDrawable(Drawable drawable) {
        mToolbar.setCloseButtonImageResource(drawable);
    }

    /**
     * Sets whether a title should be shown within the Toolbar.
     * @param showTitle Whether a title should be shown.
     */
    public void setShowTitle(boolean showTitle) {
        mToolbar.setShowTitle(showTitle);
    }

    /**
     * @see TopToolbarCoordinator#setUrlBarHidden(boolean)
     */
    public void setUrlBarHidden(boolean hidden) {
        mToolbar.setUrlBarHidden(hidden);
    }

    /**
     * Focuses or unfocuses the URL bar.
     *
     * If you request focus and the UrlBar was already focused, this will select all of the text.
     *
     * @param focused Whether URL bar should be focused.
     * @param reason The given reason.
     */
    public void setUrlBarFocus(boolean focused, @OmniboxFocusReason int reason) {
        if (!mInitializedWithNative) return;
        if (mLocationBar.getOmniboxStub() == null) return;
        boolean wasFocused = mLocationBar.getOmniboxStub().isUrlBarFocused();
        mLocationBar.getOmniboxStub().setUrlBarFocus(focused, null, reason);
        if (wasFocused && focused) {
            mLocationBar.selectAll();
        }
    }

    /**
     * See {@link #setUrlBarFocus}, but if native is not loaded it will queue the request instead
     * of dropping it.
     */
    public void setUrlBarFocusOnceNativeInitialized(
            boolean focused, @OmniboxFocusReason int reason) {
        if (mInitializedWithNative) {
            setUrlBarFocus(focused, reason);
            return;
        }

        if (focused) {
            // Remember requests to focus the Url bar and replay them once native has been
            // initialized. This is important for the Launch to Incognito Tab flow (see
            // IncognitoTabLauncher.
            mOnInitializedRunnable = () -> {
                setUrlBarFocus(focused, reason);
            };
        } else {
            mOnInitializedRunnable = null;
        }
    }

    /**
     * Reverts any pending edits of the location bar and reset to the page state.  This does not
     * change the focus state of the location bar.
     */
    public void revertLocationBarChanges() {
        mLocationBar.revertChanges();
    }

    /**
     * Handle all necessary tasks that can be delayed until initialization completes.
     * @param activityCreationTimeMs The time of creation for the activity this toolbar belongs to.
     * @param activityName Simple class name for the activity this toolbar belongs to.
     */
    public void onDeferredStartup(final long activityCreationTimeMs, final String activityName) {
        mLocationBar.onDeferredStartup();
    }

    /**
     * Finish any toolbar animations.
     */
    public void finishAnimations() {
        if (mInitializedWithNative) mToolbar.finishAnimations();
    }

    /**
     * @return The current {@link LoadProgressCoordinator}.
     */
    public LoadProgressCoordinator getProgressBarCoordinator() {
        return mProgressBarCoordinator;
    }

    /**
     * @return A {@link TopToolbarInteractabilityManager} which allows non toolbar clients to toggle
     *         the interactability of elements present in the top toolbar.
     */
    public @NonNull TopToolbarInteractabilityManager getTopToolbarInteractabilityManager() {
        return mToolbar.getTopToolbarInteractabilityManager();
    }

    /**
     * Updates the current button states and calls appropriate abstract visibility methods, giving
     * inheriting classes the chance to update the button visuals as well.
     */
    private void updateButtonStatus() {
        Tab currentTab = mLocationBarModel.getTab();
        boolean tabCrashed = currentTab != null && SadTab.isShowing(currentTab);

        mToolbar.updateButtonVisibility();
        mToolbar.updateBackButtonVisibility(currentTab != null && currentTab.canGoBack());
        mToolbar.updateForwardButtonVisibility(currentTab != null && currentTab.canGoForward());
        updateReloadState(tabCrashed);
        updateBookmarkButtonStatus();
        if (mToolbar.getMenuButtonWrapper() != null) {
            mToolbar.getMenuButtonWrapper().setVisibility(View.VISIBLE);
        }
    }

    private void updateBookmarkButtonStatus() {
        if (mBookmarkBridgeSupplier == null) return;
        Tab currentTab = mLocationBarModel.getTab();
        BookmarkBridge bridge = mBookmarkBridgeSupplier.get();
        boolean isBookmarked =
                currentTab != null && bridge != null && bridge.hasBookmarkIdForTab(currentTab);
        boolean editingAllowed =
                currentTab == null || bridge == null || bridge.isEditBookmarksEnabled();
        mToolbar.updateBookmarkButton(isBookmarked, editingAllowed);
    }

    private void updateReloadState(boolean tabCrashed) {
        Tab currentTab = mLocationBarModel.getTab();
        boolean isLoading = false;
        if (!tabCrashed) {
            isLoading = (currentTab != null && currentTab.isLoading()) || !mInitializedWithNative;
        }
        mToolbar.updateReloadButtonVisibility(isLoading);
        mMenuButtonCoordinator.updateReloadingState(isLoading);
    }

    /**
     * Triggered when the selected tab has changed.
     */
    private void refreshSelectedTab(Tab tab) {
        boolean wasIncognito = mLocationBarModel.isIncognito();
        Tab previousTab = mLocationBarModel.getTab();

        boolean isIncognito =
                tab != null ? tab.isIncognito() : mTabModelSelector.isIncognitoSelected();
        mLocationBarModel.setTab(tab, isIncognito);

        updateCurrentTabDisplayStatus();

        // This method is called prior to action mode destroy callback for incognito <-> normal
        // tab switch. Makes sure the action mode toolbar is hidden before selecting the new tab.
        if (previousTab != null && wasIncognito != isIncognito
                && DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity)) {
            mActionModeController.startHideAnimation();
        }
        if (previousTab != tab || wasIncognito != isIncognito) {
            int defaultPrimaryColor = ChromeColors.getDefaultThemeColor(mActivity, isIncognito);
            int primaryColor = tab != null
                    ? mTopUiThemeColorProvider.calculateColor(tab, tab.getThemeColor())
                    : defaultPrimaryColor;
            // TODO(jinsukkim): Let TopUiThemeColorProvider handle this by updating the theme color.
            onThemeColorChanged(primaryColor, false);

            onTabOrModelChanged();

            if (tab != null) {
                mToolbar.onNavigatedToDifferentPage();
            }

            // Ensure the URL bar loses focus if the tab it was interacting with is changed from
            // underneath it.
            setUrlBarFocus(false, OmniboxFocusReason.UNFOCUS);

            // Place the cursor in the Omnibox if applicable.  We always clear the focus above to
            // ensure the shield placed over the content is dismissed when switching tabs.  But if
            // needed, we will refocus the omnibox and make the cursor visible here.
            maybeShowCursorInLocationBar();
        }

        updateButtonStatus();
    }

    private void onTabOrModelChanged() {
        mToolbar.onTabOrModelChanged();
        checkIfNtpLoaded();
    }

    private void maybeShowPriceDropIPH() {
        // TODO(crbug.com/1335197): Check if price drop occurred before showing IPH
        if (!PriceTrackingFeatures.isPriceDropIphEnabled()) {
            return;
        }

        ToggleTabStackButton toggleTabStackButton =
                mControlContainer.findViewById(R.id.tab_switcher_button);
        UserEducationHelper userEducationHelper = new UserEducationHelper(mActivity, mHandler);
        HighlightParams params = new HighlightParams(HighlightShape.CIRCLE);
        params.setBoundsRespectPadding(true);
        userEducationHelper.requestShowIPH(
                new IPHCommandBuilder(mControlContainer.getResources(),
                        FeatureConstants.PRICE_DROP_NTP_FEATURE, R.string.price_drop_spotted_iph,
                        R.string.price_drop_spotted_iph)
                        .setInsetRect(new Rect(0, 0, 0,
                                -(mControlContainer.getResources().getDimensionPixelOffset(
                                        R.dimen.price_drop_spotted_iph_ntp_tabswitcher_y_inset))))
                        .setAnchorView(toggleTabStackButton)
                        .setHighlightParams(params)
                        .setDismissOnTouch(true)
                        .build());
    }

    private void checkIfNtpLoaded() {
        NewTabPage ntp = getNewTabPageForCurrentTab();
        if (ntp != null) {
            ntp.setOmniboxStub(mLocationBar.getOmniboxStub());
            mLocationBarModel.notifyNtpStartedLoading();
            maybeShowPriceDropIPH();
        }
    }

    private void setBookmarkBridge(BookmarkBridge bookmarkBridge) {
        if (bookmarkBridge == null) return;
        bookmarkBridge.addObserver(mBookmarksObserver);
    }

    private void setLayoutStateProvider(LayoutStateProvider layoutStateProvider) {
        assert mLayoutStateProvider == null : "the mLayoutStateProvider should set at most once.";

        mLayoutStateProvider = layoutStateProvider;
        mLayoutStateProvider.addObserver(mLayoutStateObserver);

        if (mLayoutStateProvider.isLayoutVisible(LayoutType.TAB_SWITCHER)) {
            // TODO(1222695): We shouldn't need to post this. Instead we should wait until the
            //                dependencies are ready. This logic was introduced to move asynchronous
            //                observer events from the infra (LayoutManager) into the feature using
            //                it.
            mControlContainer.post(mCallbackController.makeCancelable(
                    () -> updateForLayout(LayoutType.TAB_SWITCHER, true)));
        }

        mAppThemeColorProvider.setLayoutStateProvider(mLayoutStateProvider);
        mLocationBarModel.setLayoutStateProvider(mLayoutStateProvider);
        if (mBottomControlsCoordinatorSupplier.get() != null) {
            mBottomControlsCoordinatorSupplier.get().setLayoutStateProvider(mLayoutStateProvider);
        }
    }

    private void updateCurrentTabDisplayStatus() {
        mLocationBarModel.notifyUrlChanged();
        updateTabLoadingState(true);
    }

    private void updateTabLoadingState(boolean updateUrl) {
        if (mIsDestroyed) return;

        mLocationBarModel.notifySecurityStateChanged();
        if (updateUrl) {
            mLocationBarModel.notifyUrlChanged();
            updateButtonStatus();
        }
    }

    /**
     * @return The {@link OmniboxStub}.
     */
    @Nullable
    public OmniboxStub getOmniboxStub() {
        // TODO(crbug.com/1000295): Split fakebox component out of ntp package.
        return mLocationBar.getOmniboxStub();
    }

    @Nullable
    public VoiceRecognitionHandler getVoiceRecognitionHandler() {
        return mLocationBar.getVoiceRecognitionHandler();
    }

    /**
     * Called whenever the NTP could have been entered (e.g. tab content changed, tab navigated to
     * from the tab strip/tab switcher, etc.). If the user is on a tablet and indeed entered the
     * NTP, we will check two cases:
     *   1. If a11y is enabled, we will request a11y focus on the omnibox (e.g. for TalkBack).
     *   2. If a keyboard is plugged in, we will show the URL bar cursor (without focus animations).
     */
    private void maybeShowCursorInLocationBar() {
        if (!DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity)) return;
        Tab tab = mLocationBarModel.getTab();
        if (tab == null) return;
        NativePage nativePage = tab.getNativePage();
        if (!(nativePage instanceof NewTabPage) && !(nativePage instanceof IncognitoNewTabPage)) {
            return;
        }

        if (ChromeAccessibilityUtil.get().isAccessibilityEnabled()
                && nativePage instanceof NewTabPage) {
            mLocationBar.requestUrlBarAccessibilityFocus();
        }

        if (mActivity.getResources().getConfiguration().keyboard == Configuration.KEYBOARD_QWERTY) {
            mLocationBar.showUrlBarCursorWithoutFocusAnimations();
        }
    }

    /**
     * Sets the top margin for the control container.
     * @param margin The margin in pixels.
     */
    private void setControlContainerTopMargin(int margin) {
        final ViewGroup.MarginLayoutParams layoutParams =
                ((ViewGroup.MarginLayoutParams) mControlContainer.getLayoutParams());
        if (layoutParams.topMargin == margin) {
            return;
        }

        layoutParams.topMargin = margin;
        mControlContainer.setLayoutParams(layoutParams);
    }

    /** Returns {@link LocationBar} for access in tests. */
    @VisibleForTesting
    public LocationBar getLocationBarForTesting() {
        return mLocationBar;
    }

    /** Returns {@link LocationBarModel} for access in tests. */
    @VisibleForTesting
    public LocationBarModel getLocationBarModelForTesting() {
        return mLocationBarModel;
    }

    /**
     * @return The {@link ToolbarLayout} that constitutes the toolbar.
     */
    @VisibleForTesting
    public ToolbarLayout getToolbarLayoutForTesting() {
        return mToolbar.getToolbarLayoutForTesting();
    }

    /**
     * Get the home button on the top toolbar to verify the button status.
     * Note that this home button is not always the home button that on the UI, and the button is
     * not always visible.
     * @return The {@link HomeButton} that lives in the top toolbar.
     */
    @VisibleForTesting
    public HomeButton getHomeButtonForTesting() {
        return mToolbar.getToolbarLayoutForTesting().getHomeButton();
    }

    /**
     * @return View for toolbar container.
     */
    @VisibleForTesting
    public View getContainerViewForTesting() {
        return mControlContainer.getView();
    }

    @VisibleForTesting
    public ToolbarTabController getToolbarTabControllerForTesting() {
        return mToolbarTabController;
    }
}

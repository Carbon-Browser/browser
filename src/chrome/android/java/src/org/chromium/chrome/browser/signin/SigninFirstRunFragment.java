// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.FrameLayout;

import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.VisibleForTesting;
import androidx.fragment.app.Fragment;

import org.chromium.base.Promise;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.enterprise.util.EnterpriseInfo;
import org.chromium.chrome.browser.firstrun.FirstRunFragment;
import org.chromium.chrome.browser.firstrun.FirstRunUtils;
import org.chromium.chrome.browser.firstrun.MobileFreProgress;
import org.chromium.chrome.browser.firstrun.SkipTosDialogPolicyListener;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.ui.device_lock.DeviceLockCoordinator;
import org.chromium.chrome.browser.ui.signin.SigninUtils;
import org.chromium.chrome.browser.ui.signin.fullscreen_signin.FullscreenSigninConfig;
import org.chromium.chrome.browser.ui.signin.fullscreen_signin.FullscreenSigninCoordinator;
import org.chromium.chrome.browser.ui.signin.fullscreen_signin.FullscreenSigninMediator;
import org.chromium.chrome.browser.ui.signin.fullscreen_signin.FullscreenSigninView;
import org.chromium.components.browser_ui.device_lock.DeviceLockActivityLauncher;
import org.chromium.components.signin.AccountManagerFacadeProvider;
import org.chromium.components.signin.metrics.AccountConsistencyPromoAction;
import org.chromium.components.signin.metrics.SigninAccessPoint;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManagerHolder;

import android.content.res.Configuration;
import android.media.MediaPlayer;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.os.Handler;
import android.net.Uri;
import org.chromium.chrome.browser.device.DeviceClassManager;
import android.widget.ImageView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.graphics.Color;

import android.content.res.Configuration;
import android.media.MediaPlayer;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.os.Handler;
import android.net.Uri;
import org.chromium.chrome.browser.device.DeviceClassManager;
import android.widget.ImageView;
import android.graphics.Color;
import android.graphics.Shader;
import android.graphics.LinearGradient;

import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;
import org.chromium.ui.text.ChromeClickableSpan;
import android.text.method.LinkMovementMethod;
import android.widget.RelativeLayout;

/** This fragment handles the sign-in without sync consent during the FRE. */
public class SigninFirstRunFragment extends Fragment
        implements FirstRunFragment,
                FullscreenSigninCoordinator.Delegate,
                DeviceLockCoordinator.Delegate,
                SurfaceHolder.Callback {
    @VisibleForTesting static final int ADD_ACCOUNT_REQUEST_CODE = 1;

    // Used as a view holder for the current orientation of the device.
    private FrameLayout mFragmentView;
    private View mMainView;
    private ModalDialogManager mModalDialogManager;
    private SkipTosDialogPolicyListener mSkipTosDialogPolicyListener;
    private FullscreenSigninCoordinator mFullscreenSigninCoordinator;
    private DeviceLockCoordinator mDeviceLockCoordinator;
    private boolean mExitFirstRunCalled;
    private boolean mDelayedExitFirstRunCalledForTesting;

    private Button mAcceptButton;
    // private CheckBox mSendReportCheckBox;
    private TextView mTosAndPrivacy;
    // private View mTitle;
    private View mProgressSpinner;

    private long mTosAcceptedTime;

    private ImageView mLogoImageView;
    private SurfaceView surfaceView;
    private MediaPlayer player;
    private Handler mHandler;
    private int mCurrentVideoPosition;
    private View noAnimSpacerTop;
    private View noAnimSpacerBottom;
    private RelativeLayout mMainContainer;

    public SigninFirstRunFragment() {}

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mModalDialogManager = ((ModalDialogManagerHolder) getActivity()).getModalDialogManager();
        mFullscreenSigninCoordinator =
                new FullscreenSigninCoordinator(
                        requireContext(),
                        mModalDialogManager,
                        this,
                        PrivacyPreferencesManagerImpl.getInstance(),
                        new FullscreenSigninConfig(),
                        SigninAccessPoint.START_PAGE);

        if (getPageDelegate().isLaunchedFromCct()) {
            mSkipTosDialogPolicyListener =
                    new SkipTosDialogPolicyListener(
                            getPageDelegate().getPolicyLoadListener(),
                            EnterpriseInfo.getInstance(),
                            null);
            mSkipTosDialogPolicyListener.onAvailable(
                    (Boolean skipTos) -> {
                        if (skipTos) exitFirstRun();
                    });
        }
    }

    private void updateView(Context context) {
        // Avoid early calls.
        if (getPageDelegate() == null) {
            return;
        }

        final boolean umaDialogMayBeShown = false;
                //FREMobileIdentityConsistencyFieldTrial.shouldShowOldFreWithUmaDialog();
        final boolean hasChildAccount = false;//getPageDelegate().getProperties().getBoolean(
                //SyncConsentFirstRunFragment.IS_CHILD_ACCOUNT, false);
        final boolean isMetricsReportingDisabledByPolicy = true;

        ChromeClickableSpan clickableGoogleTermsSpan =
                new ChromeClickableSpan(
                        context, android.R.color.white, (view) -> showInfoPage(R.string.google_terms_of_service_url));
        ChromeClickableSpan clickableChromeAdditionalTermsSpan =
                new ChromeClickableSpan(
                        context, android.R.color.white,
                        (view) -> showInfoPage(R.string.chrome_additional_terms_of_service_url));

        CharSequence tosString = SpanApplier.applySpans(context.getString(R.string.fre_tos),
                    new SpanInfo("<LINK1>", "</LINK1>", clickableGoogleTermsSpan),
                    new SpanInfo("<LINK2>", "</LINK2>", clickableChromeAdditionalTermsSpan));
        mTosAndPrivacy.setText(tosString);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mFragmentView = null;
        if (mSkipTosDialogPolicyListener != null) {
            mSkipTosDialogPolicyListener.destroy();
            mSkipTosDialogPolicyListener = null;
        }
        mFullscreenSigninCoordinator.destroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // Keep device lock page if it's currently displayed.
        if (mDeviceLockCoordinator != null) {
            return;
        }
        // Inflate the view required for the current configuration and set it as the fragment view.
        mFragmentView.removeAllViews();
        mMainView =
                inflateFragmentView(
                        (LayoutInflater)
                                getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE),
                        getActivity());
        mFragmentView.addView(mMainView);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mFragmentView = new FrameLayout(getActivity());
        mMainView = inflateFragmentView(inflater, getActivity());
        mFragmentView.addView(mMainView);

        return mFragmentView;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == ADD_ACCOUNT_REQUEST_CODE
                && resultCode == Activity.RESULT_OK
                && data != null) {
            String addedAccountName = data.getStringExtra(AccountManager.KEY_ACCOUNT_NAME);
            if (addedAccountName != null) {
                mFullscreenSigninCoordinator.onAccountAdded(addedAccountName);
            }
        }
    }

    /** Implements {@link FirstRunFragment}. */
    @Override
    public void setInitialA11yFocus() {
        // Ignore calls before view is created.
        if (getView() == null) return;

        final View title = getView().findViewById(R.id.title);
        title.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
    }

    /** Implements {@link FirstRunFragment}. */
    @Override
    public void reset() {
        mFullscreenSigninCoordinator.reset();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void addAccount() {
        getPageDelegate().recordFreProgressHistogram(MobileFreProgress.WELCOME_ADD_ACCOUNT);
        AccountManagerFacadeProvider.getInstance()
                .createAddAccountIntent(
                        (@Nullable Intent intent) -> {
                            if (intent != null) {
                                startActivityForResult(intent, ADD_ACCOUNT_REQUEST_CODE);
                                return;
                            }

                            // AccountManagerFacade couldn't create intent, use SigninUtils to open
                            // settings instead.
                            SigninUtils.openSettingsForAllAccounts(getActivity());
                        });
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void acceptTermsOfService(boolean allowMetricsAndCrashUploading) {
        getPageDelegate().acceptTermsOfService(allowMetricsAndCrashUploading);
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void advanceToNextPage() {
        getPageDelegate().advanceToNextPage();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void recordUserSignInHistograms(@AccountConsistencyPromoAction int promoAction) {
        @MobileFreProgress
        int progressState =
                promoAction == AccountConsistencyPromoAction.SIGNED_IN_WITH_DEFAULT_ACCOUNT
                        ? MobileFreProgress.WELCOME_SIGNIN_WITH_DEFAULT_ACCOUNT
                        : MobileFreProgress.WELCOME_SIGNIN_WITH_NON_DEFAULT_ACCOUNT;
        getPageDelegate().recordFreProgressHistogram(progressState);
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void recordSigninDismissedHistograms() {
        getPageDelegate().recordFreProgressHistogram(MobileFreProgress.WELCOME_DISMISS);
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void recordLoadCompletedHistograms(
            @FullscreenSigninMediator.LoadPoint int slowestLoadPoint) {
        getPageDelegate().recordLoadCompletedHistograms(slowestLoadPoint);
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void recordNativeInitializedHistogram() {
        getPageDelegate().recordNativeInitializedHistogram();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void showInfoPage(@StringRes int url) {
        getPageDelegate().showInfoPage(url);
    }

    @Override
    public OneshotSupplier<ProfileProvider> getProfileSupplier() {
        return getPageDelegate().getProfileProviderSupplier();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public OneshotSupplier<Boolean> getPolicyLoadListener() {
        return getPageDelegate().getPolicyLoadListener();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public OneshotSupplier<Boolean> getChildAccountStatusSupplier() {
        return getPageDelegate().getChildAccountStatusSupplier();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public Promise<Void> getNativeInitializationPromise() {
        return getPageDelegate().getNativeInitializationPromise();
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public boolean shouldDisplayManagementNoticeOnManagedDevices() {
        return true;
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public boolean shouldDisplayFooterText() {
        return true;
    }

    @MainThread
    private void exitFirstRun() {
        // Make sure this function is called at most once.
        if (!mExitFirstRunCalled) {
            mExitFirstRunCalled = true;
            PostTask.postDelayedTask(
                    TaskTraits.UI_DEFAULT,
                    () -> {
                        mDelayedExitFirstRunCalledForTesting = true;

                        // If we've been detached, someone else has handled something, and it's no
                        // longer clear that we should still be accepting the ToS and exiting the
                        // FRE.
                        if (isDetached()) return;

                        getPageDelegate().acceptTermsOfService(false);
                        getPageDelegate().exitFirstRun();
                    },
                    FirstRunUtils.getSkipTosExitDelayMs());
        }
    }

    private View inflateFragmentView(LayoutInflater inflater, Activity activity) {
        boolean useLandscapeLayout = SigninUtils.shouldShowDualPanesHorizontalLayout(activity);

        final FullscreenSigninView view =
                (FullscreenSigninView)
                        inflater.inflate(
                                R.layout.fullscreen_signin_portrait_view,
                                null,
                                false);
        mFullscreenSigninCoordinator.setView(view);

        try {
            mLogoImageView = view.findViewById(R.id.image);

            noAnimSpacerTop = view.findViewById(R.id.fre_spacer_top);
            noAnimSpacerBottom = view.findViewById(R.id.fre_spacer_bottom);

            mMainContainer = view.findViewById(R.id.main_container);

            mProgressSpinner = view.findViewById(R.id.progress_spinner);
            mProgressSpinner.setVisibility(View.GONE);
            mAcceptButton = (Button) view.findViewById(R.id.terms_accept);
            mTosAndPrivacy = (TextView) view.findViewById(R.id.tos_and_privacy);

            // set start button gradient
            Shader textShader = new LinearGradient(0, 0, 175, 0,
                new int[]{Color.parseColor("#FF320A"),Color.parseColor("#FF9133")},
               null, Shader.TileMode.REPEAT);
            mAcceptButton.getPaint().setShader(textShader);

            mAcceptButton.setOnClickListener((v) -> {
              getPageDelegate().acceptTermsOfService(false);
              getPageDelegate().advanceToNextPage();
            });

            // Make TextView links clickable.
            mTosAndPrivacy.setMovementMethod(LinkMovementMethod.getInstance());

            // Intro animation
            surfaceView = (SurfaceView) view.findViewById(R.id.fre_surface_view);
            surfaceView.getHolder().addCallback(this);
            // if (DeviceClassManager.enableAnimations()) {
                // setupMediaPlayer();
            // } else {
                setAnimationDisabled();
            // }
            getActivity().getWindow().setNavigationBarColor(Color.parseColor("#1E1E1E"));

            updateView(view.getContext());
        } catch (Exception ignore) {
          
        }

        return view;
    }

    /** Implements {@link FullscreenSigninCoordinator.Delegate}. */
    @Override
    public void displayDeviceLockPage(Account selectedAccount) {
        Profile profile = ProfileProvider.getOrCreateProfile(getProfileSupplier().get(), false);
        mDeviceLockCoordinator =
                new DeviceLockCoordinator(
                        this,
                        getPageDelegate().getWindowAndroid(),
                        profile,
                        getActivity(),
                        selectedAccount);
    }

    /** Implements {@link DeviceLockCoordinator.Delegate}. */
    @Override
    public void setView(View view) {
        mFragmentView.removeAllViews();
        mFragmentView.addView(view);
    }

    /** Implements {@link DeviceLockCoordinator.Delegate}. */
    @Override
    public void onDeviceLockReady() {
        if (mFragmentView != null) {
            restoreMainView();
        }
        if (mDeviceLockCoordinator != null) {
            mDeviceLockCoordinator.destroy();
            mDeviceLockCoordinator = null;

            // Hold off on continuing sign-in if the delegate is null (due to the host activity
            // being killed in the background.
            if (getPageDelegate() != null) {
                mFullscreenSigninCoordinator.continueSignIn();
            }
        }
    }

    /** Implements {@link DeviceLockCoordinator.Delegate}. */
    @Override
    public void onDeviceLockRefused() {
        mFullscreenSigninCoordinator.cancelSignInAndDismiss();
    }

    @Override
    public @DeviceLockActivityLauncher.Source String getSource() {
        return DeviceLockActivityLauncher.Source.FIRST_RUN;
    }

    private void restoreMainView() {
        mFragmentView.removeAllViews();
        mFragmentView.addView(mMainView);
    }

    boolean getDelayedExitFirstRunCalledForTesting() {
        return mDelayedExitFirstRunCalledForTesting;
    }

    private void setAnimationDisabled() {
        mAcceptButton.setVisibility(View.VISIBLE);
        mTosAndPrivacy.setVisibility(View.VISIBLE);
        mLogoImageView.setVisibility(View.VISIBLE);
        // mTitle.setVisibility(View.VISIBLE);
        noAnimSpacerTop.setVisibility(View.VISIBLE);
        noAnimSpacerBottom.setVisibility(View.VISIBLE);
        surfaceView.setVisibility(View.GONE);
        mMainContainer.setBackground(mMainContainer.getContext().getResources().getDrawable(R.drawable.ic_first_launch_background));
        updateParams(false);
    }

    private java.lang.Runnable updateSurfaceViewVisibility = new java.lang.Runnable() {
        @Override
        public void run() {
            if (player.isPlaying() && player.getCurrentPosition() > 1) {
                surfaceView.setAlpha(1);
            } else {
                mHandler.postDelayed(this, 250);
            }
        }
    };

    private void updateParams(boolean shouldUpdateSurfaceView) {
        int screenHeight = getActivity().getWindowManager().getDefaultDisplay().getHeight();
        if (shouldUpdateSurfaceView) {
           // Adjust the size of the video
           // so it fits on the screen
           int videoWidth = player.getVideoWidth();
           int videoHeight = player.getVideoHeight();
           float videoProportion = (float) videoWidth / (float) videoHeight;
           int screenWidth = getActivity().getWindowManager().getDefaultDisplay().getWidth();
           float screenProportion = (float) screenWidth / (float) screenHeight;

           ViewGroup.LayoutParams lp = surfaceView.getLayoutParams();

           if (videoProportion > screenProportion) {
               lp.width = screenWidth;
               lp.height = (int) ((float) screenWidth / videoProportion);
           } else {
               lp.width = (int) (videoProportion * (float) screenHeight);
               lp.height = screenHeight;
           }
           surfaceView.setLayoutParams(lp);
        }

        // set terms and conditions text position before making visibile
        // ViewGroup.MarginLayoutParams mTosAndPrivacyParams = (ViewGroup.MarginLayoutParams) mTosAndPrivacy.getLayoutParams();
        // int margin = getResources().getDimensionPixelSize(R.dimen.fre_content_margin);
        // mTosAndPrivacyParams.setMargins(margin, margin, 0, screenHeight / 5);
        // mTosAndPrivacy.setLayoutParams(mTosAndPrivacyParams);
     }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (player == null) {
            // if (DeviceClassManager.enableAnimations()) {
            //     setupMediaPlayer();
            // } else {
                setAnimationDisabled();
            // }
        }
        try {
            player.setDisplay(holder);
            holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
            holder.setSizeFromLayout();
        } catch (Exception ignore) {

        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        if (player != null) {
            mCurrentVideoPosition = player.getCurrentPosition();
            player.release();
            player = null;
        }
    }

    private void setupMediaPlayer() {
        player = new MediaPlayer();
        player = MediaPlayer.create(getContext(), null);

        Uri mUri = Uri.parse("android.resource://" + getContext().getPackageName() + "/" + null);//"file:///android_asset/sfb_anim_intro.mp4");
        String uri = mUri.getPath();
        player.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                updateParams(true);

                if (!player.isPlaying()) {
                    player.start();

                    if (mHandler == null) mHandler = new Handler();
                    mHandler.postDelayed(updateSurfaceViewVisibility, 0);
                }

                if (mCurrentVideoPosition != 0) {
                    player.seekTo(mCurrentVideoPosition);
                    player.start();
                }
            }
        });

        player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mediaPlayer) {
                // mAcceptButton.setVisibility(View.VISIBLE);
                // mTosAndPrivacy.setVisibility(View.VISIBLE);
                setAnimationDisabled();
            }
        });

        try {
            player.setDataSource(uri);
        } catch (Exception e) { }
    }
}

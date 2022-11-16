// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.site_settings;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;

import androidx.annotation.Nullable;
import androidx.preference.Preference;

import org.chromium.base.Callback;
import org.chromium.base.CommandLine;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browserservices.permissiondelegation.InstalledWebappPermissionManager;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataTabsFragment;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncherImpl;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.notifications.channels.SiteChannelsManager;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxBridge;
import org.chromium.chrome.browser.privacy_sandbox.PrivacySandboxSnackbarController;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.chrome.browser.settings.FaviconLoader;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.webapps.WebappRegistry;
import org.chromium.components.browser_ui.settings.ManagedPreferenceDelegate;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;
import org.chromium.components.browser_ui.site_settings.SiteSettingsDelegate;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.components.embedder_support.util.Origin;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.content_public.browser.ContentFeatureList;
import org.chromium.content_public.common.ContentFeatures;
import org.chromium.content_public.common.ContentSwitches;
import org.chromium.url.GURL;

import java.util.Set;

/**
 * A SiteSettingsDelegate instance that contains Chrome-specific Site Settings logic.
 */
public class ChromeSiteSettingsDelegate implements SiteSettingsDelegate {
    private final Context mContext;
    private final Profile mProfile;
    private ManagedPreferenceDelegate mManagedPreferenceDelegate;
    private PrivacySandboxSnackbarController mPrivacySandboxController;
    private LargeIconBridge mLargeIconBridge;

    public ChromeSiteSettingsDelegate(Context context, Profile profile) {
        mContext = context;
        mProfile = profile;
    }

    @Override
    public void onDestroyView() {
        if (mLargeIconBridge != null) {
            mLargeIconBridge.destroy();
            mLargeIconBridge = null;
        }
    }

    /**
     * Used to set an instance of {@link SnackbarManager} by the parent activity.
     */
    public void setSnackbarManager(SnackbarManager manager) {
        if (manager != null) {
            mPrivacySandboxController = new PrivacySandboxSnackbarController(
                    mContext, manager, new SettingsLauncherImpl());
        }
    }

    @Override
    public BrowserContextHandle getBrowserContextHandle() {
        return mProfile;
    }

    @Override
    public ManagedPreferenceDelegate getManagedPreferenceDelegate() {
        if (mManagedPreferenceDelegate == null) {
            mManagedPreferenceDelegate = new ChromeManagedPreferenceDelegate() {
                @Override
                public boolean isPreferenceControlledByPolicy(Preference preference) {
                    return false;
                }
            };
        }
        return mManagedPreferenceDelegate;
    }

    @Override
    public void getFaviconImageForURL(GURL faviconUrl, Callback<Drawable> callback) {
        if (mLargeIconBridge == null) {
            mLargeIconBridge = new LargeIconBridge(mProfile);
        }
        FaviconLoader.loadFavicon(mContext, mLargeIconBridge, faviconUrl, callback);
    }

    @Override
    public boolean isCategoryVisible(@SiteSettingsCategory.Type int type) {
        switch (type) {
            // TODO(csharrison): Remove this condition once the experimental UI lands. It is not
            // great to dynamically remove the preference in this way.
            case SiteSettingsCategory.Type.ADS:
                return SiteSettingsCategory.adsCategoryEnabled();
            case SiteSettingsCategory.Type.AUTO_DARK_WEB_CONTENT:
                return ChromeFeatureList.isEnabled(
                        ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING);
            case SiteSettingsCategory.Type.BLUETOOTH:
                return ContentFeatureList.isEnabled(
                        ContentFeatureList.WEB_BLUETOOTH_NEW_PERMISSIONS_BACKEND);
            case SiteSettingsCategory.Type.BLUETOOTH_SCANNING:
                return CommandLine.getInstance().hasSwitch(
                        ContentSwitches.ENABLE_EXPERIMENTAL_WEB_PLATFORM_FEATURES);
            case SiteSettingsCategory.Type.FEDERATED_IDENTITY_API:
                return ContentFeatureList.isEnabled(ContentFeatures.FED_CM);
            case SiteSettingsCategory.Type.NFC:
                return ContentFeatureList.isEnabled(ContentFeatureList.WEB_NFC);
            default:
                return true;
        }
    }

    @Override
    public boolean isIncognitoModeEnabled() {
        return IncognitoUtils.isIncognitoModeEnabled();
    }

    @Override
    public boolean isQuietNotificationPromptsFeatureEnabled() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.QUIET_NOTIFICATION_PROMPTS);
    }

    @Override
    public String getChannelIdForOrigin(String origin) {
        return SiteChannelsManager.getInstance().getChannelIdForOrigin(origin);
    }

    @Override
    public String getAppName() {
        return mContext.getString(R.string.app_name);
    }

    @Override
    @Nullable
    public String getDelegateAppNameForOrigin(Origin origin, @ContentSettingsType int type) {
        if (type == ContentSettingsType.NOTIFICATIONS) {
            return InstalledWebappPermissionManager.get().getDelegateAppName(origin);
        }

        return null;
    }

    @Override
    @Nullable
    public String getDelegatePackageNameForOrigin(Origin origin, @ContentSettingsType int type) {
        if (type == ContentSettingsType.NOTIFICATIONS) {
            return InstalledWebappPermissionManager.get().getDelegatePackageName(origin);
        }

        return null;
    }

    @Override
    public boolean isHelpAndFeedbackEnabled() {
        return true;
    }

    @Override
    public void launchSettingsHelpAndFeedbackActivity(Activity currentActivity) {
        HelpAndFeedbackLauncherImpl.getInstance().show(currentActivity,
                currentActivity.getString(R.string.help_context_settings),
                Profile.getLastUsedRegularProfile(), null);
    }

    @Override
    public void launchProtectedContentHelpAndFeedbackActivity(Activity currentActivity) {
        HelpAndFeedbackLauncherImpl.getInstance().show(currentActivity,
                currentActivity.getString(R.string.help_context_protected_content),
                Profile.getLastUsedRegularProfile(), null);
    }

    @Override
    public Set<String> getOriginsWithInstalledApp() {
        WebappRegistry registry = WebappRegistry.getInstance();
        return registry.getOriginsWithInstalledApp();
    }

    @Override
    public Set<String> getAllDelegatedNotificationOrigins() {
        return InstalledWebappPermissionManager.get().getAllDelegatedOrigins();
    }

    @Override
    public void maybeDisplayPrivacySandboxSnackbar() {
        // Only show the snackbar when the Privacy Sandbox APIs are enabled.
        if (mPrivacySandboxController != null && PrivacySandboxBridge.isPrivacySandboxEnabled()
                && !PrivacySandboxBridge.isPrivacySandboxRestricted()) {
            mPrivacySandboxController.showSnackbar();
        }
    }

    @Override
    public void dismissPrivacySandboxSnackbar() {
        if (mPrivacySandboxController != null) {
            mPrivacySandboxController.dismissSnackbar();
        }
    }

    @Override
    public boolean canLaunchClearBrowsingDataDialog() {
        return true;
    }

    @Override
    public void launchClearBrowsingDataDialog(Activity currentActivity) {
        new SettingsLauncherImpl().launchSettingsActivity(
                currentActivity, ClearBrowsingDataTabsFragment.class);
    }
}

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

package org.chromium.components.adblock;

import android.content.Context;
import android.content.res.Resources;
import android.util.Log;
import android.webkit.URLUtil;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;

/**
 * @brief Main access point for java UI code to access Filer Engine.
 * It calls its native counter part also AdblockController. Stores
 * settings prefs.
 * It lives in UI thread on the browser process.
 */
public final class AdblockController {
    private static final String TAG = AdblockController.class.getSimpleName();
    private final static String ITEMS_DELIMITER = ",";

    private static AdblockController sInstance;

    private final Set<AdBlockedObserver> mOnAdBlockedObservers;
    private final Set<SubscriptionUpdateObserver> mSubscriptionUpdateObservers;

    private List<Subscription> mRecommendedSubscriptions;

    public static class FilterMatchCheckParams {
        private URL mUrl;
        private AdblockContentType mType;
        private URL mDocumentUrl;
        private String mSiteKey;

        public FilterMatchCheckParams(
                URL url, AdblockContentType type, URL documentUrl, String siteKey) {
            assert url != null;
            mUrl = url;
            mType = type;
            mDocumentUrl = documentUrl;
            mSiteKey = siteKey;
        }

        public URL url() {
            return mUrl;
        }

        public AdblockContentType type() {
            return mType;
        }

        public URL documentUrl() {
            return mDocumentUrl;
        }

        public String siteKey() {
            return mSiteKey;
        }
    }

    private AdblockController() {
        mRecommendedSubscriptions = new ArrayList<Subscription>();
        mOnAdBlockedObservers = new CopyOnWriteArraySet<>();
        mSubscriptionUpdateObservers = new CopyOnWriteArraySet<>();
    }

    /**
     * @return The singleton object.
     */
    public static AdblockController getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new AdblockController();
            AdblockControllerJni.get().bind(sInstance);
        }
        return sInstance;
    }

    public void setHasAdblockCountersObservers(boolean hasObservers) {
        nativeSetHasAdblockCountersObservers(hasObservers);
    }

    private native void nativeSetHasAdblockCountersObservers(boolean hasObservers);

    public interface AdBlockedObserver {
        /**
         * "Ad allowed" event for a request which would be blocked but there
         * was an allowlisting filter found.
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         */
        @UiThread
        void onAdAllowed(AdblockCounters.ResourceInfo info);

        /**
         * "Ad blocked" event for a request which was blocked.
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         */
        @UiThread
        void onAdBlocked(AdblockCounters.ResourceInfo info);

        /**
         * "Page allowed" event for an allowlisted domain (page).
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         */
        @UiThread
        void onPageAllowed(AdblockCounters.ResourceInfo info);

        /**
         * "Popup allowed" event for a popup which would be blocked but there
         * was an allowlisting filter found.
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         */
        @UiThread
        void onPopupAllowed(AdblockCounters.ResourceInfo info);

        /**
         * "Popup blocked" event for a popup which was blocked.
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         */
        @UiThread
        void onPopupBlocked(AdblockCounters.ResourceInfo info);
    }

    public interface SubscriptionUpdateObserver {
        @UiThread
        void onSubscriptionDownloaded(final URL url);
    }

    public static class Subscription {
        private URL mUrl;
        private String mTitle;

        public Subscription(URL url, String title) {
            this.mUrl = url;
            this.mTitle = title;
        }

        public String title() {
            return mTitle;
        }

        public URL url() {
            return mUrl;
        }

        @Override
        public boolean equals(Object object) {
            if (object == null) return false;
            if (getClass() != object.getClass()) return false;

            Subscription other = (Subscription) object;
            return url().equals(other.url());
        }
    }

    public enum ConnectionType {
        // All WiFi networks
        WIFI("wifi"),
        // Any connection
        ANY("any");

        private String mValue;

        private ConnectionType(String value) {
            this.mValue = value;
        }

        static public ConnectionType fromString(String val) {
            if (val.equals(ConnectionType.WIFI.getValue())) return ConnectionType.WIFI;

            assert val.equals(ConnectionType.ANY.getValue());
            return ConnectionType.ANY;
        }

        public String getValue() {
            return mValue;
        }
    }

    @UiThread
    public void setEnabled(boolean enabled) {
        AdblockControllerJni.get().setAdblockEnabled(enabled);
    }

    @UiThread
    public boolean isEnabled() {
        return AdblockControllerJni.get().isAdblockEnabled();
    }

    @UiThread
    public void setAcceptableAdsEnabled(boolean enabled) {
        AdblockControllerJni.get().setAcceptableAdsEnabled(enabled);
    }

    @UiThread
    public boolean isAcceptableAdsEnabled() {
        return AdblockControllerJni.get().isAcceptableAdsEnabled();
    }

    @UiThread
    public void setMoreOptionsEnabled(boolean enabled) {
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setBoolean(Pref.ADBLOCK_MORE_OPTIONS_ENABLED, enabled);
    }

    @UiThread
    public boolean areMoreOptionsEnabled() {
        return UserPrefs.get(Profile.getLastUsedRegularProfile())
                .getBoolean(Pref.ADBLOCK_MORE_OPTIONS_ENABLED);
    }

    @UiThread
    public ConnectionType getAllowedConnectionType() {
        // TODO(mpawlowski) call AdblockControllerJni, not read prefs directly
        return ConnectionType.fromString(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                 .getString(Pref.ADBLOCK_ALLOWED_CONNECTION_TYPE));
    }

    @UiThread
    public void setAllowedConnectionType(ConnectionType type) {
        // TODO(mpawlowski) call AdblockControllerJni, not write prefs directly
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(Pref.ADBLOCK_ALLOWED_CONNECTION_TYPE, type.getValue());
    }

    @UiThread
    public List<Subscription> getRecommendedSubscriptions(Context context) {
        if (mRecommendedSubscriptions.isEmpty()) {
            final Map<String, String> localeToTitle = getLocaleToTitleMap(context);
            String[] recommendedSubscriptions =
                    AdblockControllerJni.get().getRecommendedSubscriptions();
            for (String subscription : recommendedSubscriptions) {
                String title = null;
                String languages = AdblockControllerJni.get().getRecommendedSubscriptionLanguages(
                        subscription);
                if (languages != null && !languages.isEmpty()) {
                    final String[] separatedLanguages = languages.split(",");
                    for (String language : separatedLanguages) {
                        title = localeToTitle.get(language);
                        if (title != null && !title.isEmpty()) break;
                    }
                }
                title = title != null
                        ? title
                        : AdblockControllerJni.get().getRecommendedSubscriptionTitle(subscription);
                try {
                    Subscription s =
                            new Subscription(new URL(URLUtil.guessUrl(subscription)), title);
                    mRecommendedSubscriptions.add(s);
                } catch (MalformedURLException e) {
                    Log.e(TAG, "Error parsing url: " + subscription);
                }
            }
        }

        return mRecommendedSubscriptions;
    }

    @UiThread
    public boolean isRecommendedSubscriptionsAvailable() {
        return !mRecommendedSubscriptions.isEmpty();
    }

    @UiThread
    public void selectSubscription(final Subscription subscription) {
        AdblockControllerJni.get().selectSubscription(subscription.url().toString());
    }

    @UiThread
    public void unselectSubscription(final Subscription subscription) {
        AdblockControllerJni.get().unselectSubscription(subscription.url().toString());
    }

    @UiThread
    public List<Subscription> getSelectedSubscriptions() {
        String[] selectedSubscriptions = AdblockControllerJni.get().getSelectedSubscriptions();

        List<Subscription> recommended =
                getRecommendedSubscriptions(ContextUtils.getApplicationContext());
        List<Subscription> result = new ArrayList<Subscription>();
        for (String selectedSubscriptionURL : selectedSubscriptions) {
            int index = -1;
            try {
                Subscription selected = new Subscription(new URL(selectedSubscriptionURL), "");
                index = recommended.indexOf(selected);
            } catch (MalformedURLException e) {
                Log.e(TAG, "Invalid subscription URL: " + selectedSubscriptionURL);
            }
            if (index != -1) {
                result.add(recommended.get(index));
            }
        }
        return result;
    }

    @UiThread
    public String getSelectedSubscriptionVersion(final Subscription subscription) {
        return AdblockControllerJni.get().getSelectedSubscriptionVersion(
                subscription.url().toString());
    }

    @UiThread
    public void addCustomSubscription(final URL url) {
        AdblockControllerJni.get().addCustomSubscription(url.toString());
    }

    @UiThread
    public void removeCustomSubscription(final URL url) {
        AdblockControllerJni.get().removeCustomSubscription(url.toString());
    }

    @UiThread
    public List<URL> getCustomSubscriptions() {
        String[] subscriptions = AdblockControllerJni.get().getCustomSubscriptions();
        return transform(subscriptions);
    }

    @UiThread
    public void addAllowedDomain(final String domain) {
        String sanitizedDomain = sanitizeSite(domain);
        if (sanitizedDomain == null) return;
        AdblockControllerJni.get().addAllowedDomain(sanitizedDomain);
    }

    @UiThread
    public void removeAllowedDomain(final String domain) {
        AdblockControllerJni.get().removeAllowedDomain(domain);
    }

    @UiThread
    public List<String> getAllowedDomains() {
        List<String> allowedDomains = Arrays.asList(AdblockControllerJni.get().getAllowedDomains());
        Collections.sort(allowedDomains);
        return allowedDomains;
    }

    @UiThread
    public void addOnAdBlockedObserver(final AdBlockedObserver observer) {
        mOnAdBlockedObservers.add(observer);
        setHasAdblockCountersObservers(mOnAdBlockedObservers.size() != 0);
    }

    @UiThread
    public void removeOnAdBlockedObserver(final AdBlockedObserver observer) {
        mOnAdBlockedObservers.remove(observer);
        setHasAdblockCountersObservers(mOnAdBlockedObservers.size() != 0);
    }

    @UiThread
    public void addSubscriptionUpdateObserver(final SubscriptionUpdateObserver observer) {
        mSubscriptionUpdateObservers.add(observer);
    }

    @UiThread
    public void removeSubscriptionUpdateObserver(final SubscriptionUpdateObserver observer) {
        mSubscriptionUpdateObservers.remove(observer);
    }

    @UiThread
    public void composeFilterSuggestions(@NonNull final AdblockElement element,
            @NonNull final AdblockComposeFilterSuggestionsCallback callback) {
        AdblockControllerJni.get().composeFilterSuggestions(element, callback);
    }

    @UiThread
    public void addCustomFilter(final String filter) {
        AdblockControllerJni.get().addCustomFilter(filter);
    }

    @UiThread
    public void removeCustomFilter(final String filter) {
        AdblockControllerJni.get().removeCustomFilter(filter);
    }

    @UiThread
    public List<String> getCustomFilters() {
        return Arrays.asList(AdblockControllerJni.get().getCustomFilters());
    }

    private String sanitizeSite(String site) {
        // |site| is raw user input. We expect it to be either a domain or a URL.
        try {
            URL candidate = new URL(URLUtil.guessUrl(site));
            return candidate.getHost();
        } catch (java.net.MalformedURLException e) {
        }
        // Could not parse |site| as URL or domain.
        return null;
    }

    private List<URL> transform(String[] urls) {
        if (urls == null) return null;

        List<URL> result = new ArrayList<URL>();
        for (String url : urls) {
            try {
                result.add(new URL(URLUtil.guessUrl(url)));
            } catch (MalformedURLException e) {
                Log.e(TAG, "Error parsing url: " + url);
            }
        }

        return result;
    }

    private String[] transform(List<URL> urls) {
        if (urls == null) return null;

        String[] result = new String[urls.size()];
        for (int i = 0; i < urls.size(); ++i) {
            result[i] = urls.get(i).toString();
        }
        return result;
    }

    private Map<String, String> getLocaleToTitleMap(final Context context) {
        final Resources resources = context.getResources();
        final String[] locales =
                resources.getStringArray(R.array.fragment_adblock_general_locale_title);
        final String separator = resources.getString(R.string.fragment_adblock_general_separator);
        final Map<String, String> localeToTitle = new HashMap<>(locales.length);
        for (final String localeAndTitlePair : locales) {
            // in `String.split()` separator is a regexp, but we want to treat it as a string
            final int separatorIndex = localeAndTitlePair.indexOf(separator);
            final String locale = localeAndTitlePair.substring(0, separatorIndex);
            final String title = localeAndTitlePair.substring(separatorIndex + 1);
            localeToTitle.put(locale, title);
        }

        return localeToTitle;
    }

    @CalledByNative
    private void adMatchedCallback(final String requestUrl, boolean wasBlocked,
            final String[] parentFrameUrls, final String subscriptionUrl, final int contentType,
            final int tabId) {
        ThreadUtils.assertOnUiThread();
        final List<String> parentsList = Arrays.asList(parentFrameUrls);
        final AdblockCounters.ResourceInfo resourceInfo = new AdblockCounters.ResourceInfo(
                requestUrl, parentsList, subscriptionUrl, contentType, tabId);
        Log.d(TAG,
                "Adblock: adMatchedCallback() notifies " + mOnAdBlockedObservers.size()
                        + " listeners about " + resourceInfo.toString()
                        + (wasBlocked ? " getting blocked" : " being allowed"));
        for (final AdBlockedObserver observer : mOnAdBlockedObservers) {
            if (wasBlocked) {
                observer.onAdBlocked(resourceInfo);
            } else {
                observer.onAdAllowed(resourceInfo);
            }
        }
    }

    @CalledByNative
    private void pageAllowedCallback(
            final String requestUrl, final String subscriptionUrl, final int tabId) {
        ThreadUtils.assertOnUiThread();
        final AdblockCounters.ResourceInfo resourceInfo =
                new AdblockCounters.ResourceInfo(requestUrl, new ArrayList(), subscriptionUrl,
                        AdblockContentType.CONTENT_TYPE_OTHER.getValue(), tabId);
        Log.d(TAG,
                "Adblock: pageAllowedCallback() notifies " + mOnAdBlockedObservers.size()
                        + " listeners about " + resourceInfo.toString());
        for (final AdBlockedObserver observer : mOnAdBlockedObservers) {
            observer.onPageAllowed(resourceInfo);
        }
    }

    @CalledByNative
    private void popupMatchedCallback(final String requestUrl, boolean wasBlocked,
            final String openerUrl, final String subscription, final int tabId) {
        ThreadUtils.assertOnUiThread();
        final List<String> parentsList = Arrays.asList(openerUrl);
        final AdblockCounters.ResourceInfo resourceInfo =
                new AdblockCounters.ResourceInfo(requestUrl, parentsList, subscription,
                        AdblockContentType.CONTENT_TYPE_OTHER.getValue(), tabId);
        Log.d(TAG,
                "Adblock: popupMatchedCallback() notifies " + mOnAdBlockedObservers.size()
                        + " listeners about " + resourceInfo.toString()
                        + (wasBlocked ? " getting blocked" : " being allowed"));
        for (final AdBlockedObserver observer : mOnAdBlockedObservers) {
            if (wasBlocked) {
                observer.onPopupBlocked(resourceInfo);
            } else {
                observer.onPopupAllowed(resourceInfo);
            }
        }
    }

    @CalledByNative
    private void subscriptionUpdatedCallback(final String url) {
        ThreadUtils.assertOnUiThread();
        try {
            URL subscriptionUrl = new URL(URLUtil.guessUrl(url));
            for (final SubscriptionUpdateObserver observer : mSubscriptionUpdateObservers) {
                observer.onSubscriptionDownloaded(subscriptionUrl);
            }
        } catch (MalformedURLException e) {
            Log.e(TAG, "Error parsing subscription url: " + url);
        }
    }

    @NativeMethods
    interface Natives {
        void bind(AdblockController caller);
        boolean isAdblockEnabled();
        void setAdblockEnabled(boolean aa_enabled);
        boolean isAcceptableAdsEnabled();
        void setAcceptableAdsEnabled(boolean aa_enabled);
        void selectSubscription(String url);
        String[] getSelectedSubscriptions();
        void unselectSubscription(String url);
        void addCustomSubscription(String url);
        void removeCustomSubscription(String url);
        String[] getCustomSubscriptions();
        void addAllowedDomain(String domain);
        void removeAllowedDomain(String domain);
        String[] getAllowedDomains();
        String[] getRecommendedSubscriptions();
        String getRecommendedSubscriptionTitle(String subscription);
        String getRecommendedSubscriptionLanguages(String subscription);
        String getSelectedSubscriptionVersion(String subscription);
        void composeFilterSuggestions(
                AdblockElement element, AdblockComposeFilterSuggestionsCallback callback);
        void addCustomFilter(String filter);
        void removeCustomFilter(String filter);
        String[] getCustomFilters();
    }
}

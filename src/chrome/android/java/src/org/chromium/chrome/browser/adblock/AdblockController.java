/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.chrome.browser.adblock;

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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.adblock.AdblockContentType;
import org.chromium.chrome.browser.adblock.AdblockCounters;
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

import org.chromium.chrome.browser.rewards.RewardsAPIBridge;

/**
 * @brief Main access point for java UI code to access Filer Engine.
 * It calls its native counter part also AdblockController. Stores
 * settings prefs. Notifies observers  of events such as `onAdMatched`,
 * `onSubscriptionDownloaded` and `onRecommendedSubscriptionsAvailable`.
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
        private boolean mSpecificOnly;

        public FilterMatchCheckParams(URL url, AdblockContentType type, URL documentUrl,
                String siteKey, boolean specificOnly) {
            assert url != null;
            mUrl = url;
            mType = type;
            mDocumentUrl = documentUrl;
            mSiteKey = siteKey;
            mSpecificOnly = specificOnly;
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

        public boolean specificOnly() {
            return mSpecificOnly;
        }
    }

    public AdblockController() {
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

    public void bindInstance(AdblockController instance) {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = instance;
            AdblockControllerJni.get().bind(sInstance);
        }
    }

    public void setHasAdblockCountersObservers(boolean hasObservers) {
        nativeSetHasAdblockCountersObservers(hasObservers);
    }

    private native void nativeSetHasAdblockCountersObservers(boolean hasObservers);

    public interface AdBlockedObserver {
        /**
         * "Ad matched" event.
         *
         * It should not block the UI thread for too long.
         * @param info contains auxiliary information about the resource.
         * @param wasBlocked resource was blocked if true, allowed if false.
         */
        @UiThread
        void onAdMatched(AdblockCounters.ResourceInfo info, boolean wasBlocked);
    }

    public interface SubscriptionUpdateObserver {
        @UiThread
        void onSubscriptionDownloaded(final URL url);

        @UiThread
        void onRecommendedSubscriptionsAvailable();
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
        UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(Pref.ENABLE_ADBLOCK, enabled);
    }

    @UiThread
    public boolean isEnabled() {
        return UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(Pref.ENABLE_ADBLOCK);
    }

    @UiThread
    public void setAcceptableAdsEnabled(boolean enabled) {
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setBoolean(Pref.ENABLE_ACCEPTABLE_ADS, enabled);
    }

    @UiThread
    public boolean isAcceptableAdsEnabled() {
        return UserPrefs.get(Profile.getLastUsedRegularProfile())
                .getBoolean(Pref.ENABLE_ACCEPTABLE_ADS);
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
        return ConnectionType.fromString(UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                 .getString(Pref.ADBLOCK_ALLOWED_CONNECTION_TYPE));
    }

    @UiThread
    public void setAllowedConnectionType(ConnectionType type) {
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
    public void hasMatchingFilter(final FilterMatchCheckParams params,
            final AdblockMatchingFilterCheckCallback callback) {
        String docUrl = params.documentUrl() != null ? params.documentUrl().toString() : null;

        AdblockControllerJni.get().hasMatchingFilter(params.url().toString(),
                params.type().getValue(), docUrl, params.siteKey(), params.specificOnly(),
                callback);
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
            final String[] parentFrameUrls, final String[] subscriptions, final int contentType,
            final int tabId) {
        ThreadUtils.assertOnUiThread();
        final List<String> parentsList = Arrays.asList(parentFrameUrls);
        final List<String> subscriptionsList = Arrays.asList(subscriptions);
        final AdblockCounters.ResourceInfo resourceInfo = new AdblockCounters.ResourceInfo(
                requestUrl, parentsList, subscriptionsList, contentType, tabId);
        Log.d(TAG,
                "Adblock: adMatchedCallback() notifies " + mOnAdBlockedObservers.size()
                        + " listeners about " + resourceInfo.toString()
                        + (wasBlocked ? " getting blocked" : " being allowed"));
        for (final AdBlockedObserver observer : mOnAdBlockedObservers) {
            observer.onAdMatched(resourceInfo, wasBlocked);
        }

        if (wasBlocked) RewardsAPIBridge.getInstance().logAdBlocked();
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

    @CalledByNative
    private void recommendedSubscriptionsAvailableCallback() {
        ThreadUtils.assertOnUiThread();
        for (final SubscriptionUpdateObserver observer : mSubscriptionUpdateObservers) {
            observer.onRecommendedSubscriptionsAvailable();
        }
    }

    @NativeMethods
    interface Natives {
        void bind(AdblockController caller);
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
        void composeFilterSuggestions(
                AdblockElement element, AdblockComposeFilterSuggestionsCallback callback);
        void addCustomFilter(String filter);
        void removeCustomFilter(String filter);
        void hasMatchingFilter(String url, int contentType, String parentUrl, String sitekey,
                boolean specificOnly, AdblockMatchingFilterCheckCallback callback);
    }
}

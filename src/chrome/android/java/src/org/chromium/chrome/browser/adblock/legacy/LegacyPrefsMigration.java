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

package org.chromium.chrome.browser.adblock.legacy;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;
import android.webkit.URLUtil;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.adblock.AdblockController;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.user_prefs.UserPrefs;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;

/**
 * Migration process of legacy application preferences, stored as Android
 * shared preferences in versions of ABP Chromium prior to 85v2, to those
 * stored as Chromium preferences only.
 *
 * The preference for adblock being enabled was already handled as a Chromium
 * preference in previous versions, so even though it is also kept as a shared
 * preferences, this class is not responsible for migrating them.
 * The allowed domains preference had a different name (whitelisted) in previous
 * version. So even though it also was a Chromium preference, it needs to be
 * handled by the migration.
 */
public class LegacyPrefsMigration {

  private static final String TAG = LegacyPrefsMigration.class.getSimpleName();

  private SharedPrefsStorage mLegacyPrefsStorage;

  private AdblockController mAdblockController;

  /**
   * Migrate preferences from previous versions using libabp-android on
   * the first run after an install/upgrade.
   */
  public static void migrateLegacyPreferences() {
     if (ProfileManager.isInitialized()) {
       doMigrateLegacyPreferences();
     } else {
       ProfileManager.addObserver(new ProfileManager.Observer() {
         @Override
         public void onProfileAdded(final Profile profile) {
           ProfileManager.removeObserver(this);
           doMigrateLegacyPreferences();
         }
         @Override
         public void onProfileDestroyed(final Profile profile) {
         }
       });
     }
  }

  private static void doMigrateLegacyPreferences() {
     if (UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(
             Pref.ADBLOCK_PREFS_MIGRATION_ATTEMPT_REQUIRED)) {
       final String prefsPrefix = "ADBLOCK";
       SharedPreferences preferences = ContextUtils.getApplicationContext().getSharedPreferences(
           prefsPrefix, Context.MODE_PRIVATE);

       LegacyPrefsMigration legacyPrefsMigration =
           new LegacyPrefsMigration(preferences, AdblockController.getInstance());
       legacyPrefsMigration.migrate();

       // Do not try to migrate preferences again
       UserPrefs.get(Profile.getLastUsedRegularProfile()).setBoolean(
           Pref.ADBLOCK_PREFS_MIGRATION_ATTEMPT_REQUIRED, false);
     }
   }

  public LegacyPrefsMigration(SharedPreferences legacyPrefs, AdblockController adblockController) {
      this.mLegacyPrefsStorage = new SharedPrefsStorage(legacyPrefs);
      this.mAdblockController = adblockController;
  }

  public void migrate() {
      AdblockSettings legacySettings = mLegacyPrefsStorage.load();
      if (legacySettings != null) {
          Log.i(TAG, "[ABP] Migrating preferences from previous version...");

          mAdblockController.setAcceptableAdsEnabled(legacySettings.isAcceptableAdsEnabled());

          migrateAllowedConnectionType(legacySettings.getAllowedConnectionType());

          List<String> legacySubscriptions = legacySettings.getSubscriptions();
          if (legacySubscriptions != null) {
              migrateSubscriptions(legacySubscriptions);
          }

          final List<String> allowedDomains = legacySettings.getAllowedDomains();
          if (allowedDomains != null) {
              migrateAllowedDomains(allowedDomains);
          }

          Log.i(TAG, "[ABP] Preferences from previous version migrated successfully");
          Log.d(TAG, "[ABP] Preference - Adblock enabled: " + mAdblockController.isEnabled());
          Log.d(TAG,
                  "[ABP] Preference - AA enabled: " + mAdblockController.isAcceptableAdsEnabled());
          Log.d(TAG,
                  "[ABP] Preference - Connection type: "
                          + mAdblockController.getAllowedConnectionType());

          int subscriptionsSize = mAdblockController.getSelectedSubscriptions() != null
                  ? mAdblockController.getSelectedSubscriptions().size()
                  : 0;
          Log.d(TAG, "[ABP] Preference - No. of subscriptions: " + subscriptionsSize);

          int allowedSize = mAdblockController.getAllowedDomains() != null
                  ? mAdblockController.getAllowedDomains().size()
                  : 0;
          Log.d(TAG, "[ABP] Preference - No. of allowed domains: " + allowedSize);
      }
  }

  /**
   * Migrates a legacy allowed connection type. Since the legacy and new ones
   * are not totally equivalent, the migration falls on the safe side to prevent
   * additional data charges to the user.
   *
   * @param legacyConnectionType Connection type to be migrated
   */
  private void migrateAllowedConnectionType(ConnectionType legacyConnectionType) {
      switch (legacyConnectionType) {
        case WIFI:
        case WIFI_NON_METERED:
            mAdblockController.setAllowedConnectionType(AdblockController.ConnectionType.WIFI);
            break;
        case ANY:
            mAdblockController.setAllowedConnectionType(AdblockController.ConnectionType.ANY);
            break;
      }
  }

  /**
   * Clears the default subscriptions set during the browser initialization, so
   * they are not combined with the legacy preferences. Then registers the
   * legacy ones in the subscriptions list.
   *
   * @param legacySubscriptions Subscriptions to be migrated
   */
  private void migrateSubscriptions(List<String> legacySubscriptions) {
    // Clear default subscriptions
    for (final AdblockController.Subscription subscription : mAdblockController.getSelectedSubscriptions()) {
        mAdblockController.unselectSubscription(subscription);
    }

    // Add legacy subscriptions
    for (String subscription : legacySubscriptions) {
      try {
        final URL url = new URL(URLUtil.guessUrl(subscription));
        mAdblockController.selectSubscription(new AdblockController.Subscription(url, ""));
      } catch (MalformedURLException e) {
        Log.e(TAG, "[ABP] Error parsing url: " + subscription);
      }
    }
  }

  /**
   * Migrates a list of allowed domains, because the preference has been renamed
   * since older versions.
   */
  private void migrateAllowedDomains(List<String> legacyAllowedDomains) {
    for (String domain : legacyAllowedDomains) {
        mAdblockController.addAllowedDomain(domain);
    }
  }
}

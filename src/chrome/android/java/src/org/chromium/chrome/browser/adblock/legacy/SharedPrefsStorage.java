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

import android.content.SharedPreferences;

import java.util.LinkedList;
import java.util.List;

/**
 * Class to simplify the retrieval of legacy application preferences, stored as
 * Android shared preferences in versions of ABP Chromium prior to 85v2.
 *
 * It is ported and simplified from libadblockplus-android, which this
 * project no longer depends on; it would be overkill to include the whole
 * library just for a couple of classes. The hash of the commit used as
 * reference is ff59e534cb63b83eff55faae6ff400f484400cc8
 */
public class SharedPrefsStorage {

  private static final String SETTINGS_ENABLED_KEY = "enabled";
  private static final String SETTINGS_AA_ENABLED_KEY = "aa_enabled";
  private static final String SETTINGS_SUBSCRIPTIONS_KEY = "subscriptions";
  private static final String SETTINGS_SUBSCRIPTION_KEY = "subscription";
  private static final String SETTINGS_SUBSCRIPTION_URL_KEY = "url";
  private static final String SETTINGS_SUBSCRIPTION_PREFIXES_KEY = "prefixes";
  private static final String SETTINGS_SUBSCRIPTION_TITLE_KEY = "title";
  private static final String SETTINGS_ALLOWED_DOMAINS_KEY = "whitelisted_domains";
  private static final String SETTINGS_ALLOWED_DOMAIN_KEY = "domain";
  private static final String SETTINGS_ALLOWED_CONNECTION_TYPE_KEY = "allowed_connection_type";

  private SharedPreferences prefs;

  public SharedPrefsStorage(SharedPreferences prefs) {
    this.prefs = prefs;
  }

  /**
   * Load settings from the storage
   *
   * Warning: can return null if not saved yet
   * Warning: subscriptions can have `url` and `title` only to be identified among available in AdblockEngine
   *
   * @return AdblockSettings instance or null
   */
  public AdblockSettings load() {
    if (!prefs.contains(SETTINGS_ENABLED_KEY)) {
      // settings were not saved yet
      return null;
    }

    AdblockSettings settings = new AdblockSettings();
    settings.setAdblockEnabled(prefs.getBoolean(SETTINGS_ENABLED_KEY, true));
    settings.setAcceptableAdsEnabled(prefs.getBoolean(SETTINGS_AA_ENABLED_KEY, true));
    String connectionType = prefs.getString(
      SETTINGS_ALLOWED_CONNECTION_TYPE_KEY, ConnectionType.ANY.getValue());
    settings.setAllowedConnectionType(ConnectionType.findByValue(connectionType));

    loadSubscriptions(settings);
    loadAllowedDomains(settings);

    return settings;
  }

  private void loadAllowedDomains(AdblockSettings settings) {
    if (prefs.contains(SETTINGS_ALLOWED_DOMAINS_KEY)) {
      // count
      int allowedDomainsCount = prefs.getInt(SETTINGS_ALLOWED_DOMAINS_KEY, 0);

      // each domain
      List<String> allowedDomains = new LinkedList<>();
      for (int i = 0; i < allowedDomainsCount; i++) {
        String allowedDomain = prefs.getString(getArrayItemKey(i, SETTINGS_ALLOWED_DOMAIN_KEY), "");
        allowedDomains.add(allowedDomain);
      }
      settings.setAllowedDomains(allowedDomains);
    }
  }

  private void loadSubscriptions(AdblockSettings settings) {
    if (prefs.contains(SETTINGS_SUBSCRIPTIONS_KEY)) {
      // count
      int subscriptionsCount = prefs.getInt(SETTINGS_SUBSCRIPTIONS_KEY, 0);

      // each subscription
      List<String> subscriptions = new LinkedList<>();
      for (int i = 0; i < subscriptionsCount; i++) {
        String subscription = prefs.getString(getSubscriptionURLKey(i), "");
        subscriptions.add(subscription);
      }
      settings.setSubscriptions(subscriptions);
    }
  }

  private String getArrayItemKey(int index, String entity) {
    // f.e. "domain0"
    return entity + index;
  }

  private String getArrayItemKey(int index, String entity, String field) {
    // f.e. `subscription0.field`
    return getArrayItemKey(index, entity) + "." + field;
  }

  private String getSubscriptionURLKey(int index) {
    return getArrayItemKey(index, SETTINGS_SUBSCRIPTION_KEY, SETTINGS_SUBSCRIPTION_URL_KEY);
  }

}

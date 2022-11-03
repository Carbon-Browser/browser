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

import java.io.Serializable;
import java.util.List;

/**
 * Legacy application preferences, stored as Android shared preferences in
 * versions of ABP Chromium prior to 85v2.
 *
 * These are ported and simplified from libadblockplus-android, which this
 * project no longer depends on; it would be overkill to include the whole
 * library just for a couple of classes. The hash of the commit used as
 * reference is ff59e534cb63b83eff55faae6ff400f484400cc8
 */
public class AdblockSettings {
    private boolean mAdblockEnabled;
    private boolean mAcceptableAdsEnabled;
    private List<String> mSubscriptions;
    private List<String> mAllowedDomains;
    private ConnectionType mAllowedConnectionType;

    public boolean isAdblockEnabled() {
        return mAdblockEnabled;
    }

  public void setAdblockEnabled(boolean adblockEnabled) {
      this.mAdblockEnabled = adblockEnabled;
  }

  public boolean isAcceptableAdsEnabled() {
      return mAcceptableAdsEnabled;
  }

  public void setAcceptableAdsEnabled(boolean acceptableAdsEnabled) {
      this.mAcceptableAdsEnabled = acceptableAdsEnabled;
  }

  public List<String> getSubscriptions() {
      return mSubscriptions;
  }

  public void setSubscriptions(List<String> subscriptions) {
      this.mSubscriptions = subscriptions;
  }

  public List<String> getAllowedDomains() {
      return mAllowedDomains;
  }

  public void setAllowedDomains(List<String> allowedDomains) {
      this.mAllowedDomains = allowedDomains;
  }

  public ConnectionType getAllowedConnectionType() {
      return mAllowedConnectionType;
  }

  public void setAllowedConnectionType(ConnectionType allowedConnectionType) {
      this.mAllowedConnectionType = allowedConnectionType;
  }

  @Override
  public String toString() {
      return "AdblockSettings{"
              + "adblockEnabled=" + mAdblockEnabled
              + ", acceptableAdsEnabled=" + mAcceptableAdsEnabled + ", subscriptions:"
              + (mSubscriptions != null ? mSubscriptions.size() : 0) + ", allowedDomains:"
              + (mAllowedDomains != null ? mAllowedDomains.size() : 0) + ", allowedConnectionType="
              + (mAllowedConnectionType != null ? mAllowedConnectionType.getValue() : "null") + '}';
  }
}

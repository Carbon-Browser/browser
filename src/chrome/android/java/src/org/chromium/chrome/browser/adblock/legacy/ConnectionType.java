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

public enum ConnectionType {

    // All WiFi networks
    WIFI("wifi"),

    // Non-metered WiFi networks
    WIFI_NON_METERED("wifi_non_metered"),

    // Any connection
    ANY("any");

    private String value;

    ConnectionType(String value) {
      this.value = value;
    }

    public String getValue() {
      return value;
    }

    public static ConnectionType findByValue(String value) {
      if (value == null) {
        return null;
      }

      for (ConnectionType eachConnectionType : ConnectionType.values()) {
        if (eachConnectionType.getValue().equals(value)) {
          return eachConnectionType;
        }
      }

      // not found
      return null;
    }
  }

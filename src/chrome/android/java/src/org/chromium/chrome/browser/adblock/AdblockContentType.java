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

import java.util.HashMap;
import java.util.Map;

public enum AdblockContentType {
    // Note. This has to be kept in sync with c++ enum so some values
    // are skipped
    CONTENT_TYPE_OTHER(1 << 0),
    CONTENT_TYPE_SCRIPT(1 << 1),
    CONTENT_TYPE_IMAGE(1 << 2),
    CONTENT_TYPE_STYLESHEET(1 << 3),
    CONTENT_TYPE_OBJECT(1 << 4),
    CONTENT_TYPE_SUBDOCUMENT(1 << 5),
    CONTENT_TYPE_WEBSOCKET(1 << 7),
    CONTENT_TYPE_WEBRTC(1 << 8),
    CONTENT_TYPE_PING(1 << 10),
    CONTENT_TYPE_XMLHTTPREQUEST(1 << 11),
    CONTENT_TYPE_MEDIA(1 << 14),
    CONTENT_TYPE_FONT(1 << 15),
    CONTENT_TYPE_POPUP(1 << 24),
    CONTENT_TYPE_CSP(1 << 25),     // unused
    CONTENT_TYPE_HEADER(1 << 26),  // unused
    CONTENT_TYPE_DOCUMENT(1 << 27),
    CONTENT_TYPE_GENERICBLOCK(1 << 28),
    CONTENT_TYPE_ELEMHIDE(1 << 29),
    CONTENT_TYPE_GENERICHIDE(1 << 30);

    private final int contentType;

    private AdblockContentType(int contentType) {
        this.contentType = contentType;
    }

    private static final Map<Integer, AdblockContentType> intToContentTypeMap =
            new HashMap<Integer, AdblockContentType>();

    static {
        for (AdblockContentType type : AdblockContentType.values()) {
            intToContentTypeMap.put(type.contentType, type);
        }
    }

    public static AdblockContentType fromInt(int i) {
        AdblockContentType type = intToContentTypeMap.get(Integer.valueOf(i));
        return type;
    }

    public int getValue() {
        return contentType;
    }
}

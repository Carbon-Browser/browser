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

import org.chromium.chrome.browser.adblock.AdblockContentType;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AdblockCounters {
    /**
     * Immutable data-class containing an auxiliary information about resource event.
     */
    public static class ResourceInfo {
        private final String mRequestUrl;
        private final List<String> mParentFrameUrls;
        private final List<String> mSubscriptions;
        private final AdblockContentType mAdblockContentType;
        private final int mTabId;

        ResourceInfo(final String requestUrl, final List<String> parentFrameUrls,
                final List<String> subscriptions, final int adblockContentType, final int tabId) {
            this.mRequestUrl = requestUrl;
            this.mParentFrameUrls = parentFrameUrls;
            this.mSubscriptions = subscriptions;
            this.mAdblockContentType = AdblockContentType.fromInt(adblockContentType);
            this.mTabId = tabId;
        }

        /**
         * @return The request which was blocked or allowed.
         */
        public String getRequestUrl() {
            return mRequestUrl;
        }

        /**
         * @return The parent frames of the mRequestUrl.
         */
        public List<String> getParentFrameUrls() {
            return mParentFrameUrls;
        }

        public List<String> getSubscriptions() {
            return mSubscriptions;
        }

        /**
         * @return The ABP content type. See enum ContentType in
         *         third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h.
         */
        public AdblockContentType getAdblockContentType() {
            return mAdblockContentType;
        }

        /**
         * @return The current tab id for the mRequestUrl. -1 means no tab id, likely tab closed
         *         before event arrived. Numbers start from 0.
         */
        public int getTabId() {
            return mTabId;
        }

        @Override
        public String toString() {
            return "mRequestUrl: " + mRequestUrl + ", mParentFrameUrls: "
                    + mParentFrameUrls.toString() + ", mSubscriptions:" + mSubscriptions.toString()
                    + ", mAdblockContentType: " + mAdblockContentType.getValue()
                    + ", mTabId: " + mTabId;
        }
    }
}

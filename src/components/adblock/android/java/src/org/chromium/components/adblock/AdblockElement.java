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

import org.chromium.base.annotations.CalledByNative;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Map;

/**
  * Simplified DOM Element.
  * Used for generating filter suggestions for web page element.
  */
public class AdblockElement {
    private final String mName;
    private final String mUrl;
    private final Map<String, String> mAttributes;
    private final AdblockElement[] mChildren;

    AdblockElement(@NonNull final String name, @NonNull final String url,
            final Map<String, String> attributes) {
        this(name, url, attributes, null);
    }

    AdblockElement(@NonNull final String name, @NonNull final String url,
            final Map<String, String> attributes, final AdblockElement[] children) {
        this.mName = name;
        this.mUrl = url;
        this.mAttributes = attributes;
        this.mChildren = children;
    }

    /**
     * @return Local part of the qualified name of the element.
     */
    @CalledByNative
    @NonNull
    public String getLocalName() {
        return mName;
    }

    /**
      * @return Containing document url.
      */
    @CalledByNative
    @NonNull
    public String getDocumentLocation() {
        return mUrl;
    }

    /**
      * @return Retrieves the value of the named attribute from the current node.
      */
    @CalledByNative
    @Nullable
    public String getAttribute(final String name) {
        return mAttributes.get(name);
    }

    /**
      * @return Collection of child elements.
      */
    @CalledByNative
    @Nullable
    public AdblockElement[] getChildren() {
        return mChildren;
    }
};

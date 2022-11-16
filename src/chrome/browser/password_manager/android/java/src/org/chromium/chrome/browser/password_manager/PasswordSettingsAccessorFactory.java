// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_manager;

import static org.chromium.base.ThreadUtils.assertOnUiThread;

import androidx.annotation.VisibleForTesting;

/**
 * This factory returns an implementation for the password settings accessor. The factory itself is
 * also implemented downstream.
 */
public abstract class PasswordSettingsAccessorFactory {
    private static PasswordSettingsAccessorFactory sInstance;

    protected PasswordSettingsAccessorFactory() {}

    /**
     * Returns an accessor factory to be invoked whenever {@link #createAccessor()} is called. If no
     * factory was used yet, it is created.
     *
     * @return The shared {@link PasswordSettingsAccessorFactory} instance.
     */
    public static PasswordSettingsAccessorFactory getOrCreate() {
        assertOnUiThread();
        if (sInstance == null) {
            sInstance = new PasswordSettingsAccessorFactoryImpl();
        }
        return sInstance;
    }

    /**
     * Returns the downstream implementation provided by subclasses.
     *
     * @return An implementation of the {@link PasswordSettingsAccessor} if one exists.
     */
    public PasswordSettingsAccessor createAccessor() {
        return null;
    }

    public boolean canCreateAccessor() {
        return false;
    }

    @VisibleForTesting
    public static void setupFactoryForTesting(PasswordSettingsAccessorFactory accessorFactory) {
        sInstance = accessorFactory;
    }
}

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_manager.tests.utils;

import org.chromium.chrome.browser.password_manager.PasswordStoreAndroidBackend;
import org.chromium.chrome.browser.password_manager.PasswordStoreAndroidBackendFactory;

/**
 * The factory for creating fake {@link PasswordStoreAndroidBackend} to be used in integration
 * tests.
 */
public class FakePasswordStoreAndroidBackendFactoryImpl extends PasswordStoreAndroidBackendFactory {
    private PasswordStoreAndroidBackend mPasswordStoreAndroidBackend;

    /**
     * Returns the downstream implementation provided by subclasses.
     *
     * @return A non-null implementation of the {@link PasswordStoreAndroidBackend}.
     */
    @Override
    public PasswordStoreAndroidBackend createBackend() {
        if (mPasswordStoreAndroidBackend == null) {
            mPasswordStoreAndroidBackend = new FakePasswordStoreAndroidBackend();
        }
        return mPasswordStoreAndroidBackend;
    }

    /**
     * Returns whether a down-stream implementation can be instantiated.
     *
     * @return True iff all preconditions for using the down-steam implementations are fulfilled.
     */
    @Override
    public boolean canCreateBackend() {
        return true;
    }
}

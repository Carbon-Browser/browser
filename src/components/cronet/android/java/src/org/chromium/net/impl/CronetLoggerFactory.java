// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.impl;

import android.content.Context;

import androidx.annotation.Nullable;

import org.chromium.net.impl.CronetLogger.CronetSource;

/**
 * Takes care of instantiating the correct CronetLogger.
 */
public final class CronetLoggerFactory {
    private static final int SAMPLE_RATE_PER_SECOND = 1;

    private CronetLoggerFactory() {}

    private static final CronetLogger sDefaultLogger = new NoOpLogger();
    private static CronetLogger sTestingLogger;

    // Class that is packaged for Cronet telemetry.
    private static final String CRONET_LOGGER_IMPL_CLASS =
            "com.google.net.cronet.telemetry.CronetLoggerImpl";

    /**
     * @return The correct CronetLogger to be used for logging.
     */
    public static CronetLogger createLogger(Context ctx, CronetSource source) {
        if (sTestingLogger != null) return sTestingLogger;

        if (!CronetManifest.isAppOptedInForTelemetry(ctx, source)) return sDefaultLogger;

        Class<? extends CronetLogger> cronetLoggerImplClass = fetchLoggerImplClass();
        if (cronetLoggerImplClass == null) return sDefaultLogger;

        try {
            return cronetLoggerImplClass.getConstructor(int.class).newInstance(
                    SAMPLE_RATE_PER_SECOND);
        } catch (Exception e) {
            // Pass - since we dont want any failure, catch any exception that might arise.
        }
        return sDefaultLogger;
    }

    private static void setLoggerForTesting(@Nullable CronetLogger testingLogger) {
        sTestingLogger = testingLogger;
    }

    /**
     * Utility class to safely use a custom CronetLogger for the duration of a test.
     * To be used within a try-with-resources statement within the test.
     */
    public static class SwapLoggerForTesting implements AutoCloseable {
        /**
         * Forces {@code CronetLoggerFactory#createLogger} to return @param testLogger instead of
         * what it would have normally returned.
         */
        public SwapLoggerForTesting(CronetLogger testLogger) {
            CronetLoggerFactory.setLoggerForTesting(testLogger);
        }

        /**
         * Restores CronetLoggerFactory to its original state.
         */
        @Override
        public void close() {
            CronetLoggerFactory.setLoggerForTesting(null);
        }
    }

    private static Class<? extends CronetLogger> fetchLoggerImplClass() {
        ClassLoader loader = CronetLoggerFactory.class.getClassLoader();
        try {
            return loader.loadClass(CRONET_LOGGER_IMPL_CLASS).asSubclass(CronetLogger.class);
        } catch (ClassNotFoundException e) {
            return null;
        }
    }
}

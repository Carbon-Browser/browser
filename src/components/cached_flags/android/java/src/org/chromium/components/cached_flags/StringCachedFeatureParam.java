// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.cached_flags;

import android.content.SharedPreferences;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;

import org.chromium.base.FeatureList;
import org.chromium.base.FeatureMap;
import org.chromium.base.FeatureOverrides;
import org.chromium.base.cached_flags.ValuesReturned;
import org.chromium.base.supplier.Supplier;

/** A String-type {@link CachedFeatureParam}. */
public class StringCachedFeatureParam extends CachedFeatureParam<String> {
    private Supplier<String> mValueSupplier;

    public StringCachedFeatureParam(
            FeatureMap featureMap,
            String featureName,
            String variationName,
            @NonNull String defaultValue) {
        super(featureMap, featureName, variationName, FeatureParamType.STRING, defaultValue);
    }

    /**
     * @return the value of the feature parameter that should be used in this run.
     */
    @AnyThread
    public String getValue() {
        CachedFlagsSafeMode.getInstance().onFlagChecked();

        String testValue = FeatureList.getTestValueForFieldTrialParam(mFeatureName, mParamName);
        if (testValue != null) {
            return testValue;
        }

        return ValuesReturned.getReturnedOrNewStringValue(
                getSharedPreferenceKey(), getValueSupplier());
    }

    private Supplier<String> getValueSupplier() {
        if (mValueSupplier == null) {
            mValueSupplier =
                    () -> {
                        String preferenceName = getSharedPreferenceKey();
                        String value =
                                CachedFlagsSafeMode.getInstance()
                                        .getStringFeatureParam(preferenceName, mDefaultValue);
                        if (value == null) {
                            value =
                                    CachedFlagsSharedPreferences.getInstance()
                                            .readString(preferenceName, mDefaultValue);
                        }
                        return value;
                    };
        }
        return mValueSupplier;
    }

    public String getDefaultValue() {
        return mDefaultValue;
    }

    @Override
    void writeCacheValueToEditor(final SharedPreferences.Editor editor) {
        final String value = mFeatureMap.getFieldTrialParamByFeature(getFeatureName(), getName());
        editor.putString(getSharedPreferenceKey(), value.isEmpty() ? getDefaultValue() : value);
    }

    /**
     * Forces the parameter to return a specific value for testing.
     *
     * @param overrideValue the value to be returned
     */
    public void setForTesting(String overrideValue) {
        FeatureOverrides.overrideParam(getFeatureName(), getName(), overrideValue);
    }
}

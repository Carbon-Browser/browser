// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices.permissiondelegation;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.robolectric.Shadows.shadowOf;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.LooperMode;
import org.robolectric.shadows.ShadowPackageManager;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.JniMocker;
import org.chromium.chrome.browser.browserservices.TrustedWebActivityClient;
import org.chromium.chrome.browser.browserservices.metrics.TrustedWebActivityUmaRecorder;
import org.chromium.components.content_settings.ContentSettingValues;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.components.embedder_support.util.Origin;

/**
 * Tests for {@link LocationPermissionUpdater}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@LooperMode(LooperMode.Mode.LEGACY)
public class LocationPermissionUpdaterTest {
    private static final Origin ORIGIN = Origin.create("https://www.website.com");
    private static final String PACKAGE_NAME = "com.package.name";
    private static final String OTHER_PACKAGE_NAME = "com.other.package.name";
    private static final long CALLBACK = 12;

    @Rule
    public JniMocker mocker = new JniMocker();

    @Mock
    public InstalledWebappPermissionManager mPermissionManager;
    @Mock
    public TrustedWebActivityClient mTrustedWebActivityClient;
    @Mock
    public TrustedWebActivityUmaRecorder mUmaRecorder;

    @Mock
    private InstalledWebappBridge.Natives mNativeMock;

    private LocationPermissionUpdater mLocationPermissionUpdater;
    private ShadowPackageManager mShadowPackageManager;

    @ContentSettingValues
    private int mLocationPermission;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mocker.mock(InstalledWebappBridgeJni.TEST_HOOKS, mNativeMock);

        PackageManager pm = RuntimeEnvironment.application.getPackageManager();
        mShadowPackageManager = shadowOf(pm);
        mLocationPermissionUpdater = new LocationPermissionUpdater(
                mPermissionManager, mTrustedWebActivityClient, mUmaRecorder);
        installBrowsableIntentHandler(ORIGIN, PACKAGE_NAME);
    }

    @Test
    @Feature("TrustedWebActivities")
    public void disablesLocation_whenClientLocationAreDisabled() {
        installTrustedWebActivityService(ORIGIN, PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.BLOCK);

        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);

        verifyPermissionUpdated(ContentSettingValues.BLOCK);
    }

    @Test
    @Feature("TrustedWebActivities")
    public void enablesLocation_whenClientLocationAreEnabled() {
        installTrustedWebActivityService(ORIGIN, PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.ALLOW);

        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);

        verifyPermissionUpdated(ContentSettingValues.ALLOW);
    }

    @Test
    @Feature("TrustedWebActivities")
    public void updatesPermission_onSubsequentCalls() {
        installTrustedWebActivityService(ORIGIN, PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.ALLOW);
        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);
        verifyPermissionUpdated(ContentSettingValues.ALLOW);

        setLocationPermissionForClient(ContentSettingValues.BLOCK);
        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);
        verifyPermissionUpdated(ContentSettingValues.BLOCK);
    }

    @Test
    @Feature("TrustedWebActivities")
    public void updatesPermission_onNewClient() {
        installTrustedWebActivityService(ORIGIN, PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.ALLOW);
        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);
        verifyPermissionUpdated(ContentSettingValues.ALLOW);

        installBrowsableIntentHandler(ORIGIN, OTHER_PACKAGE_NAME);
        installTrustedWebActivityService(ORIGIN, OTHER_PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.BLOCK);
        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);
        verifyPermissionUpdated(OTHER_PACKAGE_NAME, ContentSettingValues.BLOCK);
    }

    @Test
    @Feature("TrustedWebActivities")
    public void unregisters_onClientUninstall() {
        installTrustedWebActivityService(ORIGIN, PACKAGE_NAME);
        setLocationPermissionForClient(ContentSettingValues.ALLOW);

        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);

        uninstallTrustedWebActivityService(ORIGIN);
        mLocationPermissionUpdater.onClientAppUninstalled(ORIGIN);

        verifyPermissionReset();
    }

    /** "Installs" the given package to handle intents for that origin. */
    private void installBrowsableIntentHandler(Origin origin, String packageName) {
        Intent intent = new Intent();
        intent.setPackage(packageName);
        intent.setData(origin.uri());
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);

        mShadowPackageManager.addResolveInfoForIntent(intent, new ResolveInfo());
    }

    /** "Installs" a Trusted Web Activity Service for the origin. */
    @SuppressWarnings("unchecked")
    private void installTrustedWebActivityService(Origin origin, String packageName) {
        doAnswer(invocation -> {
            TrustedWebActivityClient.PermissionCallback callback = invocation.getArgument(1);
            callback.onPermission(new ComponentName(packageName, "FakeClass"), mLocationPermission);
            return true;
        }).when(mTrustedWebActivityClient).checkLocationPermission(eq(origin), any());
    }

    private void uninstallTrustedWebActivityService(Origin origin) {
        doAnswer(invocation -> {
            TrustedWebActivityClient.PermissionCallback callback = invocation.getArgument(1);
            callback.onNoTwaFound();
            return true;
        }).when(mTrustedWebActivityClient).checkLocationPermission(eq(origin), any());
    }

    private void setLocationPermissionForClient(@ContentSettingValues int settingValue) {
        mLocationPermission = settingValue;
    }

    private void verifyPermissionUpdated(@ContentSettingValues int settingValue) {
        verifyPermissionUpdated(PACKAGE_NAME, settingValue);
    }

    private void verifyPermissionUpdated(
            String packageName, @ContentSettingValues int settingValue) {
        verify(mPermissionManager)
                .updatePermission(eq(ORIGIN), eq(packageName), eq(ContentSettingsType.GEOLOCATION),
                        eq(settingValue));
        verify(mNativeMock).runPermissionCallback(eq(CALLBACK), eq(settingValue));
    }

    private void verifyPermissionReset() {
        verify(mPermissionManager)
                .resetStoredPermission(eq(ORIGIN), eq(ContentSettingsType.GEOLOCATION));
    }

    private void verifyPermissionNotReset() {
        verify(mPermissionManager, never())
                .resetStoredPermission(eq(ORIGIN), eq(ContentSettingsType.GEOLOCATION));
    }

    @Test
    @Feature("TrustedWebActivity")
    public void updatesPermissionOnlyOnce_incorrectReturnsFromTwaService() {
        doAnswer(invocation -> {
            TrustedWebActivityClient.PermissionCallback callback = invocation.getArgument(1);
            // PermissionCallback is invoked twice with different result.
            callback.onPermission(
                    new ComponentName(PACKAGE_NAME, "FakeClass"), ContentSettingValues.BLOCK);
            callback.onPermission(
                    new ComponentName(PACKAGE_NAME, "FakeClass"), ContentSettingValues.ALLOW);
            return true;
        }).when(mTrustedWebActivityClient).checkLocationPermission(eq(ORIGIN), any());

        mLocationPermissionUpdater.checkPermission(ORIGIN, CALLBACK);
        verifyPermissionUpdated(PACKAGE_NAME, ContentSettingValues.BLOCK);
    }
}

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.resources.dynamics;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.MockitoAnnotations.initMocks;

import static org.chromium.base.GarbageCollectionTestUtils.canBeGarbageCollected;

import android.graphics.Bitmap;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.JniMocker;
import org.chromium.ui.resources.ResourceFactory;
import org.chromium.ui.resources.ResourceFactoryJni;

import java.lang.ref.WeakReference;

/**
 * Tests for {@link BitmapDynamicResource}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BitmapDynamicResourceTest {
    private BitmapDynamicResource mResource;

    @Rule
    public JniMocker mJniMocker = new JniMocker();
    @Mock
    private ResourceFactory.Natives mResourceFactoryJni;

    @Before
    public void setup() {
        initMocks(this);
        mJniMocker.mock(ResourceFactoryJni.TEST_HOOKS, mResourceFactoryJni);
        mResource = new BitmapDynamicResource(1);
    }

    @Test
    public void testGetBitmap() {
        Bitmap bitmap = Bitmap.createBitmap(1, 2, Bitmap.Config.ARGB_8888);
        mResource.setBitmap(bitmap);
        assertEquals(bitmap, DynamicResourceTestUtils.getBitmapSync(mResource));

        // Bitmap was already returned, next onResourceRequested should no-op.
        mResource.setOnResourceReadyCallback((resource) -> { assert false; });
        mResource.onResourceRequested();
    }

    @Test
    public void testSetBitmapGCed() {
        Bitmap bitmap = Bitmap.createBitmap(1, 2, Bitmap.Config.ARGB_8888);
        WeakReference<Bitmap> bitmapWeakReference = new WeakReference<>(bitmap);
        mResource.setBitmap(bitmap);
        bitmap = null;
        assertFalse(canBeGarbageCollected(bitmapWeakReference));

        Bitmap bitmap2 = Bitmap.createBitmap(3, 4, Bitmap.Config.ARGB_8888);
        mResource.setBitmap(bitmap2);
        assertTrue(canBeGarbageCollected(bitmapWeakReference));
    }

    @Test
    public void testGetBitmapGCed() {
        Bitmap bitmap = Bitmap.createBitmap(1, 2, Bitmap.Config.ARGB_8888);
        WeakReference<Bitmap> bitmapWeakReference = new WeakReference<>(bitmap);
        mResource.setBitmap(bitmap);
        bitmap = null;
        assertFalse(canBeGarbageCollected(bitmapWeakReference));

        DynamicResourceTestUtils.getBitmapSync(mResource);
        assertTrue(canBeGarbageCollected(bitmapWeakReference));
    }

    @Test
    public void testOnResourceRequested_NotReady() {
        Bitmap bitmap = Bitmap.createBitmap(1, 2, Bitmap.Config.ARGB_8888);

        // No callback or bitmap, onResourceRequested should no-op.
        mResource.onResourceRequested();

        // No bitmap, onResourceRequested should no-op.
        mResource.setOnResourceReadyCallback((resource) -> { assert false; });
        mResource.onResourceRequested();

        // No callback, onResourceRequested should no-op.
        mResource.setOnResourceReadyCallback(null);
        mResource.setBitmap(bitmap);
        mResource.onResourceRequested();
    }
}

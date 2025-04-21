// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.os.Build;
import android.view.WindowManager;
import android.window.InputTransferToken;

import androidx.annotation.RequiresApi;

import org.jni_zero.CalledByNative;

import org.chromium.base.ContextUtils;
import org.chromium.base.TraceEvent;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.NullUnmarked;
import org.chromium.build.annotations.Nullable;

@RequiresApi(Build.VERSION_CODES.VANILLA_ICE_CREAM)
@NullMarked
public class InputTransferHandler {
    private InputTransferToken mBrowserToken;
    private @Nullable InputTransferToken mVizToken;

    public InputTransferHandler(InputTransferToken browserToken) {
        mBrowserToken = browserToken;
    }

    private boolean canTransferInputToViz() {
        // TODO(370506271): Implement logic for when can we transfer vs not.
        return false;
    }

    public void setVizToken(InputTransferToken token) {
        TraceEvent.instant("Storing InputTransferToken");
        mVizToken = token;
    }

    @NullUnmarked
    public boolean maybeTransferInputToViz() {
        if (!canTransferInputToViz()) {
            return false;
        }
        WindowManager wm =
                ContextUtils.getApplicationContext().getSystemService(WindowManager.class);
        return wm.transferTouchGesture(mBrowserToken, mVizToken);
    }

    @CalledByNative
    private static boolean maybeTransferInputToViz(int surfaceId) {
        InputTransferHandler handler = SurfaceInputTransferHandlerMap.getMap().get(surfaceId);

        if (handler == null) {
            return false;
        }

        return handler.maybeTransferInputToViz();
    }
}

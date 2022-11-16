// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.autofill_assistant.qr_code;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.autofill_assistant.AssistantDependencies;
import org.chromium.components.autofill_assistant.guided_browsing.qr_code.AssistantQrCodeController;

/**
 * Util to expose QR Code Scan functionality from the |guided_browsing| component to the native
 * code.
 */
@JNINamespace("autofill_assistant")
public class AssistantQrCodeUtil {
    /** Prompts the user for QR Code Scanning. */
    @CalledByNative
    private static void promptQrCodeCameraScan(AssistantDependencies dependencies,
            AssistantQrCodeCameraScanModelWrapper cameraScanModelWrapper) {
        AssistantQrCodeController.promptQrCodeCameraScan(dependencies.getActivity(),
                dependencies.getWindowAndroid(), cameraScanModelWrapper.getCameraScanModel());
    }
}

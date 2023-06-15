package org.chromium.chrome.browser;

import org.chromium.chrome.R;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("chrome::android")
public class LaunchHelper {
    static public void doRestart() {
      LaunchHelperJni.get().restart();
    }

    @NativeMethods
    interface Natives {
        void restart();
    }
}

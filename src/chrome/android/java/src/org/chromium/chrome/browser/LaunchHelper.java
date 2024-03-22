package org.chromium.chrome.browser;

import org.chromium.chrome.R;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

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

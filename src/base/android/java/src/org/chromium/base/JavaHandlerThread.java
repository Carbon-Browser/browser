// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.JniType;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.lang.Thread.UncaughtExceptionHandler;

/** Thread in Java with an Android Handler. This class is not thread safe. */
@NullMarked
@JNINamespace("base::android")
public class JavaHandlerThread {
    private final HandlerThread mThread;

    private @Nullable Throwable mUnhandledException;

    /**
     * Construct a java-only instance. Can be connected with native side later.
     * Useful for cases where a java thread is needed before native library is loaded.
     */
    public JavaHandlerThread(String name, int priority) {
        mThread = new HandlerThread(name, priority);
    }

    @CalledByNative
    private static JavaHandlerThread create(@JniType("const char*") String name, int priority) {
        return new JavaHandlerThread(name, priority);
    }

    public Looper getLooper() {
        assert hasStarted();
        return mThread.getLooper();
    }

    public void maybeStart() {
        if (hasStarted()) return;
        mThread.start();
    }

    @CalledByNative
    private void startAndInitialize(final long nativeThread, final long nativeEvent) {
        maybeStart();
        new Handler(mThread.getLooper())
                .post(
                        new Runnable() {
                            @Override
                            public void run() {
                                JavaHandlerThreadJni.get()
                                        .initializeThread(nativeThread, nativeEvent);
                            }
                        });
    }

    @CalledByNative
    private void quitThreadSafely(final long nativeThread) {
        // Allow pending java tasks to run, but don't run any delayed or newly queued up tasks.
        new Handler(mThread.getLooper())
                .post(
                        new Runnable() {
                            @Override
                            public void run() {
                                mThread.quit();
                                JavaHandlerThreadJni.get().onLooperStopped(nativeThread);
                            }
                        });
        // Signal that new tasks queued up won't be run.
        mThread.getLooper().quitSafely();
    }

    @CalledByNative
    private void joinThread() {
        boolean joined = false;
        while (!joined) {
            try {
                mThread.join();
                joined = true;
            } catch (InterruptedException e) {
            }
        }
    }

    private boolean hasStarted() {
        return mThread.getState() != Thread.State.NEW;
    }

    @CalledByNative
    private boolean isAlive() {
        return mThread.isAlive();
    }

    // This should *only* be used for tests. In production we always need to call the original
    // uncaught exception handler (the framework's) after any uncaught exception handling we do, as
    // it generates crash dumps and kills the process.
    @CalledByNative
    private void listenForUncaughtExceptionsForTesting() {
        mThread.setUncaughtExceptionHandler(
                new UncaughtExceptionHandler() {
                    @Override
                    public void uncaughtException(Thread t, Throwable e) {
                        mUnhandledException = e;
                    }
                });
    }

    @CalledByNative
    private @Nullable Throwable getUncaughtExceptionIfAny() {
        return mUnhandledException;
    }

    @NativeMethods
    interface Natives {
        void initializeThread(long nativeJavaHandlerThread, long nativeEvent);

        void onLooperStopped(long nativeJavaHandlerThread);
    }
}

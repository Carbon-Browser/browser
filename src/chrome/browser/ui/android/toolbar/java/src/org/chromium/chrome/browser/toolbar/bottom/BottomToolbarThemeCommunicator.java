package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;
import android.content.res.ColorStateList;

// communicate between bottom toolbar and ToolbarManager
public interface BottomToolbarThemeCommunicator {
    void onTintChanged(ColorStateList tint, boolean isDarkTheme);
}

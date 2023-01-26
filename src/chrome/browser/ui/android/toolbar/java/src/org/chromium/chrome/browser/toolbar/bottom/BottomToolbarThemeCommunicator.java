package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;
import android.content.res.ColorStateList;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;

// communicate between bottom toolbar and ToolbarManager
public interface BottomToolbarThemeCommunicator {
    void onTintChanged(ColorStateList tint, @BrandedColorScheme int brandedColorScheme);

    void onThemeChanged(int color);
}

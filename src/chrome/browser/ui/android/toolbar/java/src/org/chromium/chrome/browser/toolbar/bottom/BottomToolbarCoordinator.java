package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;

// communicate between bottom toolbar and ToolbarManager
public interface BottomToolbarCoordinator {
    void openRewardsPopup(View v);

    void focusUrlBar();

    void openSpeedDialPopup(View v);

    void loadHomepage(View v);
}

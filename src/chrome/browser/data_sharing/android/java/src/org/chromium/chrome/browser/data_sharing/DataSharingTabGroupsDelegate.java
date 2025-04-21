// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.data_sharing;

import android.content.Context;
import android.graphics.Bitmap;

import org.chromium.base.Callback;
import org.chromium.url.GURL;

/** An interface to handle actions related to tab groups. */
public interface DataSharingTabGroupsDelegate {
    /**
     * Open the tab group dialog of the given tab group id.
     *
     * @param id The tabId of the first tab in the group.
     */
    public void openTabGroupWithTabId(int tabId);

    /**
     * Open url in the Chrome Custom Tab.
     *
     * @param context The context of the current activity.
     * @param gurl The GURL of the page to be opened in CCT.
     */
    public void openLearnMoreSharedTabGroupsPage(Context context, GURL gurl);

    /**
     * Create a preview image for the tab group to show in share sheet.
     *
     * @param collaborationId The collaboration ID of the tab group.
     * @param size The expected size in pixels of the preview bitmap.
     * @param onResult The callback to return the preview image.
     */
    public void getPreviewBitmap(String collaborationId, int size, Callback<Bitmap> onResult);
}

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices.ui.controller.webapps;

import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.browserservices.intents.WebappExtras;
import org.chromium.components.embedder_support.util.UrlUtilities;

/** Provides WebAPK specific behaviour for the {@link CurrentPageVerifier}. */
public class WebApkVerifier extends WebappVerifier {
    private final WebappExtras mWebappExtras;

    public WebApkVerifier(BrowserServicesIntentDataProvider intentDataProvider) {
        mWebappExtras = intentDataProvider.getWebappExtras();
        assert mWebappExtras != null;
    }

    @Override
    protected String getScope() {
        return mWebappExtras.scopeUrl;
    }

    @Override
    protected boolean isUrlInScope(String url) {
        return UrlUtilities.isUrlWithinScope(url, mWebappExtras.scopeUrl);
    }
}

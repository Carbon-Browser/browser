// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '/strings.m.js';

import {BrowserProxyImpl} from './browser_proxy.js';
import {GlicApiHost} from './glic_api_impl/glic_api_host.js';

const browserProxy = BrowserProxyImpl.getInstance();
const webview =
    document.getElementById('guest-frame') as chrome.webviewTag.WebView;

// Manages construction of the GlicAppHost, which must be created to match the
// lifecycle of the webview's page load.
class GlicAppHostManager {
  host: GlicApiHost|undefined;
  constructor() {
    webview.addEventListener('loadcommit', (e: any) => {
      this.loadCommit(e.url, e.isTopLevel);
    });
    webview.addEventListener('contentload', () => {
      this.contentLoaded();
    });
    webview.addEventListener('newwindow', (e: Event) => {
      this.onNewWindowEvent(e as chrome.webviewTag.NewWindowEvent);
    });
    webview.addEventListener('permissionrequest', (e: any) => {
      if (e.permission === 'media' || e.permission === 'geolocation') {
        e.request.allow();
      }
    });
  }

  onNewWindowEvent(event: chrome.webviewTag.NewWindowEvent) {
    if (!this.host) {
      return;
    }
    event.preventDefault();
    this.host.openLinkInNewTab(event.targetUrl);
    event.stopPropagation();
  }

  loadCommit(url: string, isTopLevel: boolean) {
    if (!isTopLevel) {
      return;
    }
    if (this.host) {
      this.host.destroy();
      this.host = undefined;
    }
    if (webview.contentWindow) {
      this.host = new GlicApiHost(
          browserProxy, webview.contentWindow, new URL(url).origin);
    }
  }

  contentLoaded() {
    if (this.host) {
      this.host.contentLoaded();
    }
  }
}

// Blocking on cookie syncing here introduces latency, we should consider ways
// to avoid it.
browserProxy.handler.syncWebviewCookies().then(() => new GlicAppHostManager());

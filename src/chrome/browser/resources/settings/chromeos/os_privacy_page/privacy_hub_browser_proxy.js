// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class PrivacyHubBrowserProxy {
  /** @return {!Promise<boolean>} */
  getInitialCameraHardwareToggleState() {}

  /** @return {!Promise<boolean>} */
  getInitialMicrophoneHardwareToggleState() {}
}

/**
 * @implements {PrivacyHubBrowserProxy}
 */
export class PrivacyHubBrowserProxyImpl {
  /** @override */
  getInitialCameraHardwareToggleState() {
    return sendWithPromise('getInitialCameraHardwareToggleState');
  }

  /** @override */
  getInitialMicrophoneHardwareToggleState() {
    return sendWithPromise('getInitialMicrophoneHardwareToggleState');
  }

  /** @return {!PrivacyHubBrowserProxy} */
  static getInstance() {
    return instance || (instance = new PrivacyHubBrowserProxyImpl());
  }

  /** @param {!PrivacyHubBrowserProxy} obj */
  static setInstanceForTesting(obj) {
    instance = obj;
  }
}

/** @type {?PrivacyHubBrowserProxy} */
let instance = null;

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Suite of tests for page-specific behaviors of
 * SetupSucceededPage.
 */

import 'chrome://multidevice-setup/strings.m.js';
import 'chrome://resources/cr_components/chromeos/multidevice_setup/setup_succeeded_page.m.js';

import {BrowserProxyImpl} from 'chrome://resources/cr_components/chromeos/multidevice_setup/multidevice_setup_browser_proxy.m.js';

import {TestBrowserProxy} from '../../../test_browser_proxy.js';

/**
 * @implements {BrowserProxy}
 */
export class TestMultideviceSetupBrowserProxy extends TestBrowserProxy {
  constructor() {
    super(['getProfileInfo', 'openMultiDeviceSettings']);
  }

  /** @override */
  openMultiDeviceSettings() {
    this.methodCalled('openMultiDeviceSettings');
  }

  /** @override */
  getProfileInfo() {
    this.methodCalled('getProfileInfo');
    return Promise.resolve({});
  }
}

suite('MultiDeviceSetup', () => {
  /**
   * SetupSucceededPage created before each test. Defined in setUp.
   * @type {?SetupSucceededPage}
   */
  let setupSucceededPageElement = null;
  /** @type {?TestMultideviceSetupBrowserProxy} */
  let browserProxy = null;

  setup(async () => {
    browserProxy = new TestMultideviceSetupBrowserProxy();
    BrowserProxyImpl.instance_ = browserProxy;

    setupSucceededPageElement = document.createElement('setup-succeeded-page');
    document.body.appendChild(setupSucceededPageElement);
  });

  test('Settings link opens settings page', () => {
    setupSucceededPageElement.$$('#settings-link').click();
    return browserProxy.whenCalled('openMultiDeviceSettings');
  });
});

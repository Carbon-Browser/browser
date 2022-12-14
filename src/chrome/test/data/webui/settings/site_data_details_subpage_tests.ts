// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {CookieDetails, cookieInfo, LocalDataBrowserProxyImpl, SiteDataDetailsSubpageElement} from 'chrome://settings/lazy_load.js';
import {MetricsBrowserProxyImpl, PrivacyElementInteractions, Router,routes} from 'chrome://settings/settings.js';

import {assertEquals} from 'chrome://webui-test/chai_assert.js';
import {flushTasks} from 'chrome://webui-test/test_util.js';

import {TestLocalDataBrowserProxy} from './test_local_data_browser_proxy.js';
import {TestMetricsBrowserProxy} from './test_metrics_browser_proxy.js';

// clang-format on

/** @fileoverview Suite of tests for site-data-details-subpage. */
suite('SiteDataDetailsSubpage', function() {
  let page: SiteDataDetailsSubpageElement;

  let browserProxy: TestLocalDataBrowserProxy;

  let testMetricsBrowserProxy: TestMetricsBrowserProxy;

  const cookieDetails: CookieDetails&{[key: string]: string | boolean} = {
    accessibleToScript: 'Yes',
    content: 'dummy_cookie_contents',
    created: 'Tuesday, February 7, 2017 at 11:28:45 AM',
    domain: '.foo.com',
    expires: 'Wednesday, February 7, 2018 at 11:28:45 AM',
    hasChildren: false,
    id: '328',
    idPath: '74,165,328',
    name: 'abcd',
    path: '/',
    sendfor: 'Any kind of connection',
    title: 'abcd',
    totalUsage: '1kB',
    type: 'cookie',
  };

  const site: string = 'foo.com';

  setup(function() {
    browserProxy = new TestLocalDataBrowserProxy();
    browserProxy.setCookieDetails([cookieDetails]);
    LocalDataBrowserProxyImpl.setInstance(browserProxy);
    testMetricsBrowserProxy = new TestMetricsBrowserProxy();
    MetricsBrowserProxyImpl.setInstance(testMetricsBrowserProxy);
    document.body.innerHTML = '';
    page = document.createElement('site-data-details-subpage');
    Router.getInstance().navigateTo(
        routes.SITE_SETTINGS_DATA_DETAILS, new URLSearchParams('site=' + site));

    document.body.appendChild(page);
  });

  teardown(function() {
    Router.getInstance().resetRouteForTesting();
  });

  test('DetailsShownForCookie', async function() {
    const actualSite: string =
        await browserProxy.whenCalled('getCookieDetails');
    assertEquals(site, actualSite);

    flush();
    const entries = page.shadowRoot!.querySelectorAll('.cr-row');
    assertEquals(1, entries.length);

    const listItems = page.shadowRoot!.querySelectorAll('.list-item');
    // |cookieInfo| is a global var defined in
    // site_settings/cookie_info.js, and specifies the fields that are
    // shown for a cookie.
    assertEquals(cookieInfo['cookie']!.length, listItems.length);

    // Check that all the cookie information is presented in the DOM.
    const cookieDetailValues =
        page.shadowRoot!.querySelectorAll<HTMLElement>('.secondary');
    cookieDetailValues.forEach(function(div: HTMLElement, i: number) {
      const key = cookieInfo['cookie']![i]![0]!;
      assertEquals(cookieDetails[key], div.textContent);
    });
  });

  test('InteractionMetrics', async function() {
    // Confirm that various page interactions record the appropriate metric.
    await flushTasks();
    page.shadowRoot!.querySelector<HTMLElement>('.icon-clear')!.click();
    let metric =
        await testMetricsBrowserProxy.whenCalled('recordSettingsPageHistogram');
    assertEquals(PrivacyElementInteractions.COOKIE_DETAILS_REMOVE_ITEM, metric);
    testMetricsBrowserProxy.reset();

    // removeAll is public on the element to allow it to be called from a button
    // located in the enclosing settings-subpage element. It is not bound to
    // interactions with any part of the element under test, and is thus called
    // directly from the test.
    page.removeAll();
    metric =
        await testMetricsBrowserProxy.whenCalled('recordSettingsPageHistogram');
    assertEquals(PrivacyElementInteractions.COOKIE_DETAILS_REMOVE_ALL, metric);
  });
});

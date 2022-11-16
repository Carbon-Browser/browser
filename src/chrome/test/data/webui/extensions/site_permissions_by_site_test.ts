// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-site-permissions-all-sites. */
import 'chrome://extensions/extensions.js';

import {ExtensionsSitePermissionsBySiteElement, navigation, Page} from 'chrome://extensions/extensions.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertDeepEquals, assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {isVisible} from 'chrome://webui-test/test_util.js';

import {TestService} from './test_service.js';

suite('SitePermissionsBySite', function() {
  let element: ExtensionsSitePermissionsBySiteElement;
  let delegate: TestService;
  let listenerId: number = 0;

  const siteGroups: chrome.developerPrivate.SiteGroup[] = [
    {
      etldPlusOne: 'google.ca',
      sites: [
        {
          siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
          site: 'https://images.google.ca',
        },
        {
          siteList: chrome.developerPrivate.UserSiteSet.RESTRICTED,
          site: 'http://google.ca',
        },
      ],
    },
    {
      etldPlusOne: 'example.com',
      sites: [{
        siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
        site: 'http://example.com',
      }],
    },
  ];

  setup(function() {
    delegate = new TestService();
    delegate.siteGroups = siteGroups;

    document.body.innerHTML = '';
    element = document.createElement('extensions-site-permissions-by-site');
    element.delegate = delegate;
    document.body.appendChild(element);
  });

  teardown(function() {
    if (listenerId !== 0) {
      assertTrue(navigation.removeListener(listenerId));
      listenerId = 0;
    }
  });

  test(
      'clicking close button navigates back to site permissions page',
      function() {
        let currentPage = null;
        listenerId = navigation.addListener(newPage => {
          currentPage = newPage;
        });

        flush();
        const closeButton = element.$.closeButton;
        assertTrue(!!closeButton);
        assertTrue(isVisible(closeButton));

        closeButton.click();
        flush();

        assertDeepEquals(currentPage, {page: Page.SITE_PERMISSIONS});
      });

  test('extension and user sites are present', async function() {
    await delegate.whenCalled('getUserAndExtensionSitesByEtld');
    flush();

    const sitePermissionGroups =
        element.shadowRoot!.querySelectorAll<HTMLElement>(
            'site-permissions-site-group');
    assertEquals(2, sitePermissionGroups.length);
  });

  test('extension and user sites update when event is fired', async function() {
    await delegate.whenCalled('getUserAndExtensionSitesByEtld');
    flush();
    delegate.resetResolver('getUserAndExtensionSitesByEtld');
    delegate.siteGroups = [{
      etldPlusOne: 'random.com',
      sites: [{
        siteList: chrome.developerPrivate.UserSiteSet.RESTRICTED,
        site: 'http://www.random.com',
      }],
    }];

    delegate.userSiteSettingsChangedTarget.callListeners(
        {permittedSites: [], restrictedSites: ['http://www.random.com']});
    await delegate.whenCalled('getUserAndExtensionSitesByEtld');
    flush();

    const sitePermissionGroups =
        element.shadowRoot!.querySelectorAll<HTMLElement>(
            'site-permissions-site-group');
    assertEquals(1, sitePermissionGroups.length);
  });
});

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for site-permissions-site-group. */
import 'chrome://extensions/extensions.js';

import {SitePermissionsSiteGroupElement} from 'chrome://extensions/extensions.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {isVisible} from 'chrome://webui-test/test_util.js';

suite('SitePermissionsSiteGroupElement', function() {
  const PERMITTED_TEXT = loadTimeData.getString('permittedSites');
  const RESTRICTED_TEXT = loadTimeData.getString('restrictedSites');

  let element: SitePermissionsSiteGroupElement;

  setup(function() {
    document.body.innerHTML = '';
    element = document.createElement('site-permissions-site-group');
    document.body.appendChild(element);
  });

  test('clicking expand shows all sites within this group', async function() {
    element.data = {
      etldPlusOne: 'google.ca',
      sites: [
        {
          siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
          site: 'https://images.google.ca',
        },
        {
          siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
          site: 'http://google.ca',
        },
      ],
    };
    flush();

    assertEquals('google.ca', element.$.etldOrSite.innerText);
    assertEquals(PERMITTED_TEXT, element.$.etldOrSiteSubtext.innerText);

    const sitesList =
        element.shadowRoot!.querySelector<HTMLElement>('#sites-list');
    assertFalse(isVisible(sitesList));
    element.shadowRoot!.querySelector<HTMLElement>('cr-expand-button')!.click();
    flush();

    assertTrue(isVisible(sitesList));
    const expandedSites =
        element.shadowRoot!.querySelectorAll<HTMLElement>('.site');

    assertEquals('https://images.google.ca', expandedSites[0]!.innerText);
    assertEquals('http://google.ca', expandedSites[1]!.innerText);
  });

  test('no subtext shown for sites from different sets', async function() {
    element.data = {
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
    };
    flush();

    assertEquals('google.ca', element.$.etldOrSite.innerText);
    assertEquals('', element.$.etldOrSiteSubtext.innerText);

    element.shadowRoot!.querySelector<HTMLElement>('cr-expand-button')!.click();
    flush();

    assertTrue(isVisible(
        element.shadowRoot!.querySelector<HTMLElement>('#sites-list')));
    const expandedSites =
        element.shadowRoot!.querySelectorAll<HTMLElement>('.site-subtext');

    // The subtext for each expanded site should show which set it's from.
    assertEquals(PERMITTED_TEXT, expandedSites[0]!.innerText);
    assertEquals(RESTRICTED_TEXT, expandedSites[1]!.innerText);
  });

  test('full site shown if there is only one site in group', async function() {
    element.data = {
      etldPlusOne: 'example.com',
      sites: [{
        siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
        site: 'https://a.example.com',
      }],
    };
    flush();

    assertEquals('https://a.example.com', element.$.etldOrSite.innerText);
    assertEquals(PERMITTED_TEXT, element.$.etldOrSiteSubtext.innerText);

    assertFalse(isVisible(
        element.shadowRoot!.querySelector<HTMLElement>('cr-expand-button')));
  });

  test(
      'clicking the arrow for a single site shows dialog for that site',
      async function() {
        element.data = {
          etldPlusOne: 'example.com',
          sites: [{
            siteList: chrome.developerPrivate.UserSiteSet.PERMITTED,
            site: 'https://a.example.com',
          }],
        };
        flush();

        const editSiteButton = element.shadowRoot!.querySelector<HTMLElement>(
            '#edit-one-site-button');
        assertTrue(isVisible(editSiteButton));

        editSiteButton!.click();
        flush();

        const dialog = element.shadowRoot!.querySelector(
            'site-permissions-edit-permissions-dialog');
        assertTrue(!!dialog);
        assertTrue(dialog.$.dialog.open);
        assertEquals('https://a.example.com', dialog.site);
        assertEquals(
            chrome.developerPrivate.UserSiteSet.PERMITTED,
            dialog.originalSiteSet);
      });

  test(
      'clicking the arrow for an expanded site shows dialog for that site',
      async function() {
        element.data = {
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
        };
        flush();

        element.shadowRoot!.querySelector<HTMLElement>(
                               'cr-expand-button')!.click();
        flush();

        const editSiteButtons =
            element.shadowRoot!.querySelectorAll<HTMLElement>('cr-icon-button');
        assertEquals(2, editSiteButtons.length);

        editSiteButtons[1]!.click();
        flush();

        const dialog = element.shadowRoot!.querySelector(
            'site-permissions-edit-permissions-dialog');
        assertTrue(!!dialog);
        assertTrue(dialog.$.dialog.open);
        assertEquals('http://google.ca', dialog.site);
        assertEquals(
            chrome.developerPrivate.UserSiteSet.RESTRICTED,
            dialog.originalSiteSet);
      });
});

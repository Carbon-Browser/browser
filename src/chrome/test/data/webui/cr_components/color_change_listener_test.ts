// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {COLORS_CSS_SELECTOR, refreshColorCss} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';
import {assertEquals, assertFalse, assertNotEquals, assertTrue} from 'chrome://webui-test/chai_assert.js';

suite('ColorChangeListenerTest', () => {
  setup(() => {
    document.body.innerHTML =
        '<link rel="stylesheet" href="chrome://theme/colors.css?sets=ui"/>';
  });

  test('CorrectlyUpdatesColorsStylesheetURL', async () => {
    const colorCssNode =
        document.querySelector<HTMLLinkElement>(COLORS_CSS_SELECTOR);
    assertTrue(!!colorCssNode);
    const initialHref = colorCssNode.getAttribute('href');
    assertTrue(!!initialHref);
    assertTrue(initialHref.startsWith('chrome://theme/colors.css'));

    // refreshColorCss() should append search params to the colors CSS href.
    assertTrue(refreshColorCss());

    const secondHref = colorCssNode.getAttribute('href');
    assertTrue(!!secondHref);
    assertTrue(secondHref.startsWith('chrome://theme/colors.css'));
    const params = new URLSearchParams(new URL(secondHref).search);
    assertTrue(!!params.get('version'));
    assertEquals('ui', params.get('sets'));

    assertNotEquals(initialHref, secondHref);

    // Wait 1 millisecond before refresh. Otherwise the timestamp-based
    // version might not yet be updated.
    await new Promise(resolve => setTimeout(resolve, 1));
    // refreshColorCss() should append search params to the colors CSS href.
    assertTrue(refreshColorCss());

    const thirdHref = colorCssNode.getAttribute('href');
    assertTrue(!!thirdHref);
    assertTrue(thirdHref.startsWith('chrome://theme/colors.css'));
    assertTrue(!!new URL(thirdHref).search);

    assertNotEquals(initialHref, thirdHref);
    assertNotEquals(secondHref, thirdHref);
  });

  test('HandlesCasesWhereColorsStylesheetIsNotSetCorrectly', () => {
    // Handles the case where the link element exists but the attribute is
    // malformed.
    document.body.innerHTML =
        '<link rel="stylesheet" bad_href="chrome://theme/colors.css?sets=ui"/>';
    assertFalse(refreshColorCss());

    // Handles the case where the link element does not exist.
    document.body.innerHTML = '';
    assertFalse(refreshColorCss());
  });
});

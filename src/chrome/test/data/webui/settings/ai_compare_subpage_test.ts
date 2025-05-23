// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import type {SettingsAiCompareSubpageElement} from 'chrome://settings/lazy_load.js';
import {AiEnterpriseFeaturePrefName, AiPageActions} from 'chrome://settings/lazy_load.js';
import type {SettingsPrefsElement} from 'chrome://settings/settings.js';
import {AiPageCompareInteractions, CrSettingsPrefs, loadTimeData, MetricsBrowserProxyImpl, OpenWindowProxyImpl, ModelExecutionEnterprisePolicyValue} from 'chrome://settings/settings.js';
import {flushTasks} from 'chrome://webui-test/polymer_test_util.js';
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {TestOpenWindowProxy} from 'chrome://webui-test/test_open_window_proxy.js';

import {TestMetricsBrowserProxy} from './test_metrics_browser_proxy.js';

// clang-format on

suite('CompareSubpage', function() {
  let metricsBrowserProxy: TestMetricsBrowserProxy;
  let openWindowProxy: TestOpenWindowProxy;
  let subpage: SettingsAiCompareSubpageElement;
  let settingsPrefs: SettingsPrefsElement;

  suiteSetup(function() {
    settingsPrefs = document.createElement('settings-prefs');
    openWindowProxy = new TestOpenWindowProxy();
    OpenWindowProxyImpl.setInstance(openWindowProxy);
    return CrSettingsPrefs.initialized;
  });

  teardown(function() {
    // Reset pref policy to ALLOW.
    settingsPrefs.set(
        `prefs.${AiEnterpriseFeaturePrefName.COMPARE}.value`,
        ModelExecutionEnterprisePolicyValue.ALLOW);
  });

  function createPage() {
    metricsBrowserProxy = new TestMetricsBrowserProxy();
    MetricsBrowserProxyImpl.setInstance(metricsBrowserProxy);
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    subpage = document.createElement('settings-ai-compare-subpage');
    subpage.prefs = settingsPrefs.prefs;
    document.body.appendChild(subpage);
    return flushTasks();
  }

  async function assertFeatureInteractionMetrics(
      interaction: AiPageCompareInteractions, action: string) {
    const result =
        await metricsBrowserProxy.whenCalled('recordAiPageCompareInteractions');
    assertEquals(interaction, result);

    assertEquals(action, await metricsBrowserProxy.whenCalled('recordAction'));
  }

  test('compareLinkout', async function() {
    await createPage();

    const linkout = subpage.shadowRoot!.querySelector('cr-link-row');
    assertTrue(!!linkout);

    linkout.click();
    await assertFeatureInteractionMetrics(
        AiPageCompareInteractions.FEATURE_LINK_CLICKED,
        AiPageActions.COMPARE_FEATURE_LINK_CLICKED);
    const url = await openWindowProxy.whenCalled('openUrl');
    assertEquals(url, loadTimeData.getString('compareDataHomeUrl'));
  });

  test('compareLinkoutDisabled', async function() {
    settingsPrefs.set(
        `prefs.${AiEnterpriseFeaturePrefName.COMPARE}.value`,
        ModelExecutionEnterprisePolicyValue.DISABLE);
    await createPage();

    const linkout = subpage.shadowRoot!.querySelector('cr-link-row');
    assertTrue(!!linkout);

    assertTrue(linkout.disabled);
    linkout.click();
    assertEquals(
        0, metricsBrowserProxy.getCallCount('recordAiPageCompareInteractions'));
  });

  test('compareLearnMore', async () => {
    await createPage();

    const learnMoreLink = subpage.shadowRoot!.querySelector('a');
    assertTrue(!!learnMoreLink);
    assertEquals(
        learnMoreLink.href, loadTimeData.getString('compareLearnMoreUrl'));

    learnMoreLink.click();
    await assertFeatureInteractionMetrics(
        AiPageCompareInteractions.LEARN_MORE_LINK_CLICKED,
        AiPageActions.COMPARE_LEARN_MORE_CLICKED);
  });

  test('compareLearnMoreManaged', async () => {
    settingsPrefs.set(
        `prefs.${AiEnterpriseFeaturePrefName.COMPARE}.value`,
        ModelExecutionEnterprisePolicyValue.ALLOW_WITHOUT_LOGGING);
    await createPage();

    const learnMoreLink = subpage.shadowRoot!.querySelector('a');
    assertTrue(!!learnMoreLink);
    assertEquals(
        learnMoreLink.href,
        loadTimeData.getString('compareLearnMoreManagedUrl'));
  });

  test('comparePolicyIndicatorPref', async () => {
    await createPage();

    const indicator =
        subpage.shadowRoot!.querySelector('settings-ai-policy-indicator');
    assertTrue(!!indicator);
    assertTrue(!!indicator.pref);
    assertEquals(AiEnterpriseFeaturePrefName.COMPARE, indicator.pref.key);
  });
});

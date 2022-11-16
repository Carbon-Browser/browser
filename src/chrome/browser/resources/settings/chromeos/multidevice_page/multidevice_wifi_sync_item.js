// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-multidevice-wifi-sync-item' encapsulates special
 * logic for the wifi sync item used in the multidevice subpage.
 *
 * Wifi sync depends on Chrome Sync being activated. This component uses sync
 * proxies from the people page to check whether chrome sync is enabled.
 *
 * If it is enabled the multidevice feature item is used in the standard way,
 * otherwise the feature-controller and localized-link slots are overridden with
 * a disabled toggle and the wifi sync localized string component that is a
 * special case containing two links.
 */

import './multidevice_feature_item.js';
import './multidevice_wifi_sync_disabled_link.js';
import 'chrome://resources/cr_elements/cr_toggle/cr_toggle.m.js';
import '../../settings_shared.css.js';

import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {WebUIListenerBehavior, WebUIListenerBehaviorInterface} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {SyncBrowserProxyImpl} from '../../people_page/sync_browser_proxy.js';
import {OsSyncBrowserProxy, OsSyncBrowserProxyImpl, OsSyncPrefs} from '../os_people_page/os_sync_browser_proxy.js';

import {MultiDeviceFeatureBehavior, MultiDeviceFeatureBehaviorInterface} from './multidevice_feature_behavior.js';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {MultiDeviceFeatureBehaviorInterface}
 * @implements {WebUIListenerBehaviorInterface}
 */
const SettingsMultideviceWifiSyncItemElementBase = mixinBehaviors(
    [MultiDeviceFeatureBehavior, WebUIListenerBehavior], PolymerElement);

/** @polymer */
class SettingsMultideviceWifiSyncItemElement extends
    SettingsMultideviceWifiSyncItemElementBase {
  static get is() {
    return 'settings-multidevice-wifi-sync-item';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /** @private */
      isWifiSyncV1Enabled_: Boolean,
    };
  }

  /** @override */
  constructor() {
    super();

    /** @private {?OsSyncBrowserProxy} */
    this.osSyncBrowserProxy_ = null;

    /** @private {?SyncBrowserProxy} */
    this.syncBrowserProxy_ = null;

    if (loadTimeData.getBoolean('syncSettingsCategorizationEnabled')) {
      this.osSyncBrowserProxy_ = OsSyncBrowserProxyImpl.getInstance();
    } else {
      this.syncBrowserProxy_ = SyncBrowserProxyImpl.getInstance();
    }
  }

  /** @override */
  connectedCallback() {
    super.connectedCallback();

    if (loadTimeData.getBoolean('syncSettingsCategorizationEnabled')) {
      this.addWebUIListener(
          'os-sync-prefs-changed', this.handleOsSyncPrefsChanged_.bind(this));
      this.osSyncBrowserProxy_.sendOsSyncPrefsChanged();
    } else {
      this.addWebUIListener(
          'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));
      this.syncBrowserProxy_.sendSyncPrefsChanged();
    }
  }

  /**
   * Handler for when the sync preferences are updated.
   * @param {!SyncPrefs} syncPrefs
   * @private
   */
  handleSyncPrefsChanged_(syncPrefs) {
    this.isWifiSyncV1Enabled_ =
        !!syncPrefs && syncPrefs.wifiConfigurationsSynced;
  }

  /**
   * Handler for when os sync preferences are updated.
   * @param {!OsSyncPrefs} osSyncPrefs
   * @private
   */
  handleOsSyncPrefsChanged_(osSyncPrefs) {
    this.isWifiSyncV1Enabled_ =
        !!osSyncPrefs && osSyncPrefs.osWifiConfigurationsSynced;
  }

  /** @override */
  focus() {
    this.shadowRoot.querySelector('settings-multidevice-feature-item').focus();
  }
}

customElements.define(
    SettingsMultideviceWifiSyncItemElement.is,
    SettingsMultideviceWifiSyncItemElement);

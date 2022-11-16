// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {$} from 'chrome://resources/js/util.m.js';

import './assistant_optin_flow.m.js';
import {BrowserProxyImpl} from './browser_proxy.m.js';

class InSessionAssistantScreen {
    /**
     * Starts the assistant opt-in flow.
     */
    static show() {
      var url = new URL(document.URL);
      $('assistant-optin-flow-card').onBeforeShow();
      $('assistant-optin-flow-card')
          .onShow(
              url.searchParams.get('flow-type'),
              url.searchParams.get('caption-bar-height'));
    }

    /**
     * Reloads localized strings.
     * @param {!Object} data New dictionary with i18n values.
     */
    static reloadContent(data) {
      $('assistant-optin-flow-card').reloadContent(data);
    }

    /**
     * Add a setting zippy object in the corresponding screen.
     * @param {string} type type of the setting zippy.
     * @param {!Object} data String and url for the setting zippy.
     */
    static addSettingZippy(type, data) {
      $('assistant-optin-flow-card').addSettingZippy(type, data);
    }

    /**
     * Show the next screen in the flow.
     */
    static showNextScreen() {
      $('assistant-optin-flow-card').showNextScreen();
    }

    /**
     * Called when the Voice match state is updated.
     * @param {string} state the voice match state.
     */
    static onVoiceMatchUpdate(state) {
      $('assistant-optin-flow-card').onVoiceMatchUpdate(state);
    }

    /**
     * Called to show the next settings when there are multiple unbundled
     * activity control settings in the Value prop screen.
     */
    static onValuePropUpdate() {
      $('assistant-optin-flow-card').onValuePropUpdate();
    }

    /**
     * Called when the flow finished and close the dialog.
     */
    static closeDialog() {
      BrowserProxyImpl.getInstance().dialogClose();
    }
}

function initializeInSessionAssistant() {
  if (document.readyState === 'loading') {
    return;
  }
  document.removeEventListener('DOMContentLoaded', initializeInSessionAssistant);

  login.AssistantOptInFlowScreen.show();
}


window.login = {};
window.login.AssistantOptInFlowScreen = InSessionAssistantScreen;

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initializeInSessionAssistant);
} else {
  initializeInSessionAssistant();
}

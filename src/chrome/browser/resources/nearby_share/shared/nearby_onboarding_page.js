// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The 'nearby-onboarding-page' component handles the Nearby Share
 * onboarding flow. It is embedded in chrome://os-settings, chrome://settings
 * and as a standalone dialog via chrome://nearby.
 */

import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/cr_elements/cr_icons_css.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import 'chrome://resources/polymer/v3_0/iron-media-query/iron-media-query.js';
import './nearby_page_template.js';

import {I18nBehavior, I18nBehaviorInterface} from 'chrome://resources/js/i18n_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {NearbyShareOnboardingFinalState, processOnboardingCancelledMetrics, processOnboardingInitiatedMetrics} from './nearby_metrics_logger.js';
import {getTemplate} from './nearby_onboarding_page.html.js';
import {getNearbyShareSettings} from './nearby_share_settings.js';
import {NearbySettings} from './nearby_share_settings_behavior.js';

/**
 * @type {string}
 */
const ONBOARDING_SPLASH_LIGHT_ICON =
    'nearby-images:nearby-onboarding-splash-light';

/**
 * @type {string}
 */
const ONBOARDING_SPLASH_DARK_ICON =
    'nearby-images:nearby-onboarding-splash-dark';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 */
const NearbyOnboardingPageElementBase =
    mixinBehaviors([I18nBehavior], PolymerElement);

/** @polymer */
export class NearbyOnboardingPageElement extends
    NearbyOnboardingPageElementBase {
  static get is() {
    return 'nearby-onboarding-page';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /** @type {?NearbySettings} */
      settings: {
        type: Object,
      },

      /** @type {string} */
      errorMessage: {
        type: String,
        value: '',
      },

      /**
       * Whether the onboarding page is being rendered in dark mode.
       * @private {boolean}
       */
      isDarkModeActive_: {
        type: Boolean,
        value: false,
      },

    };
  }

  ready() {
    super.ready();

    this.addEventListener('next', this.onNext_);
    this.addEventListener('close', this.onClose_);
    this.addEventListener('keydown', (event) => {
      this.onKeydown_(/** @type {!KeyboardEvent} */ (event));
    });
    this.addEventListener('view-enter-start', this.onViewEnterStart_);
  }

  /** @private */
  onNext_() {
    this.submitDeviceNameInput_();
  }

  /** @private */
  onClose_() {
    processOnboardingCancelledMetrics(
        NearbyShareOnboardingFinalState.DEVICE_NAME_PAGE);
    const onboardingCancelledEvent = new CustomEvent('onboarding-cancelled', {
      bubbles: true,
      composed: true,
    });
    this.dispatchEvent(onboardingCancelledEvent);
  }

  /**
   * @param {!KeyboardEvent} e Event containing the key
   * @private
   */
  onKeydown_(e) {
    e.stopPropagation();
    if (e.key === 'Enter') {
      this.submitDeviceNameInput_();
      e.preventDefault();
    }
  }

  /** @private */
  onViewEnterStart_() {
    this.shadowRoot.querySelector('#deviceName').focus();
    processOnboardingInitiatedMetrics(new URL(document.URL));
  }


  /** @private */
  onDeviceNameInput_() {
    getNearbyShareSettings()
        .validateDeviceName(this.$.deviceName.value)
        .then((result) => {
          this.updateErrorMessage_(result.result);
        });
  }

  /** @private */
  submitDeviceNameInput_() {
    getNearbyShareSettings()
        .setDeviceName(this.$.deviceName.value)
        .then((result) => {
          this.updateErrorMessage_(result.result);
          if (result.result ===
              nearbyShare.mojom.DeviceNameValidationResult.kValid) {
            const changePageEvent = new CustomEvent(
                'change-page',
                {bubbles: true, composed: true, detail: {page: 'visibility'}});
            this.dispatchEvent(changePageEvent);
          }
        });
  }

  /**
   * @private
   *
   * @param {!nearbyShare.mojom.DeviceNameValidationResult} validationResult The
   *     error status from validating the provided device name.
   */
  updateErrorMessage_(validationResult) {
    switch (validationResult) {
      case nearbyShare.mojom.DeviceNameValidationResult.kErrorEmpty:
        this.errorMessage = this.i18n('nearbyShareDeviceNameEmptyError');
        break;
      case nearbyShare.mojom.DeviceNameValidationResult.kErrorTooLong:
        this.errorMessage = this.i18n('nearbyShareDeviceNameTooLongError');
        break;
      case nearbyShare.mojom.DeviceNameValidationResult.kErrorNotValidUtf8:
        this.errorMessage =
            this.i18n('nearbyShareDeviceNameInvalidCharactersError');
        break;
      default:
        this.errorMessage = '';
        break;
    }
  }

  /**
   * @private
   *
   * @param {!string} errorMessage The error message.
   * @return {boolean} Whether or not the error message exists.
   */
  hasErrorMessage_(errorMessage) {
    return errorMessage !== '';
  }

  /**
   * Returns the icon based on Light/Dark mode.
   * @return {string}
   */
  getOnboardingSplashIcon_() {
    return this.isDarkModeActive_ ? ONBOARDING_SPLASH_DARK_ICON :
                                    ONBOARDING_SPLASH_LIGHT_ICON;
  }
}

customElements.define(
    NearbyOnboardingPageElement.is, NearbyOnboardingPageElement);

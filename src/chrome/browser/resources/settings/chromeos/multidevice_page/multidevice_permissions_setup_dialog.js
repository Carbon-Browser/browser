// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * This element provides the Phone Hub notification and apps access setup flow
 * that, when successfully completed, enables the feature that allows a user's
 * phone notifications and apps to be mirrored on their Chromebook.
 */

import 'chrome://resources/cr_components/localized_link/localized_link.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/cr_elements/shared_vars_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import './multidevice_screen_lock_subpage.js';
import '../os_icons.js';
import '../../settings_shared.css.js';

import {assert} from 'chrome://resources/js/assert.m.js';
import {I18nBehavior, I18nBehaviorInterface} from 'chrome://resources/js/i18n_behavior.m.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {WebUIListenerBehavior, WebUIListenerBehaviorInterface} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {MultiDeviceBrowserProxy, MultiDeviceBrowserProxyImpl} from './multidevice_browser_proxy.js';
import {MultiDeviceFeature, PhoneHubPermissionsSetupAction, PhoneHubPermissionsSetupFlowScreens} from './multidevice_constants.js';

/**
 * Numerical values should not be changed because they must stay in sync with
 * notification_access_setup_operation.h and apps_access_setup_operation.h,
 * with the exception of CONNECTION_REQUESTED. If PermissionsSetupStatus is
 * FAILED_OR_CANCELLED, we will abort all setup processes. If
 * PermissionsSetupStatus is COMPLETED_USER_REJECTED, we will proceed to the
 * next setup process.
 * @enum {number}
 */
export const PermissionsSetupStatus = {
  CONNECTION_REQUESTED: 0,
  CONNECTING: 1,
  TIMED_OUT_CONNECTING: 2,
  CONNECTION_DISCONNECTED: 3,
  SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE: 4,
  COMPLETED_SUCCESSFULLY: 5,
  NOTIFICATION_ACCESS_PROHIBITED: 6,
  COMPLETED_USER_REJECTED: 7,
  FAILED_OR_CANCELLED: 8,
};

/**
 * Numerical values the flow of dialog set up progress.
 * @enum {number}
 */
export const SetupFlowStatus = {
  INTRO: 0,
  SET_LOCKSCREEN: 1,
  WAIT_FOR_PHONE_NOTIFICATION: 2,
  WAIT_FOR_PHONE_APPS: 3,
  WAIT_FOR_PHONE_COMBINED: 4,
};

/**
 * Indicates that the onboarding flow includes Phone Hub Notification feature.
 */
export const NOTIFICATION_FEATURE = 1 << 0;

/**
 * Indicates that the onboarding flow includes Phone Hub Camera Roll feature.
 */
export const CAMERA_ROLL_FEATURE = 1 << 1;

/**
 * Indicates that the onboarding flow includes Phone Hub Apps feature.
 */
export const APPS_FEATURE = 1 << 2;

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 * @implements {WebUIListenerBehaviorInterface}
 */
const SettingsMultidevicePermissionsSetupDialogElementBase = mixinBehaviors(
    [
      I18nBehavior,
      WebUIListenerBehavior,
    ],
    PolymerElement);

/** @polymer */
class SettingsMultidevicePermissionsSetupDialogElement extends
    SettingsMultidevicePermissionsSetupDialogElementBase {
  static get is() {
    return 'settings-multidevice-permissions-setup-dialog';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /** @private {!PhoneHubPermissionsSetupFlowScreens} */
      setupScreen_: {
        type: Number,
        computed: 'getCurrentScreen_(setupState_, flowState_)',
      },

      /**
       * A null |setupState_| indicates that the operation has not yet started.
       * @private {?PermissionsSetupStatus}
       */
      setupState_: {
        type: Number,
        value: null,
      },

      /** @private */
      title_: {
        type: String,
        computed: 'getTitle_(setupState_, flowState_)',
      },

      /** @private */
      description_: {
        type: String,
        computed: 'getDescription_(setupState_, flowState_)',
      },

      /** @private */
      hasStartedSetupAttempt_: {
        type: Boolean,
        computed: 'computeHasStartedSetupAttempt_(flowState_)',
        reflectToAttribute: true,
      },

      /** @private */
      isSetupAttemptInProgress_: {
        type: Boolean,
        computed: 'computeIsSetupAttemptInProgress_(setupState_)',
        reflectToAttribute: true,
      },

      /** @private */
      isSetupScreenLockInProgress_: {
        type: Boolean,
        computed: 'computeIsSetupScreenLockInProgress_(flowState_)',
        reflectToAttribute: true,
      },

      /** @private */
      didSetupAttemptFail_: {
        type: Boolean,
        computed: 'computeDidSetupAttemptFail_(setupState_)',
        reflectToAttribute: true,
      },

      /** @private */
      hasCompletedSetup_: {
        type: Boolean,
        computed: 'computeHasCompletedSetup_(setupState_)',
        reflectToAttribute: true,
      },

      /** @private */
      isNotificationAccessProhibited_: {
        type: Boolean,
        computed: 'computeIsNotificationAccessProhibited_(setupState_)',
      },

      /**
       * @private {?SetupFlowStatus}
       */
      flowState_: {
        type: Number,
        value: SetupFlowStatus.INTRO,
      },

      /** @private */
      isScreenLockEnabled_: {
        type: Boolean,
        value: false,
      },

      /** Reflects whether the password dialog is showing. */
      isPasswordDialogShowing: {
        type: Boolean,
        value: false,
        notify: true,
      },

      /**
       * Get the value of settings.OnEnableScreenLockChanged from
       * multidevice_page.js because multidevice_permissions_setup_dialog.js
       * doesn't always popup to receive event from FireWebUIListener.
       */
      isChromeosScreenLockEnabled: {
        type: Boolean,
        value: false,
      },

      /**
       * Get the value of settings.OnScreenLockStatusChanged from
       * multidevice_page.js because multidevice_permissions_setup_dialog.js
       * doesn't always popup to receive event from FireWebUIListener.
       */
      isPhoneScreenLockEnabled: {
        type: Boolean,
        value: false,
      },

      /** Whether this dialog should show Camera Roll info */
      showCameraRoll:
          {type: Boolean, value: false, observer: 'onAccessStateChanged_'},

      /** Whether this dialog should show Notifications info */
      showNotifications:
          {type: Boolean, value: false, observer: 'onAccessStateChanged_'},

      /** Whether this dialog should show App Streaming info */
      showAppStreaming:
          {type: Boolean, value: false, observer: 'onAccessStateChanged_'},

      /**
       * Indicates that the features we want to handle during setup flow.
       * It is constructed using the bitwise _FEATURE values (ex:
       * NOTIFICATION_FEATURE) declared at the top.
       * @private
       */
      setupMode_: {
        type: Number,
        value: 0,
      },

      /**
       * Indicates that the features we have completed after setup flow.
       * It is constructed using the bitwise _FEATURE values (ex:
       * NOTIFICATION_FEATURE) declared at the top.
       * @private
       */
      completedMode_: {
        type: Number,
        value: 0,
      },

      /** @private */
      shouldShowLearnMoreButton_: {
        type: Boolean,
        computed: 'computeShouldShowLearnMoreButton_(setupState_, flowState_)',
        reflectToAttribute: true,
      },

      /** @private */
      shouldShowDisabledDoneButton_: {
        type: Boolean,
        computed: 'computeShouldShowDisabledDoneButton_(setupState_)',
        reflectToAttribute: true,
      },

      /** @private */
      isPinNumberSelected_: {
        type: Boolean,
        value: false,
      },

      /** @private */
      isSetPinDone_: {
        type: Boolean,
        value: false,
      },

      /** @private */
      showSetupPinDialog_: {
        type: Boolean,
        value: false,
      },

      /**
       * Whether the combined setup for Notifications and Camera Roll is
       * supported on the connected phone.
       */
      combinedSetupSupported: {
        type: Boolean,
        value: false,
      },

      /** @private */
      learnMoreButtonAriaLabel_: {
        type: String,
        computed: 'getLearnMoreButtonAriaLabel_()',
      },
    };
  }

  constructor() {
    super();

    /** @private {!MultiDeviceBrowserProxy} */
    this.browserProxy_ = MultiDeviceBrowserProxyImpl.getInstance();
  }

  /** @override */
  ready() {
    super.ready();

    this.addEventListener('set-pin-done', this.onSetPinDone_);
  }

  /** @override */
  connectedCallback() {
    super.connectedCallback();

    this.addWebUIListener(
        'settings.onNotificationAccessSetupStatusChanged',
        this.onNotificationSetupStateChanged_.bind(this));
    this.addWebUIListener(
        'settings.onAppsAccessSetupStatusChanged',
        this.onAppsSetupStateChanged_.bind(this));
    this.addWebUIListener(
        'settings.onCombinedAccessSetupStatusChanged',
        this.onCombinedSetupStateChanged_.bind(this));
    this.$.dialog.showModal();
  }

  /**
   * @param {!PermissionsSetupStatus} notificationSetupState
   * @private
   */
  onNotificationSetupStateChanged_(notificationSetupState) {
    if (this.flowState_ !== SetupFlowStatus.WAIT_FOR_PHONE_NOTIFICATION) {
      return;
    }

    // When the notificationSetupState is COMPLETED_SUCCESSFULLY or
    // COMPLETED_USER_REJECTED we should continue on with the setup flow if
    // there are additional features, all other results will change the screen
    // that is shown and pause or terminate the setup flow.
    if (notificationSetupState !==
            PermissionsSetupStatus.COMPLETED_SUCCESSFULLY &&
        notificationSetupState !==
            PermissionsSetupStatus.COMPLETED_USER_REJECTED) {
      this.setupState_ = notificationSetupState;
      return;
    }

    // Note: we can only update this.setupState_ after assigning
    // this.completeMode_. Otherwise, we cannot use the final
    // this.completedMode_ to determine the completed title.
    if (notificationSetupState ===
        PermissionsSetupStatus.COMPLETED_SUCCESSFULLY) {
      if (this.setupMode_ & NOTIFICATION_FEATURE && !this.showNotifications) {
        this.completedMode_ |= NOTIFICATION_FEATURE;
        this.browserProxy_.setFeatureEnabledState(
            MultiDeviceFeature.PHONE_HUB_NOTIFICATIONS, true);
      }
    }

    if (this.showAppStreaming) {
      // We still need to process the apps steaming onboarding flow, update
      // this.setupState_ to CONNECTION_REQUESTED first and wait for
      // onAppsSetupStateChanged_() callback to update this.setupState_.
      this.browserProxy_.attemptAppsSetup();
      this.flowState_ = SetupFlowStatus.WAIT_FOR_PHONE_APPS;
      this.setupState_ = PermissionsSetupStatus.CONNECTION_REQUESTED;
    } else {
      this.setupState_ = notificationSetupState;
      // We don't need to deal with the apps streaming onboarding flow, so we
      // can log completed case here.
      this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
          this.setupScreen_, PhoneHubPermissionsSetupAction.SHOWN);
    }
  }

  /**
   * @param {!PermissionsSetupStatus} appsSetupResult
   * @private
   */
  onAppsSetupStateChanged_(appsSetupResult) {
    if (this.flowState_ !== SetupFlowStatus.WAIT_FOR_PHONE_APPS) {
      return;
    }

    // Note: If appsSetupResult is COMPLETED_SUCCESSFULLY, we can only update
    // this.setupState_ after assigning this.completeMode_. Otherwise, we cannot
    // use the final this.completedMode_ to determine the completed title.
    if (appsSetupResult === PermissionsSetupStatus.COMPLETED_SUCCESSFULLY &&
        !this.showAppStreaming) {
      this.completedMode_ |= APPS_FEATURE;
      this.browserProxy_.setFeatureEnabledState(MultiDeviceFeature.ECHE, true);
      this.setupState_ = appsSetupResult;
      this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
          this.setupScreen_, PhoneHubPermissionsSetupAction.SHOWN);
    }
    this.setupState_ = appsSetupResult;
  }

  /**
   * @param {!PermissionsSetupStatus} combinedSetupResult
   * @private
   */
  onCombinedSetupStateChanged_(combinedSetupResult) {
    if (this.flowState_ !== SetupFlowStatus.WAIT_FOR_PHONE_COMBINED) {
      return;
    }

    // When the combinedSetupResult is COMPLETED_SUCCESSFULLY or
    // COMPLETED_USER_REJECTED we should continue on with the setup flow if
    // there are additional features, all other results will change the screen
    // that is shown and pause or terminate the setup flow.
    if (combinedSetupResult !== PermissionsSetupStatus.COMPLETED_SUCCESSFULLY &&
        combinedSetupResult !==
            PermissionsSetupStatus.COMPLETED_USER_REJECTED) {
      this.setupState_ = combinedSetupResult;
      return;
    }

    // Note: we can only update this.setupState_ after assigning
    // this.completeMode_. Otherwise, we cannot use the final
    // this.completedMode_ to determine the completed title.
    if (combinedSetupResult === PermissionsSetupStatus.COMPLETED_SUCCESSFULLY) {
      if (this.setupMode_ & CAMERA_ROLL_FEATURE && !this.showCameraRoll) {
        this.completedMode_ |= CAMERA_ROLL_FEATURE;
        this.browserProxy_.setFeatureEnabledState(
            MultiDeviceFeature.PHONE_HUB_CAMERA_ROLL, true);
      }

      if (this.setupMode_ & NOTIFICATION_FEATURE && !this.showNotifications) {
        this.completedMode_ |= NOTIFICATION_FEATURE;
        this.browserProxy_.setFeatureEnabledState(
            MultiDeviceFeature.PHONE_HUB_NOTIFICATIONS, true);
      }
    }

    if (this.showAppStreaming) {
      // We still need to process the apps steaming onboarding flow, update
      // this.setupState_ to CONNECTION_REQUESTED first and wait for
      // onAppsSetupStateChanged_() callback to update this.setupState_.
      this.browserProxy_.attemptAppsSetup();
      this.flowState_ = SetupFlowStatus.WAIT_FOR_PHONE_APPS;
      this.setupState_ = PermissionsSetupStatus.CONNECTION_REQUESTED;
    } else {
      this.setupState_ = combinedSetupResult;
      // We don't need to deal with the apps streaming onboarding flow, so we
      // can log completed case here.
      this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
          this.setupScreen_, PhoneHubPermissionsSetupAction.SHOWN);
    }
  }

  /**
   * @return {boolean}
   * @private
   */
  computeHasStartedSetupAttempt_() {
    return this.flowState_ !== SetupFlowStatus.INTRO;
  }

  /**
   * @return {boolean}
   * @private
   */
  computeIsSetupAttemptInProgress_() {
    return this.setupState_ ===
        PermissionsSetupStatus.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE ||
        this.setupState_ === PermissionsSetupStatus.CONNECTING ||
        this.setupState_ === PermissionsSetupStatus.CONNECTION_REQUESTED;
  }

  /**
   * @return {boolean}
   * @private
   */
  computeIsSetupScreenLockInProgress_() {
    return this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN;
  }

  /**
   * @return {boolean}
   * @private
   */
  computeHasCompletedSetup_() {
    return this.setupState_ === PermissionsSetupStatus.COMPLETED_SUCCESSFULLY ||
        this.setupState_ === PermissionsSetupStatus.COMPLETED_USER_REJECTED ||
        this.setupState_ === PermissionsSetupStatus.FAILED_OR_CANCELLED;
  }

  /**
   * @return {boolean}
   * @private
   */
  computeIsNotificationAccessProhibited_() {
    return this.setupState_ ===
        PermissionsSetupStatus.NOTIFICATION_ACCESS_PROHIBITED;
  }

  /**
   * @return {boolean}
   * @private
   * */
  computeDidSetupAttemptFail_() {
    return this.setupState_ === PermissionsSetupStatus.TIMED_OUT_CONNECTING ||
        this.setupState_ === PermissionsSetupStatus.CONNECTION_DISCONNECTED ||
        this.setupState_ ===
        PermissionsSetupStatus.NOTIFICATION_ACCESS_PROHIBITED;
  }

  /** @private */
  nextPage_() {
    this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
        this.setupScreen_, PhoneHubPermissionsSetupAction.NEXT_OR_TRY_AGAIN);
    const isScreenLockRequired =
        this.isScreenLockRequired_();
    switch (this.flowState_) {
      case SetupFlowStatus.INTRO:
        if (this.showCameraRoll) {
          this.setupMode_ |= CAMERA_ROLL_FEATURE;
        }
        if (this.showNotifications) {
          this.setupMode_ |= NOTIFICATION_FEATURE;
        }
        if (this.showAppStreaming) {
          this.setupMode_ |= APPS_FEATURE;
        }
        if (isScreenLockRequired) {
          this.flowState_ = SetupFlowStatus.SET_LOCKSCREEN;
          return;
        }
        break;
      case SetupFlowStatus.SET_LOCKSCREEN:
        if (!this.isScreenLockEnabled_) {
          return;
        }
        if (this.isPinNumberSelected_ && !this.isSetPinDone_) {
          // When users select pin number and click next button, popup set pin
          // dialog.
          this.showSetupPinDialog_ = true;
          this.propagatePinNumberSelected_(true);
          return;
        }
        this.propagatePinNumberSelected_(false);
        this.isPasswordDialogShowing = false;
        break;
    }

    if ((this.showCameraRoll || this.showNotifications) &&
        this.combinedSetupSupported) {
      this.browserProxy_.attemptCombinedFeatureSetup(
          this.showCameraRoll, this.showNotifications);
      this.flowState_ = SetupFlowStatus.WAIT_FOR_PHONE_COMBINED;
      this.setupState_ = PermissionsSetupStatus.CONNECTION_REQUESTED;
    } else if (this.showNotifications && !this.combinedSetupSupported) {
      this.browserProxy_.attemptNotificationSetup();
      this.flowState_ = SetupFlowStatus.WAIT_FOR_PHONE_NOTIFICATION;
      this.setupState_ = PermissionsSetupStatus.CONNECTION_REQUESTED;
    } else if (this.showAppStreaming) {
      this.browserProxy_.attemptAppsSetup();
      this.flowState_ = SetupFlowStatus.WAIT_FOR_PHONE_APPS;
      this.setupState_ = PermissionsSetupStatus.CONNECTION_REQUESTED;
    }
  }

  /** @private */
  onCancelClicked_() {
    if (this.flowState_ === SetupFlowStatus.WAIT_FOR_PHONE_NOTIFICATION) {
      this.browserProxy_.cancelNotificationSetup();
    } else if (this.flowState_ === SetupFlowStatus.WAIT_FOR_PHONE_APPS) {
      this.browserProxy_.cancelAppsSetup();
    } else if (this.flowState_ === SetupFlowStatus.WAIT_FOR_PHONE_COMBINED) {
      this.browserProxy_.cancelCombinedFeatureSetup();
    }
    this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
        this.setupScreen_, PhoneHubPermissionsSetupAction.CANCEL);
    this.$.dialog.close();
  }

  /** @private */
  onDoneOrCloseButtonClicked_() {
    this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
        this.setupScreen_, PhoneHubPermissionsSetupAction.DONE);
    this.$.dialog.close();
  }

  /** @private */
  onLearnMoreClicked_() {
    this.browserProxy_.logPhoneHubPermissionSetUpScreenAction(
        this.setupScreen_, PhoneHubPermissionsSetupAction.LEARN_MORE);
    window.open(this.i18n('multidevicePhoneHubPermissionsLearnMoreURL'));
  }

  /** @private */
  onPinNumberSelected_(e) {
    e.stopPropagation();
    assert(typeof e.detail.isPinNumberSelected === 'boolean');
    this.isPinNumberSelected_ = e.detail.isPinNumberSelected;
  }

  /** @private */
  onSetPinDone_() {
    // Once users confirm pin number, take them to the 'finish setup on the
    // phone' step directly.
    this.isSetPinDone_ = true;
    this.nextPage_();
  }

  /** @private */
  propagatePinNumberSelected_(selected) {
    const pinNumberEvent = new CustomEvent('pin-number-selected', {
      bubbles: true,
      composed: true,
      detail: {isPinNumberSelected: selected},
    });
    this.dispatchEvent(pinNumberEvent);
  }

  /** @private */
  getCurrentScreen_() {
    if (this.flowState_ === SetupFlowStatus.INTRO) {
      return PhoneHubPermissionsSetupFlowScreens.INTRO;
    }

    if (this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN) {
      return PhoneHubPermissionsSetupFlowScreens.SET_A_PIN_OR_PASSWORD;
    }

    const Status = PermissionsSetupStatus;
    switch (this.setupState_) {
      case Status.CONNECTION_REQUESTED:
      case Status.CONNECTING:
        return PhoneHubPermissionsSetupFlowScreens.CONNECTING;
      case Status.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE:
        return PhoneHubPermissionsSetupFlowScreens.FINISH_SET_UP_ON_PHONE;
      case Status.COMPLETED_SUCCESSFULLY:
      case Status.COMPLETED_USER_REJECTED:
      case Status.FAILED_OR_CANCELLED:
        return PhoneHubPermissionsSetupFlowScreens.CONNECTED;
      case Status.TIMED_OUT_CONNECTING:
        return PhoneHubPermissionsSetupFlowScreens.CONNECTION_TIME_OUT;
      case Status.CONNECTION_DISCONNECTED:
        return PhoneHubPermissionsSetupFlowScreens.CONNECTION_ERROR;
      default:
        return PhoneHubPermissionsSetupFlowScreens.NOT_APPLICABLE;
    }
  }

  /**
   * @return {string} The title of the dialog.
   * @private
   */
  getTitle_() {
    if (this.flowState_ === SetupFlowStatus.INTRO) {
      return this.i18n('multidevicePermissionsSetupAckTitle');
    }
    if (this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN) {
      return this.i18n('multideviceNotificationAccessSetupScreenLockTitle');
    }

    const Status = PermissionsSetupStatus;
    switch (this.setupState_) {
      case Status.CONNECTION_REQUESTED:
      case Status.CONNECTING:
        return this.i18n('multideviceNotificationAccessSetupConnectingTitle');
      case Status.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE:
        return this.i18n('multidevicePermissionsSetupAwaitingResponseTitle');
      case Status.COMPLETED_SUCCESSFULLY:
      case Status.COMPLETED_USER_REJECTED:
      case Status.FAILED_OR_CANCELLED:
        return this.getSetupCompleteTitle_();
      case Status.TIMED_OUT_CONNECTING:
        return this.i18n(
            'multidevicePermissionsSetupCouldNotEstablishConnectionTitle');
      case Status.CONNECTION_DISCONNECTED:
        return this.i18n(
            'multideviceNotificationAccessSetupConnectionLostWithPhoneTitle');
      case Status.NOTIFICATION_ACCESS_PROHIBITED:
        return this.i18n(
            'multidevicePermissionsSetupNotificationAccessProhibitedTitle');
      default:
        return '';
    }
  }

  /**
   * @return {string} A description about the connection attempt state.
   * @private
   */
  getDescription_() {
    if (this.flowState_ === SetupFlowStatus.INTRO) {
      return '';
    }

    if (this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN) {
      return '';
    }

    const Status = PermissionsSetupStatus;
    switch (this.setupState_) {
      case Status.COMPLETED_SUCCESSFULLY:
      case Status.COMPLETED_USER_REJECTED:
      case Status.FAILED_OR_CANCELLED:
        return (this.setupMode_ === this.completedMode_) ?
            '' :
            this.i18n(
                'multidevicePermissionsSetupCompletedMoreFeaturesSummary');
      case Status.TIMED_OUT_CONNECTING:
        return this.i18n('multidevicePermissionsSetupEstablishFailureSummary');
      case Status.CONNECTION_DISCONNECTED:
        return this.i18n('multidevicePermissionsSetupMaintainFailureSummary');
      case Status.NOTIFICATION_ACCESS_PROHIBITED:
        return this.i18nAdvanced(
            'multidevicePermissionsSetupNotificationAccessProhibitedSummary');
      case Status.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE:
        return this.i18n('multidevicePermissionsSetupOperationsInstructions');
      case Status.CONNECTION_REQUESTED:
      case Status.CONNECTING:
        return this.i18n('multidevicePermissionsSetupInstructions');
      default:
        return '';
    }
  }

  /**
   * @return {boolean}
   * @private
   */
  computeShouldShowLearnMoreButton_() {
    return this.flowState_ === SetupFlowStatus.INTRO ||
        this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN ||
        this.setupState_ ===
        PermissionsSetupStatus.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE;
  }

  /**
   * @return {boolean}
   * @private
   */
  shouldShowCancelButton_() {
    return this.setupState_ !== PermissionsSetupStatus.COMPLETED_SUCCESSFULLY &&
        this.setupState_ !== PermissionsSetupStatus.COMPLETED_USER_REJECTED &&
        this.setupState_ !== PermissionsSetupStatus.FAILED_OR_CANCELLED &&
        this.setupState_ !==
        PermissionsSetupStatus.NOTIFICATION_ACCESS_PROHIBITED;
  }

  /**
   * @return {boolean}
   * @private
   */
  computeShouldShowDisabledDoneButton_() {
    return this.setupState_ ===
        PermissionsSetupStatus.SENT_MESSAGE_TO_PHONE_AND_WAITING_FOR_RESPONSE;
  }

  /**
   * @return {boolean}
   * @private
   */
  shouldShowTryAgainButton_() {
    return this.setupState_ === PermissionsSetupStatus.TIMED_OUT_CONNECTING ||
        this.setupState_ === PermissionsSetupStatus.CONNECTION_DISCONNECTED;
  }

  /**
   * @return {boolean}
   * @private
   */
  shouldShowScreenLockInstructions_() {
    return this.flowState_ === SetupFlowStatus.SET_LOCKSCREEN;
  }

  /**
   * @return {boolean}
   * @private
   */
  isScreenLockRequired_() {
    return loadTimeData.getBoolean('isEcheAppEnabled') &&
        this.isPhoneScreenLockEnabled && !this.isChromeosScreenLockEnabled &&
        this.showAppStreaming;
  }

  /**
   * @return {string} A aria label about learn more button.
   * @private
   */
  getLearnMoreButtonAriaLabel_() {
    return this.i18n('multidevicePhoneHubLearnMoreAriaLabel');
  }

  /**
   * @return {string} The finish title of the dialog.
   * @private
   */
  getSetupCompleteTitle_() {
    switch (this.completedMode_) {
      case NOTIFICATION_FEATURE:
        return this.i18n(
            'multidevicePermissionsSetupNotificationsCompletedTitle');
      case CAMERA_ROLL_FEATURE:
        return this.i18n('multidevicePermissionsSetupCameraRollCompletedTitle');
      case NOTIFICATION_FEATURE|CAMERA_ROLL_FEATURE:
        return this.i18n(
            'multidevicePermissionsSetupCameraRollAndNotificationsCompletedTitle');
      case APPS_FEATURE:
        return this.i18n('multidevicePermissionsSetupAppssCompletedTitle');
      case NOTIFICATION_FEATURE|APPS_FEATURE:
        return this.i18n(
            'multidevicePermissionsSetupNotificationsAndAppsCompletedTitle');
      case CAMERA_ROLL_FEATURE|APPS_FEATURE:
        return this.i18n(
            'multidevicePermissionsSetupCameraRollAndAppsCompletedTitle');
      case NOTIFICATION_FEATURE|CAMERA_ROLL_FEATURE|APPS_FEATURE:
        return this.i18n('multidevicePermissionsSetupAllCompletedTitle');
      default:
        return this.i18n(
            'multidevicePermissionsSetupAppssCompletedFailedTitle');
    }
  }

  /** @private */
  onAccessStateChanged_() {
    if (this.flowState_ === SetupFlowStatus.INTRO && !this.showCameraRoll &&
        !this.showNotifications && !this.showAppStreaming) {
      this.$.dialog.close();
    }
  }
}

customElements.define(
    SettingsMultidevicePermissionsSetupDialogElement.is,
    SettingsMultidevicePermissionsSetupDialogElement);

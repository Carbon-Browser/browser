// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/mojo/mojo/public/mojom/base/big_buffer.mojom-lite.js';
import 'chrome://resources/mojo/mojo/public/mojom/base/string16.mojom-lite.js';

import {fakeEmptyFeedbackContext, fakeFeedbackContext} from 'chrome://os-feedback/fake_data.js';
import {FakeFeedbackServiceProvider} from 'chrome://os-feedback/fake_feedback_service_provider.js';
import {FeedbackFlowState} from 'chrome://os-feedback/feedback_flow.js';
import {FeedbackContext} from 'chrome://os-feedback/feedback_types.js';
import {setFeedbackServiceProviderForTesting} from 'chrome://os-feedback/mojo_interface_provider.js';
import {ShareDataPageElement} from 'chrome://os-feedback/share_data_page.js';
import {mojoString16ToString, stringToMojoString16} from 'chrome://resources/ash/common/mojo_utils.js';
import {getDeepActiveElement} from 'chrome://resources/js/util.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {assertArrayEquals, assertEquals, assertFalse, assertTrue} from '../../chai_assert.js';
import {eventToPromise, flushTasks, isVisible} from '../../test_util.js';

/** @type {string} */
const fakeImageUrl = 'chrome://os_feedback/app_icon_48.png';

export function shareDataPageTestSuite() {
  /** @type {?ShareDataPageElement} */
  let page = null;

  /** @type {?FakeFeedbackServiceProvider} */
  let feedbackServiceProvider;

  setup(() => {
    document.body.innerHTML = '';

    feedbackServiceProvider = new FakeFeedbackServiceProvider();
    setFeedbackServiceProviderForTesting(feedbackServiceProvider);
  });

  teardown(() => {
    page.remove();
    page = null;
  });

  function initializePage() {
    assertFalse(!!page);
    page =
        /** @type {!ShareDataPageElement} */ (
            document.createElement('share-data-page'));
    assertTrue(!!page);
    document.body.appendChild(page);
    return flushTasks();
  }

  /**
   * @param {string} selector
   * @returns {Element|null}
   */
  function getElement(selector) {
    const element = page.shadowRoot.querySelector(selector);
    return element;
  }

  /**
   * @param {string} selector
   * @returns {string}
   */
  function getElementContent(selector) {
    const element = getElement(selector);
    assertTrue(!!element);
    return element.textContent.trim();
  }

  /**
   * Helper function which will click the send button, wait for the event
   * 'continue-click', and return the detail data of the event.
   * @param {!Element} element
   */
  async function clickSendAndWait(element) {
    const clickPromise = eventToPromise('continue-click', element);

    let eventDetail;
    page.addEventListener('continue-click', (event) => {
      eventDetail = event.detail;
    });

    getElement('#buttonSend').click();

    await clickPromise;

    assertTrue(!!eventDetail);
    assertEquals(FeedbackFlowState.SHARE_DATA, eventDetail.currentState);

    return eventDetail;
  }

  // Test the page is loaded with expected HTML elements.
  test('shareDataPageLoaded', async () => {
    await initializePage();
    // Verify the title is in the page.
    assertEquals('Send feedback', getElementContent('.page-title'));
    assertTrue(page.i18nExists('pageTitle'));

    // Verify the back button is in the page.
    assertEquals('Back', getElementContent('#buttonBack'));
    assertTrue(page.i18nExists('backButtonLabel'));

    // Verify the send button is in the page.
    assertEquals('Send', getElementContent('#buttonSend'));
    assertTrue(page.i18nExists('sendButtonLabel'));

    // Verify the attach files label is in the page.
    assertTrue(page.i18nExists('attachFilesLabel'));
    assertEquals('Attach files', getElementContent('#attachFilesLabel'));

    // Verify the user email label is in the page.
    assertTrue(page.i18nExists('userEmailLabel'));
    assertEquals('Email', getElementContent('#userEmailLabel'));

    // Verify don't include email address is in the page.
    assertTrue(page.i18nExists('anonymousUser'));
    assertEquals(
        `Don't include email address`, getElementContent('#anonymousUser'));

    // Verify the share diagnostic data label is in the page.
    assertTrue(page.i18nExists('shareDiagnosticDataLabel'));
    assertEquals(
        'Share diagnostic data',
        getElementContent('#shareDiagnosticDataLabel'));

    // Screenshot elements.
    const screenshotCheckbox = getElement('#screenshotCheckbox');
    assertTrue(!!screenshotCheckbox);
    assertTrue(page.i18nExists('attachScreenshotCheckboxAriaLabel'));
    assertEquals('Attach screenshot', screenshotCheckbox.ariaLabel);

    assertTrue(page.i18nExists('attachScreenshotLabel'));
    assertEquals('Screenshot', getElementContent('#screenshotCheckLabel'));

    assertTrue(!!getElement('#screenshotImage'));
    assertEquals('Preview Screenshot', getElement('#imageButton').ariaLabel);
    assertTrue(page.i18nExists('previewImageAriaLabel'));

    // Add file attachment element.
    assertTrue(!!getElement('file-attachment'));

    // Email elements.
    assertEquals('Email', getElementContent('#userEmailLabel'));
    assertTrue(!!getElement('#userEmailDropDown'));

    // URL elements.
    assertEquals('Share URL:', getElementContent('#pageUrlLabel'));
    assertTrue(page.i18nExists('sharePageUrlLabel'));
    assertTrue(!!getElement('#pageUrlCheckbox'));
    assertTrue(!!getElement('#pageUrlText'));

    // System info label is a localized string in HTML format.
    assertTrue(getElementContent('#sysInfoCheckboxLabel').length > 0);

    // Privacy note is a long localized string in HTML format.
    assertTrue(page.i18nExists('privacyNote'));
    assertEquals(
        'Go to the Legal Help page to request content changes for ' +
            'legal reasons. Some account and system information ' +
            'may be sent to Google. We will use the information you ' +
            'give us to help address technical issues and to improve our ' +
            'services, subject to our Privacy Policy and Terms of Service.',
        getElementContent('#privacyNote'));
  });

  // Test that the email drop down is populated with two options.
  test('emailDropdownPopulated', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    const emailDropdown = getElement('#userEmailDropDown');
    assertTrue(!!emailDropdown);
    assertEquals(2, emailDropdown.options.length);

    const firstOption = emailDropdown.options.item(0);
    assertEquals('test.user2@test.com', firstOption.textContent.trim());
    assertEquals('test.user2@test.com', firstOption.value.trim());

    const secondOption = emailDropdown.options.item(1);
    assertEquals(
        'Don\'t include email address', secondOption.textContent.trim());
    assertEquals('', secondOption.value.trim());

    // The user email section should be visible.
    const userEmailElement = getElement('#userEmail');
    assertTrue(!!userEmailElement);
    assertTrue(isVisible(userEmailElement));
  });

  // Test that the email section is hidden when there is no email.
  test('emailSectionHiddenWithoutEmail', async () => {
    await initializePage();
    page.feedbackContext = fakeEmptyFeedbackContext;

    // The user email section should be hidden.
    const userEmailElement = getElement('#userEmail');
    assertTrue(!!userEmailElement);
    assertFalse(isVisible(userEmailElement));
  });

  test('pageUrlPopulated', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    assertEquals('chrome://tab/', getElementContent('#pageUrlText'));
  });

  // Test that the pageUrl section is hidden when the url is empty string.
  test('pageUrlHidden', async () => {
    await initializePage();
    fakeFeedbackContext.pageUrl.url = '';
    page.feedbackContext = fakeFeedbackContext;

    // The pageUrl section should be hidden
    const pageUrl = getElement('#pageUrl');
    assertTrue(!!pageUrl);
    assertFalse(isVisible(pageUrl));

    // Change it back otherwise it will effect other tests.
    fakeFeedbackContext.pageUrl.url = 'chrome://tab/';
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 1: Share pageUrl, do not share system logs.
   */
  test('SendReportSharePageUrlButNotSystemLogs', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    getElement('#pageUrlCheckbox').checked = true;
    getElement('#sysInfoCheckbox').checked = false;

    const eventDetail = await clickSendAndWait(page);

    assertEquals(
        'chrome://tab/', eventDetail.report.feedbackContext.pageUrl.url);
    assertFalse(eventDetail.report.includeSystemLogsAndHistograms);
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 2: Share system logs, do not share pageUrl.
   */
  test('SendReportShareSystemLogsButNotPageUrl', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    getElement('#pageUrlCheckbox').checked = false;
    getElement('#sysInfoCheckbox').checked = true;

    const request = (await clickSendAndWait(page)).report;

    assertFalse(!!request.feedbackContext.pageUrl);
    assertTrue(request.includeSystemLogsAndHistograms);
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 3: Share email and screenshot.
   */
  test('SendReportShareEmail', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;
    page.screenshotUrl = fakeImageUrl;
    assertEquals(fakeImageUrl, getElement('#screenshotImage').src);

    // Select the email.
    getElement('#userEmailDropDown').value = 'test.user2@test.com';
    // Select the screenshot.
    getElement('#screenshotCheckbox').checked = true;

    const request = (await clickSendAndWait(page)).report;

    assertEquals('test.user2@test.com', request.feedbackContext.email);
    assertTrue(request.includeScreenshot);
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 4: Do not share email or screenshot.
   * 4.1) No screenshot and screenshot checkbox is unchecked.
   */
  test('SendReportDoNotShareEmailNoScreenshotUnchecked', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;
    // When there is not a screenshot.
    page.screenshotUrl = '';
    assertFalse(!!getElement('#screenshotImage').src);

    // Select the "Don't include email address" option.
    getElement('#userEmailDropDown').value = '';

    const request = (await clickSendAndWait(page)).report;

    assertFalse(!!request.feedbackContext.email);
    assertFalse(request.includeScreenshot);
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 4: Do not share email or screenshot.
   * 4.2) No screenshot and screenshot checkbox is checked.
   */
  test('SendReportDoNotShareEmailNoScreenshotChecked', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;
    // When there is not a screenshot.
    page.screenshotUrl = '';
    assertFalse(!!getElement('#screenshotImage').src);

    // Select the "Don't include email address" option.
    getElement('#userEmailDropDown').value = '';

    // When the checkbox is selected but there is not a screenshot.
    getElement('#screenshotCheckbox').checked = true;
    assertFalse(!!getElement('#screenshotImage').src);

    const request = (await clickSendAndWait(page)).report;

    assertFalse(!!request.feedbackContext.email);
    assertFalse(request.includeScreenshot);
  });

  /**
   * Test that when when the send button is clicked, an on-continue is fired.
   * Case 4: Do not share email or screenshot.
   * 4.3) Has screenshot but screenshot checkbox is unchecked.
   */
  test('SendReportDoNotShareEmailHasScreenshotUnchecked', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    // Select the "Don't include email address" option.
    getElement('#userEmailDropDown').value = '';

    // When there is a screenshot but it is not selected.
    page.screenshotUrl = fakeImageUrl;
    assertEquals(fakeImageUrl, getElement('#screenshotImage').src);
    getElement('#screenshotCheckbox').checked = false;

    const request = (await clickSendAndWait(page)).report;

    assertFalse(!!request.feedbackContext.email);
    assertFalse(request.includeScreenshot);
  });

  // Test that the send button will be disabled once clicked.
  test('DisableSendButtonAfterClick', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    const sendButton = getElement('#buttonSend');

    assertFalse(sendButton.disabled);

    await clickSendAndWait(page);

    assertTrue(sendButton.disabled);
  });

  // Test that the screenshot checkbox is disabled when no screenshot.
  test('screenshotNotAvailable', async () => {
    await initializePage();
    page.screenshotUrl = '';

    const screenshotCheckbox = getElement('#screenshotCheckbox');
    assertTrue(screenshotCheckbox.disabled);

    const screenshotImage = getElement('#screenshotImage');
    assertFalse(!!screenshotImage.src);
  });

  // Test that the screenshot checkbox is enabled when there is a screenshot.
  test('screenshotAvailable', async () => {
    await initializePage();

    const imgUrl = 'chrome://os-feedback/image.png';
    page.screenshotUrl = imgUrl;

    const screenshotCheckbox = getElement('#screenshotCheckbox');
    assertFalse(screenshotCheckbox.disabled);

    const screenshotImage = getElement('#screenshotImage');
    assertTrue(!!screenshotImage.src);
    assertEquals(imgUrl, screenshotImage.src);
  });


  // Test that clicking the screenshot will open preview dialog and set the
  // focus on the close dialog icon button.
  test('screenshotPreview', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;
    page.screenshotUrl = fakeImageUrl;
    assertEquals(fakeImageUrl, getElement('#screenshotImage').src);

    const closeDialogButton = getElement('#closeDialogButton');
    // The preview dialog's close icon button is not visible.
    assertFalse(isVisible(closeDialogButton));

    // The screenshot is displayed as an image button.
    const imageButton = /** @type {!Element} */ (getElement('#imageButton'));
    const imageClickPromise = eventToPromise('click', imageButton);
    imageButton.click();
    await imageClickPromise;

    // The preview dialog's close icon button is visible now.
    assertTrue(isVisible(closeDialogButton));
    // The preview dialog's close icon button is focused.
    assertEquals(closeDialogButton, getDeepActiveElement());

    // Press enter should close the preview dialog.
    closeDialogButton.dispatchEvent(
        new KeyboardEvent('keydown', {key: 'Enter'}));
    await flushTasks();

    // The preview dialog's close icon button is not visible now.
    assertFalse(isVisible(closeDialogButton));
  });

  /**
   * Test that when when the send button is clicked, the getAttachedFile has
   * been called.
   */
  test('getAttachedFileCalled', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    const fileAttachment = getElement('file-attachment');
    const fakeFileData = [11, 22, 99];
    fileAttachment.getAttachedFile = async () => {
      return {
        fileName: stringToMojoString16('fake.zip'),
        fileData: {
          bytes: fakeFileData,
        },
      };
    };

    const request = (await clickSendAndWait(page)).report;

    const attachedFile = request.attachedFile;
    assertTrue(!!attachedFile);
    assertEquals('fake.zip', mojoString16ToString(attachedFile.fileName));
    assertArrayEquals(
        fakeFileData,
        /** @type {!Array<Number>} */ (attachedFile.fileData.bytes));
  });

  /**
   * Test that when page initially loaded "user-consent" checkbox is false and
   * expected localize message displayed.
   */
  test('UserConsentGrantedCheckbox_StartsFalse', async () => {
    const expectedUserConsentMessage =
        'We may email you for more information or updates';
    await initializePage();

    assertTrue(page.i18nExists('userConsentLabel'));

    const userConsentCheckboxChecked =
        getElement('#userConsentCheckbox').checked;
    const userConsentText = getElementContent('#userConsentLabel');

    assertFalse(userConsentCheckboxChecked);
    assertEquals(expectedUserConsentMessage, userConsentText);
  });

  /**
   * Test that report "contact_user_consent_granted" matches "user-consent"
   * checkbox value.
   */
  test(
      'UserConsentGrantedCheckbox_UpdatesReportContactUserConsentGranted',
      async () => {
        await initializePage();
        page.feedbackContext = fakeFeedbackContext;
        getElement('#userConsentCheckbox').checked = true;
        await flushTasks();

        const reportWithConsent = (await clickSendAndWait(page)).report;

        assertTrue(reportWithConsent.contactUserConsentGranted);

        page.reEnableSendReportButton();
        page.feedbackContext = fakeFeedbackContext;
        getElement('#userConsentCheckbox').checked = false;
        await flushTasks();

        const reportWithoutConsent = (await clickSendAndWait(page)).report;
        assertFalse(reportWithoutConsent.contactUserConsentGranted);
      });

  /**
   * Test that when report is anonymous (no email provided), "user-consent"
   * checkbox is disabled and value is false.
   */
  test(
      'UserConsentGrantedCheckbox_ReportAnonymous_FalseAndDisabled',
      async () => {
        await initializePage();
        page.feedbackContext = fakeFeedbackContext;
        const disabledInputClass = 'disabled-input-text';

        const consentLabel = getElement('#userConsentLabel');
        const consentCheckbox = getElement('#userConsentCheckbox');
        const emailDropdown = getElement('#userEmailDropDown');

        // Select the email.
        emailDropdown.value = 'test.user2@test.com';
        await flushTasks();

        // With email selected and consent not granted.
        assertFalse(consentCheckbox.disabled);
        assertFalse(consentCheckbox.checked);
        assertFalse(consentLabel.classList.contains(disabledInputClass));

        // Check checkbox.
        consentCheckbox.click();
        await flushTasks();

        // With email selected and consent granted.
        assertFalse(consentCheckbox.disabled);
        assertTrue(consentCheckbox.checked);
        assertFalse(consentLabel.classList.contains(disabledInputClass));

        // Select the "Do Not Provide Email" option.
        emailDropdown.value = '';
        emailDropdown.dispatchEvent(new CustomEvent('change'));
        flush();

        // With anonymous email selected and consent not granted.
        assertTrue(consentCheckbox.disabled);
        assertFalse(consentCheckbox.checked);
        assertTrue(consentLabel.classList.contains(disabledInputClass));
      });

  /**
   * Test that when feedback context contains extra_diagnostics matching value
   * is set on report.
   */
  test('AdditionalContext_ExtraDiagnostics', async () => {
    await initializePage();
    page.feedbackContext = fakeFeedbackContext;

    const reportWithoutExtraDiagnostics = (await clickSendAndWait(page)).report;
    assertFalse(
        !!reportWithoutExtraDiagnostics.feedbackContext.extraDiagnostics);

    page.reEnableSendReportButton();
    const fakeFeedbackContextWithExtraDiagnostics =
        /** @type {!FeedbackContext} */ ({extraDiagnostics: 'some extra info'});
    page.feedbackContext = fakeFeedbackContextWithExtraDiagnostics;
    await flushTasks();

    const reportWithExtraDiagnostics = (await clickSendAndWait(page)).report;
    assertEquals(
        fakeFeedbackContextWithExtraDiagnostics.extraDiagnostics,
        reportWithExtraDiagnostics.feedbackContext.extraDiagnostics);

    getElement('#sysInfoCheckbox').checked = false;
    page.reEnableSendReportButton();
    const reportNoSysInfo = (await clickSendAndWait(page)).report;
    assertFalse(!!reportNoSysInfo.feedbackContext.extraDiagnostics);
  });

  /**
   * Test that feedbackServiceProvider.openMetricsDialog is called
   * when #histogramsLink ("metrics") link is clicked.
   */
  test('openMetricsDialog', async () => {
    await initializePage();

    assertEquals(0, feedbackServiceProvider.getOpenMetricsDialogCallCount());

    getElement('#histogramsLink').click();

    assertEquals(1, feedbackServiceProvider.getOpenMetricsDialogCallCount());
  });

  /**
   * Test that feedbackServiceProvider.openSystemInfoDialog is called
   * when #sysInfoLink ("system and app info") link is clicked.
   */
  test('openSystemInfoDialog', async () => {
    await initializePage();

    assertEquals(0, feedbackServiceProvider.getOpenSystemInfoDialogCallCount());

    getElement('#sysInfoLink').click();

    assertEquals(1, feedbackServiceProvider.getOpenSystemInfoDialogCallCount());
  });
}

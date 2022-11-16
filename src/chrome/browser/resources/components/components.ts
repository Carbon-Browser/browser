// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './strings.m.js';

import {assert} from 'chrome://resources/js/assert_ts.js';
import {addWebUIListener, isChromeOS, sendWithPromise} from 'chrome://resources/js/cr.m.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {$} from 'chrome://resources/js/util.m.js';

declare global {
  class JsEvalContext {
    constructor(data: any);
  }
  function jstProcess(context: JsEvalContext, template: HTMLElement): void;

  const trustedTypes: {emptyHTML: string};
}

type Component = {
  id: string,
  name: string,
  status: string,
  version: string,
};

type ComponentsData = {
  components: Component[],
  showOsLink: boolean,
};

/**
 * An array of the latest component data including ID, name, status and
 * version. This is populated in returnComponentsData() for the convenience of
 * tests.
 */
let currentComponentsData: Component[]|null = null;

/**
 * Takes the |componentsData| input argument which represents data about the
 * currently installed components and populates the html jstemplate with
 * that data. It expects an object structure like the above.
 * @param componentsData Detailed info about installed components.
 *      Same expected format as returnComponentsData().
 */
function renderTemplate(componentsData: ComponentsData) {
  // This is the javascript code that processes the template:
  const input = new JsEvalContext(componentsData);
  const output =
      document.body.querySelector<HTMLElement>(
                       '#component-template')!.cloneNode(true) as HTMLElement;
  $('component-placeholder').innerHTML = trustedTypes.emptyHTML;
  $('component-placeholder').appendChild(output);
  jstProcess(input, output);
  output.removeAttribute('hidden');

  // <if expr="chromeos_ash or chromeos_lacros">
  const crosUrlRedirectButton = $('os-link-href');
  if (crosUrlRedirectButton) {
    crosUrlRedirectButton.onclick = crosUrlComponentRedirect;
  }
  // </if>
}

// <if expr="chromeos_ash or chromeos_lacros">
/**
 * Called when the user clicks on the os-link-href button.
 */
function crosUrlComponentRedirect() {
  chrome.send('crosUrlComponentsRedirect');
}
// </if>

/**
 * Asks the C++ ComponentsDOMHandler to get details about the installed
 * components.
 */
function requestComponentsData() {
  sendWithPromise('requestComponentsData').then(returnComponentsData);
}

/**
 * Called by the WebUI to re-populate the page with data representing the
 * current state of installed components. The componentsData will also be
 * stored in currentComponentsData to be available to JS for testing purposes.
 * @param componentsData Detailed info about installed components.
 */
function returnComponentsData(componentsData: ComponentsData) {
  const bodyContainer = $('body-container');
  const body = document.body;

  bodyContainer.style.visibility = 'hidden';
  body.className = '';

  // Initialize |currentComponentsData|, which can also be updated in
  // onComponentEvent() later.
  currentComponentsData = componentsData.components;

  renderTemplate(componentsData);

  // Add handlers to dynamically created HTML elements.
  const links =
      document.body.querySelectorAll<HTMLButtonElement>('.button-check-update');
  for (const link of links) {
    link.onclick = function(e) {
      handleCheckUpdate(link);
      e.preventDefault();
    };
  }

  // Disable some controls for Guest mode in ChromeOS.
  if (isChromeOS && loadTimeData.getBoolean('isGuest')) {
    document.body.querySelectorAll<HTMLButtonElement>('[guest-disabled]')
        .forEach(function(element) {
          element.disabled = true;
        });
  }

  const systemFlagsLinkDiv = $('os-link-container');
  if (systemFlagsLinkDiv) {
    systemFlagsLinkDiv.hidden = !componentsData.showOsLink;
  }

  bodyContainer.style.visibility = 'visible';
  body.className = 'show-tmi-mode-initial';
}

type ComponentEvent = {
  event: string,
  id?: string,
  version?: string,
};

/**
 * Listener called when state of component updater service changes.
 * @param event Contains event and component ID. Component ID is
 *     optional.
 */
function onComponentEvent(event: ComponentEvent) {
  if (!event.id) {
    return;
  }

  const id = event.id;

  assert(currentComponentsData);
  const filteredComponents = currentComponentsData.filter(function(entry) {
    return entry.id === id;
  });

  // A component may be added from another page so the status and version
  // should only be updated if the component is listed on this page.
  if (filteredComponents.length === 0) {
    return;
  }

  const component = filteredComponents[0];
  assert(component);

  const status = event.event;
  $('status-' + id).textContent = status;
  component.status = status;

  if (event.version) {
    const version = event.version;
    $('version-' + id).textContent = version;
    component.version = version;
  }
}

/**
 * Handles an 'enable' or 'disable' button getting clicked.
 * @param node The HTML element representing the component being checked for
 *     update.
 */
function handleCheckUpdate(node: HTMLElement) {
  $('status-' + String(node.id)).textContent =
      loadTimeData.getString('checkingLabel');

  // Tell the C++ ComponentssDOMHandler to check for update.
  chrome.send('checkUpdate', [String(node.id)]);
}

// Get data and have it displayed upon loading.
document.addEventListener('DOMContentLoaded', function() {
  addWebUIListener('component-event', onComponentEvent);
  requestComponentsData();
});

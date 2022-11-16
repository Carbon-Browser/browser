// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <if expr="is_ios">
import 'chrome://resources/js/ios/web_ui.js';
// </if>

import {addWebUIListener} from 'chrome://resources/js/cr.m.js';
import {$} from 'chrome://resources/js/util.m.js';

import {DownloadInternalsBrowserProxy, DownloadInternalsBrowserProxyImpl, ServiceEntry, ServiceEntryState, ServiceRequest, ServiceStatus} from './download_internals_browser_proxy.js';

declare global {
  class JsEvalContext {
    constructor(data: any);
  }

  function jstProcess(context: JsEvalContext, template: HTMLElement): void;
}

const browserProxy: DownloadInternalsBrowserProxy =
    DownloadInternalsBrowserProxyImpl.getInstance();

const ongoingServiceEntries: ServiceEntry[] = [];
const finishedServiceEntries: ServiceEntry[] = [];
const serviceRequests: ServiceRequest[] = [];

/**
 * @param list A list to remove the entry from.
 * @param guid The guid to remove from the list.
 */
function removeGuidFromList(list: ServiceEntry[], guid: string) {
  const index = list.findIndex(entry => entry.guid == guid);
  if (index != -1) {
    list.splice(index, 1);
  }
}

/**
 * Replaces the ServiceEntry specified by guid in the list or, if it's not
 * found, adds a new entry.
 * @param list A list to update.
 */
function addOrUpdateEntryByGuid(list: ServiceEntry[], newEntry: ServiceEntry) {
  const index = list.findIndex(entry => entry.guid == newEntry.guid);
  if (index != -1) {
    list[index] = newEntry;
  } else {
    list.unshift(newEntry);
  }
}

function updateEntryTables() {
  const ongoingInput = new JsEvalContext({entries: ongoingServiceEntries});
  jstProcess(ongoingInput, $('download-service-ongoing-entries-info'));

  const finishedInput = new JsEvalContext({entries: finishedServiceEntries});
  jstProcess(finishedInput, $('download-service-finished-entries-info'));
}

/**
 * @param state The current status of the download service.
 */
function onServiceStatusChanged(state: ServiceStatus) {
  $('service-state').textContent = state.serviceState;
  $('service-status-model').textContent = state.modelStatus;
  $('service-status-driver').textContent = state.driverStatus;
  $('service-status-file').textContent = state.fileMonitorStatus;
}

/**
 * @param entries A list entries currently tracked by the download service.
 */
function onServiceDownloadsAvailable(entries: ServiceEntry[]) {
  for (let i = 0; i < entries.length; i++) {
    const entry = entries[i]!;
    if (entry.state == ServiceEntryState.COMPLETE) {
      finishedServiceEntries.unshift(entry);
    } else {
      ongoingServiceEntries.unshift(entry);
    }
  }

  updateEntryTables();
}

/**
 * @param entry The new state for a particular download service entry.
 */
function onServiceDownloadChanged(entry: ServiceEntry) {
  if (entry.state == ServiceEntryState.COMPLETE) {
    removeGuidFromList(ongoingServiceEntries, entry.guid);
    addOrUpdateEntryByGuid(finishedServiceEntries, entry);
  } else {
    addOrUpdateEntryByGuid(ongoingServiceEntries, entry);
  }

  updateEntryTables();
}

/**
 * @param entry The new state for a failed download service entry.
 */
function onServiceDownloadFailed(entry: ServiceEntry) {
  removeGuidFromList(ongoingServiceEntries, entry.guid);
  addOrUpdateEntryByGuid(finishedServiceEntries, entry);

  updateEntryTables();
}

/**
 * @param request The state for a newly issued download service request.
 */
function onServiceRequestMade(request: ServiceRequest) {
  serviceRequests.unshift(request);
  const input = new JsEvalContext({requests: serviceRequests});
  jstProcess(input, $('download-service-request-info'));
}

function initialize() {
  // Register all event listeners.
  addWebUIListener('service-status-changed', onServiceStatusChanged);
  addWebUIListener('service-downloads-available', onServiceDownloadsAvailable);
  addWebUIListener('service-download-changed', onServiceDownloadChanged);
  addWebUIListener('service-download-failed', onServiceDownloadFailed);
  addWebUIListener('service-request-made', onServiceRequestMade);

  $('start-download').onclick = function() {
    browserProxy.startDownload(($('download-url') as HTMLInputElement).value);
  };

  // Kick off requests for the current system state.
  browserProxy.getServiceStatus().then(onServiceStatusChanged);
  browserProxy.getServiceDownloads().then(onServiceDownloadsAvailable);
}

document.addEventListener('DOMContentLoaded', initialize);

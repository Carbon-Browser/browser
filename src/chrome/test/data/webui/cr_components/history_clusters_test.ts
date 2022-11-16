// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://history/strings.m.js';
import 'chrome://webui-test/mojo_webui_test_support.js';

import {BrowserProxyImpl} from 'chrome://resources/cr_components/history_clusters/browser_proxy.js';
import {HistoryClustersElement} from 'chrome://resources/cr_components/history_clusters/clusters.js';
import {Cluster, PageCallbackRouter, PageHandlerRemote, QueryResult, RawVisitData, URLVisit} from 'chrome://resources/cr_components/history_clusters/history_clusters.mojom-webui.js';
import {assertEquals} from 'chrome://webui-test/chai_assert.js';
import {TestBrowserProxy} from 'chrome://webui-test/test_browser_proxy.js';
import {flushTasks} from 'chrome://webui-test/test_util.js';

let handler: PageHandlerRemote&TestBrowserProxy;
let callbackRouterRemote: PageCallbackRouter;

function createBrowserProxy() {
  handler = TestBrowserProxy.fromClass(PageHandlerRemote);
  const callbackRouter = new PageCallbackRouter();
  BrowserProxyImpl.setInstance(new BrowserProxyImpl(handler, callbackRouter));
  callbackRouterRemote = callbackRouter.$.bindNewPipeAndPassRemote();
}

suite('history-clusters', () => {
  setup(() => {
    document.body.innerHTML = '';

    createBrowserProxy();
  });

  function getTestResult() {
    const cluster1 = new Cluster();
    const urlVisit1 = new URLVisit();
    urlVisit1.normalizedUrl = {url: 'https://www.google.com'};
    urlVisit1.urlForDisplay = 'https://www.google.com';
    urlVisit1.pageTitle = '';
    urlVisit1.titleMatchPositions = [];
    urlVisit1.urlForDisplayMatchPositions = [];
    urlVisit1.duplicates = [];
    urlVisit1.relativeDate = '';
    urlVisit1.annotations = [];
    urlVisit1.hidden = false;
    urlVisit1.debugInfo = {};
    const rawVisitData = new RawVisitData();
    rawVisitData.url = {url: ''};
    rawVisitData.visitTime = {internalValue: BigInt(0)};
    urlVisit1.rawVisitData = rawVisitData;
    cluster1.visits = [urlVisit1];
    cluster1.labelMatchPositions = [];
    cluster1.relatedSearches = [];
    const cluster2 = new Cluster();
    cluster2.visits = [];
    cluster2.labelMatchPositions = [];
    cluster2.relatedSearches = [];

    const queryResult = new QueryResult();
    queryResult.query = '';
    queryResult.clusters = [cluster1, cluster2];

    return queryResult;
  }

  async function setupClustersElement() {
    const clustersElement = new HistoryClustersElement();
    document.body.appendChild(clustersElement);

    const query = await handler.whenCalled('startQueryClusters');
    assertEquals(query, '');

    callbackRouterRemote.onClustersQueryResult(getTestResult());
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    // Make the handler ready for new assertions.
    handler.reset();

    return clustersElement;
  }

  test('List displays one element per cluster', async () => {
    const clustersElement = await setupClustersElement();

    const ironListItems = clustersElement.$.clusters.items!;
    assertEquals(ironListItems.length, 2);
  });

  test('Externally deleted history triggers re-query', async () => {
    // We don't directly reference the clusters element here.
    await setupClustersElement();

    callbackRouterRemote.onHistoryDeleted();
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    const newQuery = await handler.whenCalled('startQueryClusters');
    assertEquals(newQuery, '');
  });

  test('Non-empty query', async () => {
    const clustersElement = await setupClustersElement();
    clustersElement.query = 'foobar';

    const query = await handler.whenCalled('startQueryClusters');
    assertEquals(query, 'foobar');

    callbackRouterRemote.onClustersQueryResult(getTestResult());
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    // When History is externally deleted, we should hit the backend with the
    // same query again.
    handler.reset();
    callbackRouterRemote.onHistoryDeleted();
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    const newQuery = await handler.whenCalled('startQueryClusters');
    assertEquals(newQuery, 'foobar');
  });

  test('Navigate to url visit via click', async () => {
    const clustersElement = await setupClustersElement();

    callbackRouterRemote.onClustersQueryResult(getTestResult());
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    const urlVisit =
        clustersElement.$.clusters.querySelector('history-cluster')!.$.container
            .querySelector('url-visit');
    const urlVisitHeader =
        urlVisit!.shadowRoot!.querySelector<HTMLElement>('#header');

    urlVisitHeader!.click();

    const openHistoryClusterArgs =
        await handler.whenCalled('openHistoryCluster');

    assertEquals(urlVisit!.$.url.innerHTML, openHistoryClusterArgs[0].url);
    assertEquals(1, handler.getCallCount('openHistoryCluster'));
  });

  test('Navigate to url visit via keyboard', async () => {
    const clustersElement = await setupClustersElement();

    callbackRouterRemote.onClustersQueryResult(getTestResult());
    await callbackRouterRemote.$.flushForTesting();
    flushTasks();

    const urlVisit =
        clustersElement.$.clusters.querySelector('history-cluster')!.$.container
            .querySelector('url-visit');
    const urlVisitHeader =
        urlVisit!.shadowRoot!.querySelector<HTMLElement>('#header');

    // First url visit is selected.
    urlVisitHeader!.focus();

    const shiftEnter = new KeyboardEvent('keydown', {
      bubbles: true,
      cancelable: true,
      composed: true,  // So it propagates across shadow DOM boundary.
      key: 'Enter',
      shiftKey: true,
    });
    urlVisitHeader!.dispatchEvent(shiftEnter);

    // Navigates to the first match is selected.
    const openHistoryClusterArgs =
        await handler.whenCalled('openHistoryCluster');

    assertEquals(urlVisit!.$.url.innerHTML, openHistoryClusterArgs[0].url);
    assertEquals(true, openHistoryClusterArgs[1].shiftKey);
    assertEquals(1, handler.getCallCount('openHistoryCluster'));
  });
});

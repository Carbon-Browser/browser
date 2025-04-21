// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '/strings.m.js';
import './header.js';
import './loading_state.js';
import './new_column_selector.js';
import './product_selector.js';
import './table.js';
import './horizontal_carousel.js';
import './comparison_table_list.js';
import 'chrome://resources/cr_elements/cr_hidden_style.css.js';
import 'chrome://resources/cr_elements/cr_feedback_buttons/cr_feedback_buttons.js';
import 'chrome://resources/cr_elements/cr_toast/cr_toast.js';
import './shared_vars.css.js';

import {ColorChangeUpdater} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';
import type {PageCallbackRouter} from 'chrome://resources/cr_components/commerce/product_specifications.mojom-webui.ts';
import type {ProductSpecificationsBrowserProxy} from 'chrome://resources/cr_components/commerce/product_specifications_browser_proxy.js';
import {ProductSpecificationsBrowserProxyImpl} from 'chrome://resources/cr_components/commerce/product_specifications_browser_proxy.js';
import type {ProductInfo, ProductSpecificationsSet} from 'chrome://resources/cr_components/commerce/shared.mojom-webui.js';
import {UserFeedback} from 'chrome://resources/cr_components/commerce/shopping_service.mojom-webui.js';
import type {ProductSpecificationsFeatureState} from 'chrome://resources/cr_components/commerce/shopping_service.mojom-webui.js';
import type {ShoppingServiceBrowserProxy} from 'chrome://resources/cr_components/commerce/shopping_service_browser_proxy.js';
import {ShoppingServiceBrowserProxyImpl} from 'chrome://resources/cr_components/commerce/shopping_service_browser_proxy.js';
import type {CrButtonElement} from 'chrome://resources/cr_elements/cr_button/cr_button.js';
import {CrFeedbackOption} from 'chrome://resources/cr_elements/cr_feedback_buttons/cr_feedback_buttons.js';
import type {CrToastElement} from 'chrome://resources/cr_elements/cr_toast/cr_toast.js';
import {assert} from 'chrome://resources/js/assert.js';
import {EventTracker} from 'chrome://resources/js/event_tracker.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {OpenWindowProxyImpl} from 'chrome://resources/js/open_window_proxy.js';
import type {Uuid} from 'chrome://resources/mojo/mojo/public/mojom/base/uuid.mojom-webui.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './app.html.js';
import type {BuyingOptions} from './buying_options_section.js';
import type {ComparisonTableDetails, ComparisonTableListElement} from './comparison_table_list.js';
import type {ComparisonTableListItemClickEvent} from './comparison_table_list_item.js';
import type {ProductDescription} from './description_section.js';
import type {HeaderElement} from './header.js';
import type {NewColumnSelectorElement} from './new_column_selector.js';
import {SectionType} from './product_selection_menu.js';
import type {ProductSelectorElement} from './product_selector.js';
import {Router} from './router.js';
import type {ProductSpecifications, ProductSpecificationsProduct} from './shopping_service.mojom-webui.js';
import type {TableElement} from './table.js';
import {isValidLowercaseUuid, type UrlListEntry} from './utils.js';
import {WindowProxy} from './window_proxy.js';

interface AggregatedProductData {
  productInfo: ProductInfo|null;
  spec: ProductSpecificationsProduct|null;
}

interface LoadingState {
  loading: boolean;
  urlCount: number;
}

export type Content = string|ProductDescription|BuyingOptions|null;

interface ProductDetail {
  title: string|null;
  content: Content;
}

export interface TableColumn {
  selectedItem: UrlListEntry;
  productDetails: ProductDetail[]|null;
}

export interface ProductSpecificationsElement {
  $: {
    comparisonTableList: ComparisonTableListElement,
    empty: HTMLElement,
    error: HTMLElement,
    errorToast: CrToastElement,
    header: HeaderElement,
    loading: HTMLElement,
    managementContainer: HTMLElement,
    newColumnSelector: NewColumnSelectorElement,
    offlineToast: CrToastElement,
    productSelector: ProductSelectorElement,
    specs: HTMLElement,
    summaryContainer: HTMLElement,
    summaryTable: TableElement,
    syncPromo: HTMLElement,
    turnOnSyncButton: CrButtonElement,
  };
}

// This enum is used for metrics and should be kept in sync with the enum of
// the same name in enums.xml.
export enum CompareTableColumnAction {
  REMOVE = 0,
  CHANGE_ORDER_DRAG_AND_DROP = 1,
  ADD_FROM_SUGGESTED = 2,
  UPDATE_FROM_SUGGESTED = 3,
  ADD_FROM_RECENTLY_VIEWED = 4,
  UPDATE_FROM_RECENTLY_VIEWED = 5,
  // Must be last:
  MAX_VALUE = 6,
}

// This enum is used for metrics and should be kept in sync with the enum of
// the same name in enums.xml.
export enum CompareTableLoadStatus {
  SUCCESS = 0,
  FAILURE = 1,
  FAILURE_EMPTY_TABLE_BACKEND = 2,
  FAILURE_EMPTY_TABLE_NON_PRODUCTS = 2,
  FAILURE_USER_NOT_ELIGIBLE = 3,
  FAILURE_OFFLINE = 4,
  // Must be last:
  MAX_VALUE = 5,
}

export const COLUMN_MODIFICATION_HISTOGRAM_NAME: string =
    'Commerce.Compare.Table.ColumnModification';

export const TABLE_LOAD_HISTOGRAM_NAME: string =
    'Commerce.Compare.Table.LoadStatus';

enum AppState {
  ERROR = 0,
  TABLE_EMPTY = 1,
  SYNC_SCREEN = 2,
  TABLE_POPULATED = 3,
  LOADING = 4,
  NO_CONTENT = 5,
}

function getProductDetails(
    product: ProductSpecificationsProduct|null,
    productSpecs: ProductSpecifications,
    productInfo: ProductInfo|null): ProductDetail[] {
  const productDetails: ProductDetail[] = [];

  // First add rows that don't come directly from the product specifications
  // backend. This includes the current price and buying options URL.
  productDetails.push({
    title: loadTimeData.getString('priceRowTitle'),
    content: {
      price: productInfo?.priceSummary || productInfo?.currentPrice || '',
      jackpotUrl: product?.buyingOptionsUrl.url || '',
    },
  });

  // The second row is the product-level summary.
  productDetails.push({
    title: loadTimeData.getString('productSummaryRowTitle'),
    content: {
      attributes: [],
      summary: product?.summary || [],
    },
  });

  productSpecs.productDimensionMap.forEach((title: string, key: bigint) => {
    if (!product) {
      // Fill in missing product details to ensure uniform table row count.
      productDetails.push({title, content: null});
    } else {
      const value = product.productDimensionValues.get(key);
      const attributes =
          (value?.specificationDescriptions || []).flatMap(description => {
            return {
              label: description.label,
              value: description.options.flatMap(option => option.descriptions)
                         .flatMap(desc => desc.text)
                         .join(', '),
            };
          }) ||
          [];
      const summary = value?.summary || [];
      productDetails.push({
        title,
        content: {
          attributes,
          summary,
        },
      });
    }
  });

  return productDetails;
}

function areStatesEqual(
    firstState: ProductSpecificationsFeatureState,
    secondState: ProductSpecificationsFeatureState) {
  return firstState.isSyncingTabCompare === secondState.isSyncingTabCompare &&
      firstState.canLoadFullPageUi === secondState.canLoadFullPageUi &&
      firstState.canManageSets === secondState.canManageSets &&
      firstState.canFetchData === secondState.canFetchData &&
      firstState.isAllowedForEnterprise === secondState.isAllowedForEnterprise;
}

function findProductInResults(clusterId: bigint, specs: ProductSpecifications):
    ProductSpecificationsProduct|null {
  if (!specs) {
    return null;
  }

  for (const product of specs.products) {
    if (product.productClusterId.toString() === clusterId.toString()) {
      return product;
    }
  }

  return null;
}

// Custom event types for the start and end of the loading animation.
export const LOADING_START_EVENT_TYPE: string = 'loading-animation-start';
export const LOADING_END_EVENT_TYPE: string = 'loading-animation-end';

const LOADING_ANIMATION_SLIDE_PX = 16;
const LOADING_ANIMATION_SLIDE_DURATION_MS = 200;

export class ProductSpecificationsElement extends PolymerElement {
  static get is() {
    return 'product-specifications-app';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      appState_: {
        type: Object,
        computed: 'computeAppState_(productSpecificationsFeatureState_.*,' +
            ' loadingState_.loading, showEmptyState_)',
      },
      comparisonTableDetails_: Array,
      loadingState_: Object,
      setName_: String,
      showComparisonTableList_: {
        type: Boolean,
        computed: 'computeShowComparisonTableList_(showEmptyState_,' +
            ' comparisonTableDetails_)',
      },
      showTableDataUnavailableContainer_: {
        type: Boolean,
        computed: 'computeShowTableDataUnavailableContainer_(appState_)',
        reflectToAttribute: true,
      },
      tableColumns_: Object,
    };
  }

  private appState_: AppState = AppState.NO_CONTENT;
  private comparisonTableDetails_: ComparisonTableDetails[] = [];
  private loadingState_: LoadingState = {loading: false, urlCount: 0};
  private setName_: string|null = null;
  private showComparisonTableList_: boolean = false;
  private showTableDataUnavailableContainer_: boolean;
  private tableColumns_: TableColumn[] = [];

  private callbackRouter_: PageCallbackRouter;
  private eventTracker_: EventTracker = new EventTracker();
  private id_: Uuid|null = null;
  private isWindowFocused_: boolean = true;
  private listenerIds_: number[] = [];
  private minLoadingAnimationMs_: number = 500;
  private pendingSetUpdate_: (() => void)|null = null;
  private productSpecificationsFeatureState_: ProductSpecificationsFeatureState;
  private productSpecificationsProxy_: ProductSpecificationsBrowserProxy =
      ProductSpecificationsBrowserProxyImpl.getInstance();
  private shoppingApi_: ShoppingServiceBrowserProxy =
      ShoppingServiceBrowserProxyImpl.getInstance();
  private showEmptyState_: boolean;

  constructor() {
    super();
    this.callbackRouter_ = this.productSpecificationsProxy_.getCallbackRouter();
    ColorChangeUpdater.forDocument().start();
  }

  override async connectedCallback() {
    super.connectedCallback();

    this.listenerIds_.push(
        this.callbackRouter_.onProductSpecificationsSetAdded.addListener(
            (set: ProductSpecificationsSet) => this.onSetAdded_(set)),
        this.callbackRouter_.onProductSpecificationsSetRemoved.addListener(
            (uuid: Uuid) => this.onSetRemoved_(uuid)),
        this.callbackRouter_.onProductSpecificationsSetUpdated.addListener(
            (set: ProductSpecificationsSet) => this.onSetUpdated_(set)));

    this.addEventListener(
        'comparison-table-list-item-click',
        this.onComparisonTableListItemClickEvent_);

    // TODO: b/358131415 - use listeners to update. Temporary workaround uses
    // window focus to update the feature state, to check signin.
    window.addEventListener('focus', async () => {
      this.isWindowFocused_ = true;

      const previousState = this.productSpecificationsFeatureState_;
      const {state} =
          await this.shoppingApi_.getProductSpecificationsFeatureState();
      if (!state || areStatesEqual(previousState, state)) {
        if (this.pendingSetUpdate_) {
          this.pendingSetUpdate_();
          this.pendingSetUpdate_ = null;
        }
        return;
      }

      // If there is a set update, the new set will be fetched when the table
      // is reloaded.
      this.pendingSetUpdate_ = null;

      // States have changed, so we need to reload the table.
      // Update the featureState after loadTable_(), so that the loading
      // state will animate first.
      await this.loadTable_(state);
      this.productSpecificationsFeatureState_ = state;
    });

    window.addEventListener('blur', () => {
      this.isWindowFocused_ = false;
    });

    this.eventTracker_.add(
        this, 'click',
        () => {
          this.$.offlineToast.hide();
          this.$.errorToast.hide();
        },
        /*useCapture=*/ true);
    this.eventTracker_.add(window, 'online', () => {
      this.$.offlineToast.hide();
    });

    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    // TODO(b/358131415): update after we use listener/ observers and no longer
    // need the featureState
    const {state} =
        await this.shoppingApi_.getProductSpecificationsFeatureState();
    if (!state) {
      return;
    }
    await this.loadTable_(state);
    this.productSpecificationsFeatureState_ = state;
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    this.listenerIds_.forEach(id => this.callbackRouter_.removeListener(id));
    this.eventTracker_.removeAll();
  }

  disableMinLoadingAnimationMsForTesting() {
    this.minLoadingAnimationMs_ = 0;
  }

  focusWindowForTesting() {
    this.isWindowFocused_ = true;
  }

  private async loadTable_(state: ProductSpecificationsFeatureState) {
    // Don't load the table if access conditions are not met.
    if (!(state.isSyncingTabCompare && state.canLoadFullPageUi &&
          state.canFetchData && state.isAllowedForEnterprise)) {
      return;
    }

    const router = Router.getInstance();
    const params = new URLSearchParams(router.getCurrentQuery());
    const idParam = params.get('id');
    if (idParam && isValidLowercaseUuid(idParam)) {
      if (await this.loadSet_({value: idParam})) {
        return;
      }
    }

    const urlsParam = params.get('urls');
    if (!urlsParam) {
      this.updateEmptyState_(true);
      return;
    }

    let urls: string[] = [];
    try {
      urls = JSON.parse(urlsParam);
    } catch (_) {
      return;
    }

    // TODO(b/346601645): Detect if a set already exists
    await this.createNewSet_(urls);
  }

  private computeAppState_() {
    if (this.productSpecificationsFeatureState_) {
      if (!this.productSpecificationsFeatureState_.isSyncingTabCompare) {
        return AppState.SYNC_SCREEN;
      }
      if (!(this.productSpecificationsFeatureState_.canLoadFullPageUi &&
            this.productSpecificationsFeatureState_.canFetchData &&
            this.productSpecificationsFeatureState_.isAllowedForEnterprise)) {
        chrome.metricsPrivate.recordEnumerationValue(
            TABLE_LOAD_HISTOGRAM_NAME,
            CompareTableLoadStatus.FAILURE_USER_NOT_ELIGIBLE,
            CompareTableLoadStatus.MAX_VALUE);
        return AppState.ERROR;
      }
      if (this.loadingState_.loading) {
        return AppState.LOADING;
      }
      if (this.showEmptyState_) {
        return AppState.TABLE_EMPTY;
      }
      return AppState.TABLE_POPULATED;
    }

    if (this.isOffline_) {
      chrome.metricsPrivate.recordEnumerationValue(
          TABLE_LOAD_HISTOGRAM_NAME, CompareTableLoadStatus.FAILURE_OFFLINE,
          CompareTableLoadStatus.MAX_VALUE);
      return AppState.ERROR;
    }

    return AppState.NO_CONTENT;
  }

  private isAppStateError_() {
    return this.appState_ === AppState.ERROR;
  }

  private isAppStateTableEmpty_() {
    return this.appState_ === AppState.TABLE_EMPTY;
  }

  private isAppStateSyncScreen_() {
    return this.appState_ === AppState.SYNC_SCREEN;
  }

  private isAppStateTablePopulated_() {
    return this.appState_ === AppState.TABLE_POPULATED;
  }

  private isAppStateLoading_() {
    return this.appState_ === AppState.LOADING;
  }

  private isAppStateNoContent_() {
    return this.appState_ === AppState.NO_CONTENT;
  }

  private computeShowTableDataUnavailableContainer_() {
    return this.appState_ === AppState.ERROR ||
        this.appState_ === AppState.TABLE_EMPTY ||
        this.appState_ === AppState.SYNC_SCREEN;
  }

  private computeShowComparisonTableList_() {
    if (!loadTimeData.getBoolean('comparisonTableListEnabled')) {
      return false;
    }

    return this.showEmptyState_ && this.id_ === null &&
        this.comparisonTableDetails_.length > 0;
  }

  private canShowFooter_(
      showTableDataUnavailableContainer: boolean, appState: AppState) {
    return !(
        showTableDataUnavailableContainer || appState === AppState.NO_CONTENT);
  }

  private canShowFeedbackButtons_() {
    return Boolean(
        this.productSpecificationsFeatureState_?.isQualityLoggingAllowed);
  }

  private showSyncSetupFlow_() {
    assert(this.productSpecificationsFeatureState_);
    assert(!this.productSpecificationsFeatureState_.isSyncingTabCompare);

    // If user's already signed in at the account level, then user needs to turn
    // on the compare-specific sync from settings.
    if (this.productSpecificationsFeatureState_.isSignedIn) {
      OpenWindowProxyImpl.getInstance().openUrl(
          'chrome://settings/syncSetup/advanced');
      return;
    }
    this.productSpecificationsProxy_.showSyncSetupFlow();
  }

  private showOfflineToast_() {
    this.$.offlineToast.show();
  }

  private async populateTable_(urls: string[]) {
    this.$.errorToast.hide();

    // Transition directly to the empty state if there are no URLs.
    if (urls.length === 0) {
      this.tableColumns_ = [];
      this.updateEmptyState_(true);
      return;
    }

    await this.enterLoadingState_(urls.length);

    const start = Date.now();
    this.updateEmptyState_(false);

    const tableColumns: TableColumn[] = [];
    if (urls.length) {
      const {productSpecs} =
          await this.shoppingApi_.getProductSpecificationsForUrls(
              urls.map(url => ({url})));
      const aggregatedDataByUrl =
          await this.aggregateProductDataByUrl_(urls, productSpecs);


      await Promise.all(urls.map(async (url: string) => {
        const info = aggregatedDataByUrl.get(url)?.productInfo;
        const product = aggregatedDataByUrl.get(url)?.spec;
        const title = product?.title || info?.title ||
            (await this.productSpecificationsProxy_.getPageTitleFromHistory(
                 {url}))
                .title;

        tableColumns.push({
          selectedItem: {
            title,
            url,
            imageUrl: info?.imageUrl?.url || product?.imageUrl?.url || '',
          },
          productDetails:
              getProductDetails(product || null, productSpecs, info || null),
        });
      }));

      // Show an error message if we didn't get back any dimensions. Note that
      // the URLs in the comparison will still be displayed as columns.
      if (productSpecs.productDimensionMap.size === 0 && urls.length > 1) {
        this.$.errorToast.show();
        // If there's no product info for any of the URLs, the table is a
        // collection of non-products.
        if (urls.some((url) => !!aggregatedDataByUrl.get(url))) {
          chrome.metricsPrivate.recordEnumerationValue(
              TABLE_LOAD_HISTOGRAM_NAME,
              CompareTableLoadStatus.FAILURE_EMPTY_TABLE_NON_PRODUCTS,
              CompareTableLoadStatus.MAX_VALUE);
        } else {
          chrome.metricsPrivate.recordEnumerationValue(
              TABLE_LOAD_HISTOGRAM_NAME,
              CompareTableLoadStatus.FAILURE_EMPTY_TABLE_BACKEND,
              CompareTableLoadStatus.MAX_VALUE);
        }
      } else {
        chrome.metricsPrivate.recordEnumerationValue(
            TABLE_LOAD_HISTOGRAM_NAME, CompareTableLoadStatus.SUCCESS,
            CompareTableLoadStatus.MAX_VALUE);
      }
    }

    // Enforce a minimum amount of time in the loading state to avoid it
    // appearing like an unintentional flash.
    const delta = Date.now() - start;
    if (delta < this.minLoadingAnimationMs_ && urls.length > 0) {
      await new Promise(
          res => setTimeout(res, this.minLoadingAnimationMs_ - delta));
    }

    this.tableColumns_ = tableColumns;
    this.updateEmptyState_(this.tableColumns_.length === 0);
    this.exitLoadingState_();
  }

  private get isOffline_(): boolean {
    return !WindowProxy.getInstance().onLine;
  }

  private async getProductInfoForUrls_(urls: string[]):
      Promise<Map<string, ProductInfo>> {
    const infoMap: Map<string, ProductInfo> = new Map();
    for (const url of urls) {
      const {productInfo} = await this.shoppingApi_.getProductInfoForUrl({url});
      if (productInfo && productInfo.clusterId) {
        infoMap.set(url, productInfo);
      }
    }
    return infoMap;
  }

  private async aggregateProductDataByUrl_(
      urls: string[], specs: ProductSpecifications):
      Promise<Map<string, AggregatedProductData>> {
    const urlToProductInfoMap: Map<string, ProductInfo> =
        await this.getProductInfoForUrls_(urls);
    const specProductMap: Map<string, ProductSpecificationsProduct> = new Map();
    urlToProductInfoMap.forEach((value, key) => {
      const product = findProductInResults(value.clusterId, specs);
      if (product) {
        specProductMap.set(key, product);
      }
    });

    const aggregatedDatas: Map<string, AggregatedProductData> = new Map();
    urls.forEach((url) => {
      const productInfo = urlToProductInfoMap.get(url);
      const productSpecs = specProductMap.get(url);
      aggregatedDatas.set(url, {
        productInfo: productInfo ?? null,
        spec: productSpecs ?? null,
      });
    });
    return aggregatedDatas;
  }

  private deleteSet_() {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    if (this.id_) {
      this.shoppingApi_.deleteProductSpecificationsSet(this.id_);
    }
  }

  private updateSetName_(e: CustomEvent<{name: string}>) {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    if (this.id_) {
      this.shoppingApi_.setNameForProductSpecificationsSet(
          this.id_, e.detail.name);
    }
  }

  private async fetchComparisonTableDetails_() {
    const {sets} = await this.shoppingApi_.getAllProductSpecificationsSets();

    if (sets.length === 0 && this.comparisonTableDetails_.length === 0) {
      return;
    }

    this.comparisonTableDetails_ = await Promise.all(
        sets.map(async set => this.createTableDetailsFromSet_(set)));
  }

  private async createTableDetailsFromSet_(set: ProductSpecificationsSet):
      Promise<ComparisonTableDetails> {
    // Find the first product with an image to use as the list item image.
    let imageUrl = null;
    for (let i = 0; i < set.urls.length; i++) {
      const {productInfo} =
          await this.shoppingApi_.getProductInfoForUrl(set.urls[i]);

      if (productInfo.imageUrl.url) {
        imageUrl = productInfo.imageUrl;
        break;
      }
    }

    return {
      name: set.name,
      uuid: set.uuid,
      numUrls: set.urls.length,
      imageUrl,
    };
  }

  private seeAllSets_() {
    OpenWindowProxyImpl.getInstance().openUrl(
        loadTimeData.getString('productSpecificationsManagementUrl'));
  }

  private async onUrlAdd_(
      e: CustomEvent<{url: string, urlSection: SectionType}>) {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    let recordValue = CompareTableColumnAction.MAX_VALUE;
    switch (e.detail.urlSection) {
      case SectionType.RECENT:
        recordValue = CompareTableColumnAction.ADD_FROM_RECENTLY_VIEWED;
        break;
      case SectionType.SUGGESTED:
        recordValue = CompareTableColumnAction.ADD_FROM_SUGGESTED;
        break;
    }
    chrome.metricsPrivate.recordEnumerationValue(
        COLUMN_MODIFICATION_HISTOGRAM_NAME, recordValue,
        CompareTableColumnAction.MAX_VALUE);

    const urls = this.getTableUrls_();
    urls.push(e.detail.url);
    // If there is already a current set, we won't be showing the disclosure and
    // we can modify the set directly; otherwise, user is trying to add a url
    // from empty state, and we'll try to show the disclosure.
    if (this.id_) {
      this.modifyUrls_(urls);
      return;
    }
    const {disclosureShown} =
        await this.productSpecificationsProxy_.maybeShowDisclosure(
            urls.map(url => ({url})), this.setName_ ? this.setName_ : '',
            /* set_id= */ '');
    // If the disclosure is shown, we won't update the current set.
    if (!disclosureShown) {
      this.modifyUrls_(urls);
    }
  }

  private onUrlChange_(
      e: CustomEvent<{url: string, urlSection: SectionType, index: number}>) {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    let recordValue = CompareTableColumnAction.MAX_VALUE;
    switch (e.detail.urlSection) {
      case SectionType.RECENT:
        recordValue = CompareTableColumnAction.UPDATE_FROM_RECENTLY_VIEWED;
        break;
      case SectionType.SUGGESTED:
        recordValue = CompareTableColumnAction.UPDATE_FROM_SUGGESTED;
        break;
    }
    chrome.metricsPrivate.recordEnumerationValue(
        COLUMN_MODIFICATION_HISTOGRAM_NAME, recordValue,
        CompareTableColumnAction.MAX_VALUE);

    const urls = this.getTableUrls_();
    urls[e.detail.index] = e.detail.url;
    this.modifyUrls_(urls);
  }

  private onUrlOrderUpdate_() {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    chrome.metricsPrivate.recordEnumerationValue(
        COLUMN_MODIFICATION_HISTOGRAM_NAME,
        CompareTableColumnAction.CHANGE_ORDER_DRAG_AND_DROP,
        CompareTableColumnAction.MAX_VALUE);

    const urls = this.getTableUrls_();
    this.modifyUrls_(urls);
  }

  private onUrlRemove_(e: CustomEvent<{index: number}>) {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    chrome.metricsPrivate.recordEnumerationValue(
        COLUMN_MODIFICATION_HISTOGRAM_NAME, CompareTableColumnAction.REMOVE,
        CompareTableColumnAction.MAX_VALUE);

    const urls = this.getTableUrls_();
    urls.splice(e.detail.index, 1);
    this.modifyUrls_(urls);
  }

  private modifyUrls_(urls: string[]) {
    if (this.id_) {
      this.shoppingApi_.setUrlsForProductSpecificationsSet(
          this.id_!, urls.map(url => ({url})));
    } else {
      this.createNewSet_(urls);
    }
  }

  private async createNewSet_(urls: string[]) {
    if (this.isOffline_) {
      this.showOfflineToast_();
      return;
    }

    assert(!this.id_ && !this.setName_);
    this.setName_ = loadTimeData.getString('defaultTableTitle');
    const {createdSet} = await this.shoppingApi_.addProductSpecificationsSet(
        this.setName_, urls.map(url => ({url})));
    if (createdSet) {
      this.id_ = createdSet.uuid;
      document.title = this.setName_;
      window.history.replaceState(undefined, '', `?id=${this.id_.value}`);
    }
    this.populateTable_(urls);
  }

  private getTableUrls_(): string[] {
    return this.tableColumns_.map(
        (column: TableColumn) => column.selectedItem.url);
  }

  private isTableFull_(columnCount: number): boolean {
    return columnCount >= loadTimeData.getInteger('maxTableSize');
  }

  private async onSetUpdated_(set: ProductSpecificationsSet) {
    // If the page does not have focus, schedule the update for later in case a
    // newer update is received before the tab is focused. This prevents all
    // updates from triggering at the same time, which may cause a flicker.
    if (!this.isWindowFocused_) {
      this.pendingSetUpdate_ = this.updateSet_.bind(this, set);
      return;
    }

    this.updateSet_(set);
  }

  private async updateSet_(set: ProductSpecificationsSet) {
    if (this.showEmptyState_) {
      const tableIndex = this.comparisonTableDetails_.findIndex(
          table => table.uuid.value === set.uuid.value);
      if (tableIndex !== -1) {
        this.comparisonTableDetails_ = this.comparisonTableDetails_.toSpliced(
            tableIndex, 1, await this.createTableDetailsFromSet_(set));
      }
    }

    if (set.uuid.value !== this.id_?.value) {
      return;
    }
    document.title = set.name;
    this.setName_ = set.name;

    let urlSetChanged = false;
    let orderChanged = false;
    const tableUrls = this.getTableUrls_();

    if (tableUrls.length === set.urls.length) {
      for (const [i, setUrl] of set.urls.entries()) {
        if (setUrl.url !== tableUrls[i]) {
          orderChanged = true;
        }

        if (!tableUrls.includes(setUrl.url)) {
          urlSetChanged = true;
          break;
        }
      }
    } else {
      urlSetChanged = true;
    }

    if (urlSetChanged) {
      this.populateTable_(set.urls.map(url => url.url));
    } else if (orderChanged) {
      const newCols: TableColumn[] = [];

      for (const [_, setUrl] of set.urls.entries()) {
        const existingIndex = tableUrls.indexOf(setUrl.url);
        assert(existingIndex >= 0, 'Did not find column to reorder!');

        newCols.push(this.tableColumns_[existingIndex]);
      }

      this.tableColumns_ = newCols;
    }
  }

  private onSetRemoved_(id: Uuid) {
    if (id.value === this.id_?.value) {
      window.location.replace(window.location.origin);
    }

    if (this.showEmptyState_) {
      this.comparisonTableDetails_ = this.comparisonTableDetails_.filter(
          table => table.uuid.value !== id.value);
    }
  }

  private onFeedbackSelectedOptionChanged_(
      e: CustomEvent<{value: CrFeedbackOption}>) {
    switch (e.detail.value) {
      case CrFeedbackOption.UNSPECIFIED:
        this.shoppingApi_.setProductSpecificationsUserFeedback(
            UserFeedback.kUnspecified);
        return;
      case CrFeedbackOption.THUMBS_UP:
        this.shoppingApi_.setProductSpecificationsUserFeedback(
            UserFeedback.kThumbsUp);
        return;
      case CrFeedbackOption.THUMBS_DOWN:
        this.shoppingApi_.setProductSpecificationsUserFeedback(
            UserFeedback.kThumbsDown);
        return;
    }
  }

  private async onSetAdded_(set: ProductSpecificationsSet) {
    if (this.showEmptyState_) {
      this.comparisonTableDetails_ =
          [await this.createTableDetailsFromSet_(set)].concat(
              this.comparisonTableDetails_);
    }
  }

  private getDisclaimerText_(): string {
    return loadTimeData.getStringF(
        'experimentalFeatureDisclaimer', loadTimeData.getString('userEmail'));
  }

  private async fadeAndSlideOutManagementContainer_() {
    await this.$.managementContainer
        .animate(
            [
              {opacity: 1, transform: 'translateY(0px)'},
              {
                opacity: 0,
                transform: `translateY(-${LOADING_ANIMATION_SLIDE_PX}px)`,
              },
            ],
            {
              duration: LOADING_ANIMATION_SLIDE_DURATION_MS,
              easing: 'ease-out',
              fill: 'forwards',
            })
        .finished;
  }

  private async fadeAndSlideInManagementContainer_() {
    await this.$.managementContainer
        .animate(
            [
              {
                opacity: 0,
                transform: `translateY(${LOADING_ANIMATION_SLIDE_PX}px)`,
              },
              {opacity: 1, transform: 'translateY(0px)'},
            ],
            {
              duration: LOADING_ANIMATION_SLIDE_DURATION_MS,
              easing: 'ease-out',
              fill: 'forwards',
            })
        .finished;
  }

  // Resolves upon updating the loading state.
  private async enterLoadingState_(urlCount: number): Promise<void> {
    if ([
          AppState.ERROR,
          AppState.SYNC_SCREEN,
          AppState.LOADING,
          AppState.NO_CONTENT,
        ].includes(this.appState_)) {
      this.loadingState_ = {loading: true, urlCount};
      this.dispatchLoadingStartEvent_();
      return Promise.resolve();
    }

    return new Promise<void>(async resolve => {
      await this.fadeAndSlideOutManagementContainer_();
      this.loadingState_ = {loading: true, urlCount};
      resolve();
      await this.fadeAndSlideInManagementContainer_();
      this.dispatchLoadingStartEvent_();
    });
  }

  private async exitLoadingState_() {
    await this.fadeAndSlideOutManagementContainer_();
    this.loadingState_ = {loading: false, urlCount: 0};
    await this.fadeAndSlideInManagementContainer_();
    this.dispatchLoadingEndEvent_();
  }

  private dispatchLoadingStartEvent_() {
    this.dispatchEvent(new CustomEvent(
        LOADING_START_EVENT_TYPE, {bubbles: true, composed: true}));
  }

  private dispatchLoadingEndEvent_() {
    this.dispatchEvent(new CustomEvent(
        LOADING_END_EVENT_TYPE, {bubbles: true, composed: true}));
  }

  private updateEmptyState_(shouldShow: boolean) {
    this.showEmptyState_ = shouldShow;

    // If we show the empty state and there are no comparison tables, try to
    // fetch them.
    if (this.showEmptyState_ && this.comparisonTableDetails_.length === 0) {
      this.fetchComparisonTableDetails_();
    }
  }

  private onComparisonTableListItemClickEvent_(
      event: ComparisonTableListItemClickEvent) {
    window.history.replaceState(
        undefined, '', `?id=${event.detail.uuid.value}`);
    this.loadSet_(event.detail.uuid);
  }

  private async loadSet_(uuid: Uuid): Promise<boolean> {
    const {set} =
        await this.shoppingApi_.getProductSpecificationsSetByUuid(uuid);
    if (set) {
      const {disclosureShown} =
          await this.productSpecificationsProxy_.maybeShowDisclosure(
              /* urls= */[], /* name= */ '', uuid.value);
      if (disclosureShown) {
        this.updateEmptyState_(true);
        this.id_ = null;
        return false;
      }
      this.id_ = set.uuid;
      document.title = set.name;
      this.setName_ = set.name;
      this.populateTable_(set.urls.map(url => (url.url)));
      return true;
    }

    this.updateEmptyState_(true);
    this.id_ = null;
    return false;
  }
}

declare global {
  interface HTMLElementEventMap {
    'comparison-table-list-item-click': ComparisonTableListItemClickEvent;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'product-specifications-app': ProductSpecificationsElement;
  }
}

customElements.define(
    ProductSpecificationsElement.is, ProductSpecificationsElement);

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// API between the Chrome browser and the Glic web client.
//
// Overall notes:
// - There will only ever be one single instance of the web client running at
//   a time. It may be destroyed and restarted, and each time the initialization
//   process will be repeated.
// - As in TypeScript all `number`s are 64 bit floating points, we decided to
//   make all identifier values be of the `string` type (e.g. for a window or a
//   tab).
// - The defined functions and interfaces can be "evolved" to provide more
//   functionality and data, as needed.

/**
 * Allows the Glic web client to register with the host WebUI.
 * NOTE: There will be a TBD setup step to be executed by the web client before
 * this API can be used. It will inject needed infrastructure code to
 * enable/implement this API, and provide an instance of GlicHostRegistry to the
 * web client.
 */
export declare interface GlicHostRegistry {
  // Registers the GlicWebClient instance with the browser. On success, the
  // browser will follow up with a call initialize. A rejection of the promise
  // indicates a browser side failure.
  // The web client must call this once when its webview on-load event is fired.
  registerWebClient(webClient: GlicWebClient): Promise<void>;
}

/**
 * Implemented by the Glic web client. Most functions are optional.
 */
export declare interface GlicWebClient {
  // Signals to the web client that it should initialize itself and get ready to
  // be invoked by the user. The provided GlicBrowserHost instance offers
  // information access functions so that the web client can configure itself
  // That instance should also be stored for later usage.
  // Successful initialization should be signaled with a void promise return, in
  // which case the web client is considered ready to be invoked by the user and
  // initialize() will not be called again.
  // A failed promise means initialization failed and it will not be retried.
  // NOTE: Later on, error-status information may be bundled with the failed
  // promise so that the browser may retry calling initialize() in case of
  // retryable errors.
  initialize(glicBrowserHost: GlicBrowserHost): Promise<void>;

  // The user has requested activation of the web client.
  // The dockedToWindowId identifies the browser window to which the
  // panel is docked to. It is undefined if it is floating free.
  // Note: The returned promise is currently not used in the browser.
  notifyPanelOpened?(dockedToWindowId: string|undefined): Promise<void>;

  // The user has closed the web client window. The window may be activated
  // again later.
  // The promise being resolved indicates the web client has stored any needed
  // information and stopped accepting the user's input.
  notifyPanelClosed?(): Promise<void>;
}

/**
 * Provides functionality implemented by the browser to the Glic web client.
 * Most functions are optional.
 */
export declare interface GlicBrowserHost {
  // Returns Chrome's version.
  getChromeVersion():
      Promise<{major: number, minor: number, build: number, patch: number}>;

  // Sets the size of the glic window to the specified dimensions. Returns the
  // resulting width and height of the window.
  resizeWindow(width: number, height: number):
      Promise<{actualWidth: number, actualHeight: number}>;

  // Set the areas of the glic window from which it should be draggable. Returns
  // a promise that resolves when the browser has updated the draggable area.
  setWindowDraggableAreas(areas: DraggableArea[]): Promise<void>;

  // Fetches page context for the currently focused tab, optionally including
  // more expensive-to-generate data. Undefined optional arguments indicate that
  // the respective data is not being requested.
  // If innerText is true, an innerText representation of the page will be
  // included in the response.
  // If viewportScreenshot is true, a screenshot of the user visible viewport
  // will be included in the response.
  // Rejects the promise with a `GetTabContextError` on failure.
  getContextFromFocusedTab?
      (options: {innerText?: boolean, viewportScreenshot?: boolean}):
          Promise<TabContextResult>;

  // Creates a tab and navigates to a url. It is made the active tab by default
  // but that can be changed using the openInBackground option.
  // The tab is created in the currently active window by default. If windowId
  // is specified, it is created within the respective window.
  // The promise returns information about the newly created tab. The promise
  // may be rejected in case of errors.
  // NOTE: This function does not return loading information for the newly
  // created tab. If that's needed, we can add another function that does it.
  createTab?(
      url: string,
      options: {openInBackground?: boolean, windowId?: string},
      ): Promise<TabData>;

  // Requests the closing of the panel containing the web client.
  closePanel?(): Promise<void>;

  // Returns the state of the panel.
  getPanelState?(): Observable<PanelState>;

  // Returns the state of the microphone permission.
  getMicrophonePermissionState?(): Observable<boolean>;

  // Returns the state of the location permission.
  getLocationPermissionState?(): Observable<boolean>;

  // Returns the state of the tab context permission.
  getTabContextPermissionState?(): Observable<boolean>;

  // Set the state of the microphone permission in settings. Returns a promise
  // that resolves when the browser has stored the new pref value.
  setMicrophonePermissionState(enabled: boolean): Promise<void>;

  // Set the state of the location permission in settings. Returns a promise
  // that resolves when the browser has stored the new pref value.
  setLocationPermissionState(enabled: boolean): Promise<void>;

  // Set the state of the tab context permission in settings. Returns a promise
  // that resolves when the browser has stored the new pref value.
  setTabContextPermissionState(enabled: boolean): Promise<void>;

  // Returns the user profile information.
  getUserProfileInfo?(): Promise<UserProfileInfo>;

  // Update Google sign-in cookies for this client. Resolves after the cookies
  // are successfully updated. Rejects if sign-in cookies cannot be updated.
  // This should only be called if the web client detects that it is not
  // signed-in, as Chrome will attempt to refresh cookies automatically in some
  // circumstances. If this is called while a cookie refresh is already in
  // progress, only one cookie refresh will take place.
  refreshSignInCookies?(): Promise<void>;
}

// A panel can be in one of these three states.
export enum PanelStateKind {
  // Not shown. This is the initial state.
  HIDDEN = 0,
  // A floating window.
  FLOATING = 1,
  // Docked to a Chrome window.
  DOCKED = 2,
}

export declare interface PanelState {
  kind: PanelStateKind;
  // Present when docked, indicates to which window the panel is docked.
  windowId?: string;
}

/**
 * Data class holding information and contents extracted from a web page.
 */
export declare interface TabContextResult {
  // Metadata about the tab that holds the page. Always provided.
  tabData: TabData;
  // Information about a web page rendered in the tab at its current state.
  // Provided only if requested.
  webPageData?: WebPageData;
  // A screenshot of the user-visible portion of the page. Provided only if
  // requested.
  viewportScreenshot?: Screenshot;
}

/**
 * Information about a web page being rendered in a tab.
 */
export declare interface WebPageData {
  mainDocument: DocumentData;
}

/**
 * Text information about a web document.
 */
export declare interface DocumentData {
  // Origin of the document.
  origin: string;
  // The innerText of the document at its current state. Currently includes
  // embedded same-origin iframes.
  innerText?: string;
}

/**
 * Various bits of data about a browser tab.
 */
export declare interface TabData {
  // Unique ID of the tab that owns the page.
  tabId: string;
  // Unique ID of the browser window holding the tab.
  windowId: string;
  // URL of the page.
  url: string;
  // The title of the loaded page. Returned if the page is loaded enough for it
  // to be available. It may be empty if the page did not define a title.
  title?: string;
  // The favicon for the tab. Provided when available.
  favicon?(): Promise<Blob|undefined>;
}

/**
 * Annotates an image, providing security relevant information about the origins
 * from which image is composed.
 * Note: This will be updated in the future when we have a solution worked out
 * for annotating the captured screenshots.
 */
export declare interface ImageOriginAnnotations {}

/**
 * An encoded screenshot image and associated metadata.
 * NOTE: Only JPEG images will be supported initially, so mimeType will always
 * be "image/jpeg".
 */
export declare interface Screenshot {
  // Width and height of the image in pixels.
  widthPixels: number;
  heightPixels: number;
  // Encoded image data. ArrayBuffer is transferrable, so it should be copied
  // more efficiently over postMessage.
  data: ArrayBuffer;
  // The image encoding format represented as a MIME type.
  mimeType: string;
  // Image annotations for this screenshot.
  originAnnotations: ImageOriginAnnotations;
}

export declare interface ErrorWithReason<T> extends Error {
  reason: T;
}

// Reason for failure while getting tab context.
export enum GetTabContextErrorReason {
  UNKNOWN = 'UNKNOWN',
  // A valid web contents was not found, or was navigated or closed during
  // context gathering.
  WEB_CONTENTS_CHANGED = 'WEB_CONTENTS_CHANGED',
  // The request was throttled, try again later.
  REQUEST_THROTTLED = 'REQUEST_THROTTLED',
}

export type GetTabContextError = ErrorWithReason<GetTabContextErrorReason>;

/**
 * A rectangular area based in the glic window's coordinate system.
 */
export declare interface DraggableArea {
  x: number;
  y: number;
  width: number;
  height: number;
}

// A value that can be observed.
export declare interface Observable<T> {
  // Get the current value.
  getValue(): T;
  // Listen for changes.
  subscribe(change: (newValue: T) => void): Subscriber;
}

export declare interface Subscriber {
  unsubscribe(): void;
}

export declare interface UserProfileInfo {
  // Returns the avatar icon for the profile, if available.
  avatarIcon(): Promise<Blob|undefined>;
  // The name displayed for this profile.
  displayName: string;
  // The profile email.
  email: string;
}


//
// Types used in the boot process.
//


export declare interface WithGlicApi {
  internalAutoGlicBoot?(chromeSource: WindowProxy): GlicHostRegistry;
}

export declare interface GlicApiBootMessage {
  type: 'glic-bootstrap';
  glicApiSource: string;
}

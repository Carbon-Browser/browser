// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/app_startup_parameters.h"

#import "base/feature_list.h"
#import "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/public/provider/chrome/browser/application_mode_fetcher/application_mode_fetcher_api.h"
#import "net/base/apple/url_conversions.h"
#import "net/base/url_util.h"
#import "url/gurl.h"

@implementation AppStartupParameters {
  GURL _externalURL;
  GURL _completeURL;
  std::vector<GURL> _URLs;
  ApplicationModeRequestStatus _applicationModeRequestStatus;
  NSString* _sourceAppID;

  // The mode in which the tab must be opened. Defaults to UNDETERMINED.
  ApplicationModeForTabOpening _applicationMode;

  // Whether the application mode is forced or not (for example incognito mode
  // or regular mode were forced based on the profile prefs).
  BOOL _forceApplicationMode;

  // An array of blocks to execute once the `applicationMode` is available.
  NSMutableArray<AppModeRequestBlock>* _pendingBlocks;
}

@synthesize inputURLs = _inputURLs;
@synthesize postOpeningAction = _postOpeningAction;
@synthesize textQuery = _textQuery;

- (const GURL&)externalURL {
  return _externalURL;
}

- (const GURL&)completeURL {
  return _completeURL;
}

- (instancetype)initWithExternalURL:(const GURL&)externalURL
                        completeURL:(const GURL&)completeURL
                    applicationMode:(ApplicationModeForTabOpening)mode
               forceApplicationMode:(BOOL)forceApplicationMode {
  self = [super init];
  if (self) {
    _externalURL = externalURL;
    _completeURL = completeURL;
    _applicationMode = mode;
    _applicationModeRequestStatus =
        forceApplicationMode ? ApplicationModeRequestStatus::kAvailable
                             : ApplicationModeRequestStatus::kUnavailable;
    _forceApplicationMode = forceApplicationMode;
  }
  return self;
}

- (instancetype)initWithExternalURL:(const GURL&)externalURL
                        completeURL:(const GURL&)completeURL
                        sourceAppID:(NSString*)sourceAppID
                    applicationMode:(ApplicationModeForTabOpening)mode
               forceApplicationMode:(BOOL)forceApplicationMode {
  self = [super init];
  if (self) {
    _externalURL = externalURL;
    _completeURL = completeURL;
    _sourceAppID = [sourceAppID copy];
    _applicationMode = mode;
    _applicationModeRequestStatus =
        forceApplicationMode ? ApplicationModeRequestStatus::kAvailable
                             : ApplicationModeRequestStatus::kUnavailable;
    _forceApplicationMode = forceApplicationMode;
  }
  return self;
}

- (instancetype)initWithURLs:(const std::vector<GURL>&)URLs
             applicationMode:(ApplicationModeForTabOpening)mode
        forceApplicationMode:(BOOL)forceApplicationMode {
  if (URLs.empty()) {
    self = [self initWithExternalURL:GURL(kChromeUINewTabURL)
                         completeURL:GURL(kChromeUINewTabURL)
                     applicationMode:mode
                forceApplicationMode:forceApplicationMode];
  } else {
    self = [self initWithExternalURL:URLs.front()
                         completeURL:URLs.front()
                     applicationMode:mode
                forceApplicationMode:forceApplicationMode];
  }

  if (self) {
    _URLs = URLs;
  }
  return self;
}

- (NSString*)description {
  NSMutableString* description =
      [NSMutableString stringWithFormat:@"AppStartupParameters: %s",
                                        _externalURL.spec().c_str()];
  if (_applicationMode == ApplicationModeForTabOpening::INCOGNITO) {
    [description appendString:@", should launch in incognito"];
  }

  switch (self.postOpeningAction) {
    case START_QR_CODE_SCANNER:
      [description appendString:@", should launch QR scanner"];
      break;
    case START_LENS_FROM_APP_ICON_LONG_PRESS:
    case START_LENS_FROM_HOME_SCREEN_WIDGET:
    case START_LENS_FROM_SPOTLIGHT:
    case START_LENS_FROM_INTENTS:
      [description appendString:@", should launch Lens"];
      break;
    case START_VOICE_SEARCH:
      [description appendString:@", should launch voice search"];
      break;
    case FOCUS_OMNIBOX:
      [description appendString:@", should focus omnibox"];
      break;
    case OPEN_READING_LIST:
      [description appendString:@", should open reading list"];
      break;
    case OPEN_BOOKMARKS:
      [description appendString:@", should open bookmarks"];
      break;
    case OPEN_RECENT_TABS:
      [description appendString:@", should open recent tabs"];
      break;
    case OPEN_TAB_GRID:
      [description appendString:@", should open tab grid"];
      break;
    case SET_CHROME_DEFAULT_BROWSER:
      [description appendString:@", should open set chrome default browser"];
      break;
    case VIEW_HISTORY:
      [description appendString:@", should open history"];
      break;
    case OPEN_PAYMENT_METHODS:
      [description appendString:@", should open payment methods"];
      break;
    case RUN_SAFETY_CHECK:
      [description appendString:@", should run safety check"];
      break;
    case MANAGE_PASSWORDS:
      [description appendString:@", should open manage passwords setting page"];
      break;
    case MANAGE_SETTINGS:
      [description appendString:@", should open settings page"];
      break;
    case OPEN_LATEST_TAB:
      [description appendString:@", should resume latest tab"];
      break;
    case OPEN_CLEAR_BROWSING_DATA_DIALOG:
      [description appendString:@", should open Clear Browsing Data dialog"];
      break;
    case ADD_BOOKMARKS:
      [description appendString:@", should add bookmarks"];
      break;
    case ADD_READING_LIST_ITEMS:
      [description appendString:@", should add reading list items"];
      break;
    default:
      break;
  }

  return description;
}

- (void)setPostOpeningAction:(TabOpeningPostOpeningAction)action {
  DCHECK([self isValidPostOpeningAction:action]);
  _postOpeningAction = action;
}

#pragma mark - Private methods

- (BOOL)isValidPostOpeningAction:(TabOpeningPostOpeningAction)action {
  switch (action) {
      // Post opening actions that are allowed on any URL.
    case NO_ACTION:
    case SHOW_DEFAULT_BROWSER_SETTINGS:
    case EXTERNAL_ACTION_SHOW_BROWSER_SETTINGS:
    case SEARCH_PASSWORDS:
      return YES;

      // Lens action are valid on empty URLs, in addition to
      // the URLs where all actions are valid.
    case START_LENS_FROM_APP_ICON_LONG_PRESS:
    case START_LENS_FROM_HOME_SCREEN_WIDGET:
    case START_LENS_FROM_SPOTLIGHT:
    case OPEN_LATEST_TAB:
    case START_LENS_FROM_INTENTS:
      if (_externalURL.is_empty()) {
        return YES;
      }
      [[fallthrough]];

      // Other actions are only valid on NTP;
    default:
      return _externalURL == GURL(kChromeUINewTabURL);
  }
}

- (void)requestApplicationModeWithBlock:(AppModeRequestBlock)block {
  switch (_applicationModeRequestStatus) {
    case ApplicationModeRequestStatus::kAvailable:
      CHECK(!_pendingBlocks);
      block(_applicationMode);
      break;
    case ApplicationModeRequestStatus::kRequested:
      CHECK(_pendingBlocks);
      [_pendingBlocks addObject:block];
      break;
    case ApplicationModeRequestStatus::kUnavailable: {
      CHECK(!_pendingBlocks);
      _pendingBlocks = [[NSMutableArray alloc] init];
      [_pendingBlocks addObject:block];
      __weak __typeof(self) weakSelf = self;
      auto callback = base::BindOnce(
          [](AppStartupParameters* startupParams, bool isAppSwitcherIncognito) {
            [startupParams handleApplicationModeRequest:isAppSwitcherIncognito];
          },
          weakSelf);
      ios::provider::FetchApplicationMode(_externalURL, _sourceAppID,
                                          std::move(callback));
      break;
    }
  }
}

- (void)setApplicationMode:(ApplicationModeForTabOpening)applicationMode
      forceApplicationMode:(BOOL)forceApplicationMode {
  if (forceApplicationMode) {
    if (applicationMode == ApplicationModeForTabOpening::INCOGNITO) {
      self.unexpectedMode =
          _applicationMode == ApplicationModeForTabOpening::NORMAL;

    } else if (applicationMode == ApplicationModeForTabOpening::NORMAL) {
      self.unexpectedMode =
          _applicationMode == ApplicationModeForTabOpening::INCOGNITO;
    }
  }
  _applicationMode = applicationMode;
  _forceApplicationMode = forceApplicationMode;
}

- (ApplicationModeForTabOpening)applicationMode {
  CHECK(!base::FeatureList::IsEnabled(kChromeStartupParametersAsync));
  return _applicationMode;
}

- (void)handleApplicationModeRequest:(BOOL)isAppSwitcherIncognito {
  _applicationModeRequestStatus = ApplicationModeRequestStatus::kAvailable;
  if (isAppSwitcherIncognito) {
    _applicationMode = ApplicationModeForTabOpening::APP_SWITCHER_INCOGNITO;
  }

  for (AppModeRequestBlock block in _pendingBlocks) {
    block(_applicationMode);
  }
  _pendingBlocks = nil;
}

@end

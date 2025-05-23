// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTHENTICATION_UI_BUNDLED_ACCOUNT_MENU_ACCOUNT_MENU_MEDIATOR_H_
#define IOS_CHROME_BROWSER_AUTHENTICATION_UI_BUNDLED_ACCOUNT_MENU_ACCOUNT_MENU_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/authentication/ui_bundled/account_menu/account_menu_data_source.h"
#import "ios/chrome/browser/authentication/ui_bundled/account_menu/account_menu_mutator.h"

@protocol AccountMenuConsumer;
@protocol AccountMenuMediatorDelegate;
class AuthenticationService;
class ChromeAccountManagerService;
@protocol ManageSyncSettingsCommandHandler;
class PrefService;
typedef NS_ENUM(NSUInteger, SigninCoordinatorResult);
namespace signin {
class IdentityManager;
}  // namespace signin
namespace syncer {
class SyncService;
}  // namespace syncer
@protocol SystemIdentity;

// Mediator for AccountMenu
@interface AccountMenuMediator
    : NSObject <AccountMenuDataSource, AccountMenuMutator>

// The consumer of the mediator.
@property(nonatomic, weak) id<AccountMenuConsumer> consumer;

// The delegate of the mediator.
@property(nonatomic, weak) id<AccountMenuMediatorDelegate> delegate;

// Parameters for [super
// runCompletionWithSigninResult:completionIdentity];
@property(nonatomic, readwrite) SigninCoordinatorResult signinCoordinatorResult;
// the identity with which the user is newly signed-in.
@property(nonatomic, strong, readonly) id<SystemIdentity>
    signinCompletionIdentity;

- (instancetype)initWithSyncService:(syncer::SyncService*)syncService
              accountManagerService:
                  (ChromeAccountManagerService*)accountManagerService
                        authService:(AuthenticationService*)authService
                    identityManager:(signin::IdentityManager*)identityManager
                              prefs:(PrefService*)prefs
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// Disconnects the mediator.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_AUTHENTICATION_UI_BUNDLED_ACCOUNT_MENU_ACCOUNT_MENU_MEDIATOR_H_

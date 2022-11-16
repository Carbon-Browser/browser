// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/providers/signin/fake_trusted_vault_client_backend.h"

#import <UIKit/UIKit.h>

#import "base/callback.h"
#import "base/check.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using CompletionBlock = TrustedVaultClientBackend::CompletionBlock;

@interface FakeTrustedVaultClientBackendViewController : UIViewController

// Completion to call once the view controller is dismiss.
@property(nonatomic, copy) CompletionBlock completion;

- (instancetype)initWithCompletion:(CompletionBlock)completion
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;

// Simulate user cancelling the reauth dialog.
- (void)simulateUserCancel;

@end

@implementation FakeTrustedVaultClientBackendViewController

- (instancetype)initWithCompletion:(CompletionBlock)completion {
  if ((self = [super initWithNibName:nil bundle:nil])) {
    _completion = completion;
  }
  return self;
}

- (void)simulateUserCancel {
  __weak __typeof(self) weakSelf = self;
  [self.presentingViewController
      dismissViewControllerAnimated:YES
                         completion:^() {
                           if (weakSelf.completion) {
                             weakSelf.completion(NO, nil);
                           }
                         }];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = UIColor.orangeColor;
}

@end

FakeTrustedVaultClientBackend::FakeTrustedVaultClientBackend() = default;

FakeTrustedVaultClientBackend::~FakeTrustedVaultClientBackend() = default;

void FakeTrustedVaultClientBackend::AddObserver(Observer* observer) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::RemoveObserver(Observer* observer) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::FetchKeys(ChromeIdentity* chrome_identity,
                                              KeyFetchedCallback callback) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::MarkLocalKeysAsStale(
    ChromeIdentity* chrome_identity,
    base::OnceClosure callback) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::GetDegradedRecoverabilityStatus(
    ChromeIdentity* chrome_identity,
    base::OnceCallback<void(bool)> callback) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::Reauthentication(
    ChromeIdentity* chrome_identity,
    UIViewController* presenting_view_controller,
    CompletionBlock callback) {
  DCHECK(!view_controller_);
  view_controller_ = [[FakeTrustedVaultClientBackendViewController alloc]
      initWithCompletion:callback];
  [presenting_view_controller presentViewController:view_controller_
                                           animated:YES
                                         completion:nil];
}

void FakeTrustedVaultClientBackend::FixDegradedRecoverability(
    ChromeIdentity* chrome_identity,
    UIViewController* presenting_view_controller,
    CompletionBlock callback) {
  // Do nothing.
}

void FakeTrustedVaultClientBackend::CancelDialog(BOOL animated,
                                                 ProceduralBlock callback) {
  DCHECK(view_controller_);
  [view_controller_.presentingViewController
      dismissViewControllerAnimated:animated
                         completion:callback];
  view_controller_ = nil;
}

void FakeTrustedVaultClientBackend::SimulateUserCancel() {
  DCHECK(view_controller_);
  [view_controller_ simulateUserCancel];
  view_controller_ = nil;
}

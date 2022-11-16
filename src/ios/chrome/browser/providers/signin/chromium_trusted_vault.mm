// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/signin/trusted_vault_api.h"

#import "base/callback.h"
#import "base/notreached.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {
namespace provider {
namespace {

// A null implementation of TrustedVaultClient used for the public builds. It
// fails all method calls (as it should only be called after the user has been
// signed-in which is not supported by public build).
class ChromiumTrustedVaultClientBackend final
    : public TrustedVaultClientBackend {
 public:
  // TrustedVaultClientBackend implementation.
  void AddObserver(Observer* observer) final;
  void RemoveObserver(Observer* observer) final;
  void FetchKeys(ChromeIdentity* chrome_identity,
                 KeyFetchedCallback callback) final;
  void MarkLocalKeysAsStale(ChromeIdentity* chrome_identity,
                            base::OnceClosure callback) final;
  void GetDegradedRecoverabilityStatus(
      ChromeIdentity* chrome_identity,
      base::OnceCallback<void(bool)> callback) final;
  void Reauthentication(ChromeIdentity* chrome_identity,
                        UIViewController* presenting_view_controller,
                        CompletionBlock callback) final;
  void FixDegradedRecoverability(ChromeIdentity* chrome_identity,
                                 UIViewController* presenting_view_controller,
                                 CompletionBlock callback) final;
  void CancelDialog(BOOL animated, ProceduralBlock callback) final;
};

void ChromiumTrustedVaultClientBackend::AddObserver(Observer* observer) {
  // Do nothing.
}

void ChromiumTrustedVaultClientBackend::RemoveObserver(Observer* observer) {
  // Do nothing.
}

void ChromiumTrustedVaultClientBackend::FetchKeys(
    ChromeIdentity* chrome_identity,
    KeyFetchedCallback callback) {
  NOTREACHED();
}

void ChromiumTrustedVaultClientBackend::MarkLocalKeysAsStale(
    ChromeIdentity* chrome_identity,
    base::OnceClosure callback) {
  NOTREACHED();
}

void ChromiumTrustedVaultClientBackend::GetDegradedRecoverabilityStatus(
    ChromeIdentity* chrome_identity,
    base::OnceCallback<void(bool)> callback) {
  NOTREACHED();
}

void ChromiumTrustedVaultClientBackend::Reauthentication(
    ChromeIdentity* chrome_identity,
    UIViewController* presenting_view_controller,
    CompletionBlock callback) {
  NOTREACHED();
}

void ChromiumTrustedVaultClientBackend::FixDegradedRecoverability(
    ChromeIdentity* chrome_identity,
    UIViewController* presenting_view_controller,
    CompletionBlock callback) {
  NOTREACHED();
}

void ChromiumTrustedVaultClientBackend::CancelDialog(BOOL animated,
                                                     ProceduralBlock callback) {
  NOTREACHED();
}

}  // anonymous namespace

std::unique_ptr<TrustedVaultClientBackend> CreateTrustedVaultClientBackend(
    TrustedVaultConfiguration* configuration) {
  return std::make_unique<ChromiumTrustedVaultClientBackend>();
}

}  // namespace provider
}  // namespace ios

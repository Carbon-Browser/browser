// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_LOGIN_AUTH_AUTH_STATUS_CONSUMER_H_
#define ASH_COMPONENTS_LOGIN_AUTH_AUTH_STATUS_CONSUMER_H_

#include "base/component_export.h"
#include "base/observer_list_types.h"

namespace ash {

class AuthFailure;
class UserContext;

// An interface that defines the callbacks for objects that the
// Authenticator class will call to report the success/failure of
// authentication for Chromium OS.
class COMPONENT_EXPORT(ASH_LOGIN_AUTH) AuthStatusConsumer
    : public base::CheckedObserver {
 public:
  ~AuthStatusConsumer() override = default;
  // The current login attempt has ended in failure, with error |error|.
  virtual void OnAuthFailure(const AuthFailure& error) = 0;

  // The current login attempt has succeeded for |user_context|.
  virtual void OnAuthSuccess(const UserContext& user_context) = 0;
  // The current guest login attempt has succeeded.
  virtual void OnOffTheRecordAuthSuccess() {}
  // The same password didn't work both online and offline.
  virtual void OnPasswordChangeDetected(const UserContext& user_context);
  // The cryptohome is encrypted in old format and needs migration.
  virtual void OnOldEncryptionDetected(const UserContext& user_context,
                                       bool has_incomplete_migration);
};

}  // namespace ash

#endif  // ASH_COMPONENTS_LOGIN_AUTH_AUTH_STATUS_CONSUMER_H_

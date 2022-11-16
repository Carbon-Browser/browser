// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_LOGIN_AUTH_PUBLIC_AUTH_FAILURE_H_
#define ASH_COMPONENTS_LOGIN_AUTH_PUBLIC_AUTH_FAILURE_H_

#include <string>

#include "base/check.h"
#include "base/component_export.h"
#include "base/notreached.h"
#include "google_apis/gaia/gaia_auth_consumer.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/net_errors.h"

namespace ash {

class COMPONENT_EXPORT(ASH_LOGIN_AUTH) AuthFailure {
 public:
  // Enum used for UMA. Do NOT reorder or remove entry. Don't forget to
  // update LoginFailureReason enum in enums.xml when adding new entries.
  enum FailureReason {
    NONE = 0,
    COULD_NOT_MOUNT_CRYPTOHOME = 1,
    COULD_NOT_MOUNT_TMPFS = 2,
    COULD_NOT_UNMOUNT_CRYPTOHOME = 3,
    DATA_REMOVAL_FAILED = 4,  // Could not destroy your old data
    LOGIN_TIMED_OUT = 5,
    UNLOCK_FAILED = 6,
    NETWORK_AUTH_FAILED = 7,     // Could not authenticate against Google
    OWNER_REQUIRED = 8,          // Only the device owner can log-in.
    ALLOWLIST_CHECK_FAILED = 9,  // Login attempt blocked by allowlist. This
    // value is synthesized by the ExistingUserController and passed to the
    // login_status_consumer_ in tests only. It is never generated or seen by
    // any of the other authenticator classes.
    TPM_ERROR = 10,                   // Critical TPM error encountered.
    USERNAME_HASH_FAILED = 11,        // Could not get username hash.
    FAILED_TO_INITIALIZE_TOKEN = 12,  // Could not get OAuth2 Token,
    MISSING_CRYPTOHOME = 13,          // cryptohome missing from disk.
    AUTH_DISABLED = 14,               // Authentication disabled for user.
    TPM_UPDATE_REQUIRED = 15,         // TPM firmware update is required.
    UNRECOVERABLE_CRYPTOHOME = 16,    // cryptohome is corrupted.
    NUM_FAILURE_REASONS,              // This has to be the last item.
  };

  explicit AuthFailure(FailureReason reason);

  static AuthFailure FromNetworkAuthFailure(
      const GoogleServiceAuthError& error);

  inline bool operator==(const AuthFailure& b) const {
    if (reason_ != b.reason_) {
      return false;
    }
    if (reason_ == NETWORK_AUTH_FAILED) {
      return error_ == b.error_;
    }
    return true;
  }

  const std::string GetErrorString() const;

  const GoogleServiceAuthError& error() const { return error_; }
  const FailureReason& reason() const { return reason_; }

 private:
  AuthFailure(FailureReason reason, GoogleServiceAuthError error);

  FailureReason reason_;
  GoogleServiceAuthError error_;
};

// Enum used for UMA. Do NOT reorder or remove entry. Don't forget to
// update histograms.xml when adding new entries.
enum SuccessReason {
  OFFLINE_AND_ONLINE = 0,
  OFFLINE_ONLY = 1,
  NUM_SUCCESS_REASONS,  // This has to be the last item.
};

}  // namespace ash

#endif  // ASH_COMPONENTS_LOGIN_AUTH_PUBLIC_AUTH_FAILURE_H_

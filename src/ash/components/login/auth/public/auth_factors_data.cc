// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/components/login/auth/public/auth_factors_data.h"

#include "ash/components/cryptohome/cryptohome_parameters.h"
#include "ash/components/login/auth/public/cryptohome_key_constants.h"
#include "base/check_op.h"

namespace ash {

AuthFactorsData::AuthFactorsData(std::vector<cryptohome::KeyDefinition> keys)
    : keys_(std::move(keys)) {}

AuthFactorsData::AuthFactorsData() = default;
AuthFactorsData::AuthFactorsData(const AuthFactorsData&) = default;
AuthFactorsData::AuthFactorsData(AuthFactorsData&&) = default;
AuthFactorsData::~AuthFactorsData() = default;
AuthFactorsData& AuthFactorsData::operator=(const AuthFactorsData&) = default;

const cryptohome::KeyDefinition* AuthFactorsData::FindOnlinePasswordKey()
    const {
  for (const cryptohome::KeyDefinition& key_def : keys_) {
    if (key_def.label == kCryptohomeGaiaKeyLabel)
      return &key_def;
  }
  for (const cryptohome::KeyDefinition& key_def : keys_) {
    // Check if label starts with prefix and has required type.
    if ((key_def.label.find(kCryptohomeGaiaKeyLegacyLabelPrefix) == 0) &&
        key_def.type == cryptohome::KeyDefinition::TYPE_PASSWORD)
      return &key_def;
  }
  return nullptr;
}

const cryptohome::KeyDefinition* AuthFactorsData::FindKioskKey() const {
  for (const cryptohome::KeyDefinition& key_def : keys_) {
    if (key_def.type == cryptohome::KeyDefinition::TYPE_PUBLIC_MOUNT)
      return &key_def;
  }
  return nullptr;
}

bool AuthFactorsData::HasPasswordKey(const std::string& label) const {
  DCHECK_NE(label, kCryptohomePinLabel);

  for (const cryptohome::KeyDefinition& key_def : keys_) {
    if (key_def.type == cryptohome::KeyDefinition::TYPE_PASSWORD &&
        key_def.label == label)
      return true;
  }
  return false;
}

const cryptohome::KeyDefinition* AuthFactorsData::FindPinKey() const {
  for (const cryptohome::KeyDefinition& key_def : keys_) {
    if (key_def.type == cryptohome::KeyDefinition::TYPE_PASSWORD &&
        key_def.policy.low_entropy_credential) {
      DCHECK_EQ(key_def.label, kCryptohomePinLabel);
      return &key_def;
    }
  }
  return nullptr;
}

}  // namespace ash

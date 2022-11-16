// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_PACKAGE_SIGNED_WEB_BUNDLES_SIGNED_WEB_BUNDLE_ID_H_
#define COMPONENTS_WEB_PACKAGE_SIGNED_WEB_BUNDLES_SIGNED_WEB_BUNDLE_ID_H_

#include "base/strings/string_piece_forward.h"
#include "base/types/expected.h"
#include "components/web_package/signed_web_bundles/ed25519_public_key.h"

namespace web_package {

// This class represents the ID of a signed web bundle. There are currently two
// types of IDs: IDs used for development and testing, and IDs based on an
// Ed25519 public key.
// IDs are base32 encoded (without padding), and then transformed to lowercase.
//
// New instances of this class can only be constructed via the static `Create`
// function, which will validate the format of the given ID. This means that you
// can assume that every instance of this class wraps a correctly formatted ID.
class SignedWebBundleId {
 public:
  enum class Type {
    // This is intended for use during development, where a web bundle might not
    // be signed with a real key and instead uses a fake development-only ID.
    kDevelopment,
    kEd25519PublicKey,
  };

  // Attempts to parse a a signed web bundle ID, and returns an instance of
  // `SignedWebBundleId` if it works. If it doesn't, then the return value
  // contains an error message detailing the issue.
  static base::expected<SignedWebBundleId, std::string> Create(
      base::StringPiece base32_encoded_id);

  SignedWebBundleId(const SignedWebBundleId& other);

  ~SignedWebBundleId();

  Type type() const { return type_; }

  // Returns the Ed25519 public key corresponding to this ID. Will CHECK if
  // type() != kEd25519PublicKey
  Ed25519PublicKey GetEd25519PublicKey() const;

  const std::string& id() const { return encoded_id_; }

  bool operator<(const SignedWebBundleId& other) const {
    return decoded_id_ < other.decoded_id_;
  }

  bool operator==(const SignedWebBundleId& other) const {
    return decoded_id_ == other.decoded_id_;
  }

 private:
  static constexpr size_t kEncodedIdLength = 56;
  static constexpr size_t kDecodedIdLength = 35;

  SignedWebBundleId(Type type,
                    base::StringPiece encoded_id,
                    std::array<uint8_t, kDecodedIdLength> decoded_id);

  Type type_;
  const std::string encoded_id_;
  const std::array<uint8_t, kDecodedIdLength> decoded_id_;
};

}  // namespace web_package

#endif  // COMPONENTS_WEB_PACKAGE_SIGNED_WEB_BUNDLES_SIGNED_WEB_BUNDLE_ID_H_

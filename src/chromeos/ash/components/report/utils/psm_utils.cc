// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/ash/components/report/utils/psm_utils.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/hmac.h"
#include "third_party/private_membership/src/private_membership_rlwe.pb.h"

namespace psm_rlwe = private_membership::rlwe;

namespace ash::report::utils {

namespace {

std::string GetDigestString(const std::string& key,
                            const std::string& message) {
  return base::HexEncode(crypto::hmac::SignSha256(base::as_byte_span(key),
                                                  base::as_byte_span(message)));
}

}  // namespace

std::optional<psm_rlwe::RlwePlaintextId> GeneratePsmIdentifier(
    const std::string& high_entropy_seed,
    const std::string& psm_use_case_str,
    const std::string& window_id) {
  if (high_entropy_seed.empty() || psm_use_case_str.empty() ||
      window_id.empty()) {
    LOG(ERROR)
        << "Failed to generate PSM id without the high entropy seed, use "
        << "case, and window id being defined.";
    return std::nullopt;
  }

  std::string unhashed_psm_id =
      base::JoinString({psm_use_case_str, window_id}, "|");

  psm_rlwe::RlwePlaintextId psm_rlwe_id;
  psm_rlwe_id.set_sensitive_id(
      GetDigestString(high_entropy_seed, unhashed_psm_id));
  return psm_rlwe_id;
}

}  // namespace ash::report::utils

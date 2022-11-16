// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_NETWORK_NETWORK_NAME_UTIL_H_
#define CHROMEOS_ASH_COMPONENTS_NETWORK_NETWORK_NAME_UTIL_H_

#include <string>

#include "base/component_export.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chromeos/ash/components/network/cellular_esim_profile_handler.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chromeos/ash/components/network/network_state.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ash::network_name_util {

// Returns eSIM profile name for  a given |network_state|.
// Returns null if |cellular_esim_profile_handler| is null, or network is not
// an eSIM network.
COMPONENT_EXPORT(CHROMEOS_NETWORK)
absl::optional<std::string> GetESimProfileName(
    CellularESimProfileHandler* cellular_esim_profile_handler,
    const NetworkState* network_state);

// Returns network name for a given |network_state|. If network
// is eSIM it calls GetESimProfileName and uses |cellular_esim_profile_handler|
// to get the eSIM profile name. If |cellular_esim_profile_handler| is null,
// this function returns |network_state->name|.
COMPONENT_EXPORT(CHROMEOS_NETWORK)
std::string GetNetworkName(
    CellularESimProfileHandler* cellular_esim_profile_handler,
    const NetworkState* network_state);

}  // namespace ash::network_name_util

// TODO(https://crbug.com/1164001): remove when the migration is finished.
namespace chromeos::network_name_util {
using ::ash::network_name_util::GetESimProfileName;
using ::ash::network_name_util::GetNetworkName;
}  // namespace chromeos::network_name_util

#endif  // CHROMEOS_ASH_COMPONENTS_NETWORK_NETWORK_NAME_UTIL_H_

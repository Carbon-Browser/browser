// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_DBUS_AUDIO_AUDIO_NODE_H_
#define CHROMEOS_ASH_COMPONENTS_DBUS_AUDIO_AUDIO_NODE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/component_export.h"

namespace ash {

// Structure to hold AudioNode data received from cras.
struct COMPONENT_EXPORT(DBUS_AUDIO) AudioNode {
  bool is_input = false;
  uint64_t id = 0;
  bool has_v2_stable_device_id = false;
  uint64_t stable_device_id_v1 = 0;
  uint64_t stable_device_id_v2 = 0;
  std::string device_name;
  std::string type;
  std::string name;
  bool active = false;
  // Time that the node was plugged in.
  uint64_t plugged_time = 0;
  // Max supported channel count of the device for the node.
  uint32_t max_supported_channels = 0;
  // Bit-wise audio effect support information.
  uint32_t audio_effect = 0;

  AudioNode();
  AudioNode(bool is_input,
            uint64_t id,
            bool has_v2_stable_device_id,
            uint64_t stable_device_id_v1,
            uint64_t stable_device_id_v2,
            std::string device_name,
            std::string type,
            std::string name,
            bool active,
            uint64_t plugged_time,
            uint32_t max_supported_channels,
            uint32_t audio_effect);
  AudioNode(const AudioNode& other);
  ~AudioNode();

  std::string ToString() const;
  int StableDeviceIdVersion() const;
  uint64_t StableDeviceId() const;
};

typedef std::vector<AudioNode> AudioNodeList;

}  // namespace ash

// TODO(https://crbug.com/1164001): remove when the migration is finished.
namespace chromeos {
using ::ash::AudioNode;
}

#endif  // CHROMEOS_ASH_COMPONENTS_DBUS_AUDIO_AUDIO_NODE_H_

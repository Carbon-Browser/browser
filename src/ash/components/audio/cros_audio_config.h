// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_AUDIO_CROS_AUDIO_CONFIG_H_
#define ASH_COMPONENTS_AUDIO_CROS_AUDIO_CONFIG_H_

#include "ash/components/audio/public/mojom/cros_audio_config.mojom.h"
#include "ash/constants/ash_features.h"
#include "base/component_export.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ash::audio_config {

// Implements the CrosAudioConfig API, which will support Audio system UI on
// Chrome OS, providing and allowing configuration of system audio settings.
class COMPONENT_EXPORT(ASH_COMPONENTS_AUDIO) CrosAudioConfig
    : public mojom::CrosAudioConfig {
 public:
  CrosAudioConfig(const CrosAudioConfig&) = delete;
  CrosAudioConfig& operator=(const CrosAudioConfig&) = delete;
  ~CrosAudioConfig() override;

  void BindPendingReceiver(
      mojo::PendingReceiver<mojom::CrosAudioConfig> pending_receiver);

 protected:
  CrosAudioConfig();

  void NotifyObserversAudioSystemPropertiesChanged();

  virtual uint8_t GetOutputVolumePercent() const = 0;
  virtual mojom::MuteState GetOutputMuteState() const = 0;

 private:
  // mojom::CrosAudioConfig:
  void ObserveAudioSystemProperties(
      mojo::PendingRemote<mojom::AudioSystemPropertiesObserver> observer)
      override;

  mojom::AudioSystemPropertiesPtr GetAudioSystemProperties();

  mojo::ReceiverSet<mojom::CrosAudioConfig> receivers_;
  mojo::RemoteSet<mojom::AudioSystemPropertiesObserver> observers_;
};

}  // namespace ash::audio_config

#endif  // ASH_COMPONENTS_AUDIO_CROS_AUDIO_CONFIG_H_

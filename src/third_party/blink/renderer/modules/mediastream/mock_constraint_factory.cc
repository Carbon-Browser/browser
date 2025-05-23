// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediastream/mock_constraint_factory.h"

#include <stddef.h>

#include "third_party/blink/renderer/platform/mediastream/media_stream_audio_processor_options.h"

namespace blink {

MockConstraintFactory::MockConstraintFactory() {}

MockConstraintFactory::~MockConstraintFactory() {}

MediaTrackConstraintSetPlatform& MockConstraintFactory::AddAdvanced() {
  advanced_.emplace_back();
  return advanced_.back();
}

MediaConstraints MockConstraintFactory::CreateMediaConstraints() const {
  MediaConstraints constraints;
  constraints.Initialize(basic_, advanced_);
  return constraints;
}

void MockConstraintFactory::DisableDefaultAudioConstraints() {
  basic_.echo_cancellation.SetExact(false);
  basic_.auto_gain_control.SetExact(false);
  basic_.noise_suppression.SetExact(false);
  basic_.voice_isolation.SetExact(false);
}

void MockConstraintFactory::DisableAecAudioConstraints() {
  basic_.echo_cancellation.SetExact(false);
}

void MockConstraintFactory::Reset() {
  basic_ = MediaTrackConstraintSetPlatform();
  advanced_.clear();
}

}  // namespace blink

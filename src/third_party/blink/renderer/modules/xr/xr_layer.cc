// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_layer.h"

#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/xr/xr_layer_shared_image_manager.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"

namespace blink {

XRLayer::XRLayer(XRSession* session)
    : session_(session), layer_id_(session->GetNextLayerId()) {}

ExecutionContext* XRLayer::GetExecutionContext() const {
  return session_->GetExecutionContext();
}

const AtomicString& XRLayer::InterfaceName() const {
  return event_target_names::kXRLayer;
}

const XRLayerSharedImages& XRLayer::GetSharedImages() const {
  return session_->LayerSharedImageManager().GetLayerSharedImages(this);
}

void XRLayer::Trace(Visitor* visitor) const {
  visitor->Trace(session_);
  EventTarget::Trace(visitor);
}

}  // namespace blink

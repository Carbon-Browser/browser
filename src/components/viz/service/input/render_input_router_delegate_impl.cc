// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/input/render_input_router_delegate_impl.h"

#include <utility>

#include "base/notimplemented.h"
#include "components/input/render_widget_host_input_event_router.h"
#include "components/viz/service/input/peak_gpu_memory_tracker_impl.h"
#include "ui/latency/latency_info.h"

namespace viz {

namespace {
bool IsInputEventContinuous(const blink::WebInputEvent& event) {
  using Type = blink::mojom::EventType;
  return (event.GetType() == Type::kTouchMove ||
          event.GetType() == Type::kGestureScrollUpdate ||
          event.GetType() == Type::kGesturePinchUpdate);
}

}  // namespace

RenderInputRouterDelegateImpl::RenderInputRouterDelegateImpl(
    scoped_refptr<input::RenderWidgetHostInputEventRouter> rwhier,
    Delegate& delegate,
    const FrameSinkId& frame_sink_id,
    const base::UnguessableToken& grouping_id)
    : rwhier_(std::move(rwhier)),
      delegate_(delegate),
      frame_sink_id_(frame_sink_id),
      grouping_id_(grouping_id) {
  TRACE_EVENT_INSTANT(
      "input", "RenderInputRouterDelegateImpl::RenderInputRouterDelegateImpl",
      "frame_sink_id", frame_sink_id);
  CHECK(rwhier_);
}

RenderInputRouterDelegateImpl::~RenderInputRouterDelegateImpl() {
  TRACE_EVENT_INSTANT(
      "input", "RenderInputRouterDelegateImpl::~RenderInputRouterDelegateImpl",
      "frame_sink_id", frame_sink_id_);
}

input::RenderWidgetHostViewInput*
RenderInputRouterDelegateImpl::GetPointerLockView() {
  // This is required when we are doing targeting for mouse/mousewheel events.
  // Mouse events are not being handled on Viz with current scope of
  // InputVizard.
  NOTREACHED();
}

std::optional<bool> RenderInputRouterDelegateImpl::IsDelegatedInkHovering() {
  return delegate_->IsDelegatedInkHovering(frame_sink_id_);
}

std::unique_ptr<input::RenderInputRouterIterator>
RenderInputRouterDelegateImpl::GetEmbeddedRenderInputRouters() {
  return delegate_->GetEmbeddedRenderInputRouters(frame_sink_id_);
}

input::RenderWidgetHostInputEventRouter*
RenderInputRouterDelegateImpl::GetInputEventRouter() {
  return rwhier_.get();
}

bool RenderInputRouterDelegateImpl::IsIgnoringWebInputEvents(
    const blink::WebInputEvent& event) const {
  // TODO(b/365541296): Implement RenderInputRouterDelegate interface in Viz.
  NOTIMPLEMENTED();
  return false;
}

bool RenderInputRouterDelegateImpl::PreHandleGestureEvent(
    const blink::WebGestureEvent& event) {
  return false;
}

void RenderInputRouterDelegateImpl::NotifyObserversOfInputEvent(
    const blink::WebInputEvent& event) {
  if (IsInputEventContinuous(event)) {
    return;
  }
  auto web_coalesced_event =
      std::make_unique<blink::WebCoalescedInputEvent>(event, ui::LatencyInfo());

  delegate_->NotifyObserversOfInputEvent(frame_sink_id_, grouping_id_,
                                         std::move(web_coalesced_event));
}

void RenderInputRouterDelegateImpl::NotifyObserversOfInputEventAcks(
    blink::mojom::InputEventResultSource ack_source,
    blink::mojom::InputEventResultState ack_result,
    const blink::WebInputEvent& event) {
  if (IsInputEventContinuous(event)) {
    return;
  }
  auto web_coalesced_event =
      std::make_unique<blink::WebCoalescedInputEvent>(event, ui::LatencyInfo());

  delegate_->NotifyObserversOfInputEventAcks(frame_sink_id_, grouping_id_,
                                             ack_source, ack_result,
                                             std::move(web_coalesced_event));
}

bool RenderInputRouterDelegateImpl::IsInitializedAndNotDead() {
  // Since this is being checked in Viz, Renderer process should already have
  // been initialized. When the renderer process dies, |this| will be deleted
  // and sending input to renderer is stopped at InputManager level.
  return true;
}

input::TouchEmulator* RenderInputRouterDelegateImpl::GetTouchEmulator(
    bool create_if_necessary) {
  // Touch emulation is handled solely on browser.
  return nullptr;
}

void RenderInputRouterDelegateImpl::OnInvalidInputEventSource() {
  delegate_->OnInvalidInputEventSource(frame_sink_id_, grouping_id_);
}

std::unique_ptr<PeakGpuMemoryTracker>
RenderInputRouterDelegateImpl::MakePeakGpuMemoryTracker(
    PeakGpuMemoryTracker::Usage usage) {
  return std::make_unique<PeakGpuMemoryTrackerImpl>(usage,
                                                    delegate_->GetGpuService());
}

}  // namespace viz

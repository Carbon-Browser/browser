// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPUTE_PRESSURE_PRESSURE_SERVICE_IMPL_H_
#define CONTENT_BROWSER_COMPUTE_PRESSURE_PRESSURE_SERVICE_IMPL_H_

#include "base/callback.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "content/browser/compute_pressure/pressure_quantizer.h"
#include "content/common/content_export.h"
#include "content/public/browser/document_user_data.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/device/public/mojom/pressure_manager.mojom.h"
#include "services/device/public/mojom/pressure_state.mojom.h"
#include "third_party/blink/public/mojom/compute_pressure/pressure_service.mojom.h"

namespace content {

class RenderFrameHost;

// Serves all the Compute Pressure API mojo requests for a frame.
// RenderFrameHostImpl owns an instance of this class.
//
// This class is not thread-safe, so each instance must be used on one sequence.
class CONTENT_EXPORT PressureServiceImpl
    : public blink::mojom::PressureService,
      public device::mojom::PressureClient,
      public DocumentUserData<PressureServiceImpl> {
 public:
  static constexpr base::TimeDelta kDefaultVisibleObserverRateLimit =
      base::Seconds(1);

  static void Create(
      RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<blink::mojom::PressureService> receiver);

  ~PressureServiceImpl() override;

  PressureServiceImpl(const PressureServiceImpl&) = delete;
  PressureServiceImpl& operator=(const PressureServiceImpl&) = delete;

  void BindReceiver(
      mojo::PendingReceiver<blink::mojom::PressureService> receiver);

  // blink::mojom::PressureService implementation.
  void AddObserver(mojo::PendingRemote<blink::mojom::PressureObserver> observer,
                   blink::mojom::PressureQuantizationPtr quantization,
                   AddObserverCallback callback) override;

  // device::mojom::PressureClient implementation.
  void PressureStateChanged(device::mojom::PressureStatePtr state,
                            base::Time timestamp) override;

 private:
  friend class content::DocumentUserData<PressureServiceImpl>;

  PressureServiceImpl(RenderFrameHost* render_frame_host,
                      base::TimeDelta visible_observer_rate_limit);

  void OnObserverRemoteDisconnected(mojo::RemoteSetElementId /*id*/);

  void OnManagerRemoteDisconnected();

  void DidAddObserver(
      mojo::PendingRemote<blink::mojom::PressureObserver> observer,
      AddObserverCallback callback,
      bool success);

  // Resets the state used to dispatch updates to observers.
  //
  // Called when the quantizing scheme changes.
  void ResetObserverState();

  SEQUENCE_CHECKER(sequence_checker_);

  // The minimum delay between two Update() calls for observers belonging to
  // the frame.
  const base::TimeDelta visible_observer_rate_limit_;

  // Implements the quantizing scheme used for all the frame's observers.
  PressureQuantizer quantizer_ GUARDED_BY_CONTEXT(sequence_checker_);

  // The (quantized) sample that was last reported to this frame's observers.
  //
  // Stored to avoid sending updates when the underlying compute pressure state
  // changes, but quantization produces the same values that were reported in
  // the last update.
  device::mojom::PressureState last_reported_state_
      GUARDED_BY_CONTEXT(sequence_checker_);

  // The last time the frame's observers received an update.
  //
  // Stored to implement rate-limiting.
  base::Time last_reported_timestamp_ GUARDED_BY_CONTEXT(sequence_checker_);

  mojo::RemoteSet<blink::mojom::PressureObserver> observers_
      GUARDED_BY_CONTEXT(sequence_checker_);
  mojo::ReceiverSet<blink::mojom::PressureService> receivers_
      GUARDED_BY_CONTEXT(sequence_checker_);

  mojo::Remote<device::mojom::PressureManager> remote_
      GUARDED_BY_CONTEXT(sequence_checker_);
  mojo::Receiver<device::mojom::PressureClient> GUARDED_BY_CONTEXT(
      sequence_checker_) client_{this};

  friend DocumentUserData;
  DOCUMENT_USER_DATA_KEY_DECL();
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPUTE_PRESSURE_PRESSURE_SERVICE_IMPL_H_

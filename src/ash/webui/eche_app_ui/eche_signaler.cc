// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/webui/eche_app_ui/eche_signaler.h"

#include "ash/components/multidevice/logging/logging.h"
#include "ash/webui/eche_app_ui/mojom/eche_app.mojom.h"
#include "ash/webui/eche_app_ui/proto/exo_messages.pb.h"
#include "base/metrics/histogram_functions.h"

namespace ash {

namespace {

using ConnectionStatus = secure_channel::ConnectionManager::Status;
using eche_app::mojom::ConnectionFailReason;

// From google3: typescript/webrtc/webrtc_peer_connection.ts
constexpr base::TimeDelta kSignalingTimeoutDuration = base::Milliseconds(10000);

}  // namespace

namespace eche_app {

EcheSignaler::EcheSignaler(
    EcheConnector* eche_connector,
    secure_channel::ConnectionManager* connection_manager)
    : eche_connector_(eche_connector), connection_manager_(connection_manager) {
  connection_manager_->AddObserver(this);
}

EcheSignaler::~EcheSignaler() {
  connection_manager_->RemoveObserver(this);
  signaling_timeout_timer_.reset();
}

void EcheSignaler::SendSignalingMessage(const std::vector<uint8_t>& signal) {
  PA_LOG(INFO) << "echeapi EcheSignaler SendSignalingMessage";
  std::string encoded_signal(signal.begin(), signal.end());
  proto::SignalingRequest request;
  request.set_data(encoded_signal);
  proto::ExoMessage message;
  *message.mutable_request() = std::move(request);
  eche_connector_->SendMessage(message);

  if (!signaling_timeout_timer_) {
    signaling_timeout_timer_ = std::make_unique<base::DelayTimer>(
        FROM_HERE, kSignalingTimeoutDuration, this,
        &EcheSignaler::RecordSignalingTimeout);
    signaling_timeout_timer_->Reset();
  }
}

void EcheSignaler::SetSignalingMessageObserver(
    mojo::PendingRemote<mojom::SignalingMessageObserver> observer) {
  PA_LOG(INFO) << "echeapi EcheSignaler SetSignalingMessageObserver";
  observer_.reset();
  observer_.Bind(std::move(observer));
  signaling_timeout_timer_.reset();
  probably_connection_failed_reason_ = ConnectionFailReason::kUnknown;
}

void EcheSignaler::TearDownSignaling() {
  PA_LOG(INFO) << "echeapi EcheSignaler TearDownSignaling";
  proto::SignalingAction action;
  action.set_action_type(proto::ActionType::ACTION_TEAR_DOWN);
  proto::ExoMessage message;
  *message.mutable_action() = std::move(action);
  eche_connector_->SendMessage(message);
  signaling_timeout_timer_.reset();
  probably_connection_failed_reason_ = ConnectionFailReason::kUnknown;
}

void EcheSignaler::Bind(
    mojo::PendingReceiver<mojom::SignalingMessageExchanger> receiver) {
  exchanger_.reset();
  exchanger_.Bind(std::move(receiver));
}

void EcheSignaler::OnMessageReceived(const std::string& payload) {
  if (!observer_.is_bound())
    return;

  proto::ExoMessage message;
  message.ParseFromString(payload);
  std::string signal;
  if (message.has_request()) {
    PA_LOG(INFO) << "echeapi EcheSignaler OnMessageReceived has request";
    signal = message.request().data();
    probably_connection_failed_reason_ =
        ConnectionFailReason::SignalingHasLateRequest;
  } else if (message.has_response()) {
    PA_LOG(INFO) << "echeapi EcheSignaler OnMessageReceived has response";
    signal = message.response().data();
    probably_connection_failed_reason_ =
        ConnectionFailReason::SignalingHasLateResponse;
  } else {
    PA_LOG(INFO) << "echeapi EcheSignaler OnMessageReceived return";
    return;
  }
  PA_LOG(INFO) << "echeapi EcheSignaler OnMessageReceived";
  std::vector<uint8_t> encoded_signal(signal.begin(), signal.end());
  observer_->OnReceivedSignalingMessage(encoded_signal);
}

void EcheSignaler::RecordSignalingTimeout() {
  if (connection_manager_->GetStatus() == ConnectionStatus::kDisconnected) {
    probably_connection_failed_reason_ =
        ConnectionFailReason::kSecurityChannelDisconnected;
  } else if (probably_connection_failed_reason_ ==
             ConnectionFailReason::kUnknown) {
    probably_connection_failed_reason_ =
        ConnectionFailReason::kSignalingNotTriggered;
  }

  PA_LOG(INFO) << "echeapi EcheSignaler timeout: "
               << probably_connection_failed_reason_;
  base::UmaHistogramEnumeration("Eche.StreamEvent.ConnectionFail",
                                probably_connection_failed_reason_);
}

}  // namespace eche_app
}  // namespace ash

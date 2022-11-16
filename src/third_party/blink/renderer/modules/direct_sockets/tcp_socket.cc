// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/direct_sockets/tcp_socket.h"

#include "base/barrier_callback.h"
#include "base/functional/identity.h"
#include "base/metrics/histogram_functions.h"
#include "net/base/net_errors.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_tcp_socket_open_info.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_tcp_socket_options.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/streams/readable_stream.h"
#include "third_party/blink/renderer/core/streams/writable_stream.h"
#include "third_party/blink/renderer/modules/direct_sockets/direct_sockets_service_mojo_remote.h"
#include "third_party/blink/renderer/modules/direct_sockets/stream_wrapper.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

constexpr char kTCPNetworkFailuresHistogramName[] =
    "DirectSockets.TCPNetworkFailures";

bool CheckSendReceiveBufferSize(const TCPSocketOptions* options,
                                ExceptionState& exception_state) {
  if (options->hasSendBufferSize() && options->sendBufferSize() == 0) {
    exception_state.ThrowTypeError("sendBufferSize must be greater than zero.");
    return false;
  }
  if (options->hasReceiveBufferSize() && options->receiveBufferSize() == 0) {
    exception_state.ThrowTypeError(
        "receiverBufferSize must be greater than zero.");
    return false;
  }
  return true;
}

mojom::blink::DirectSocketOptionsPtr CreateTCPSocketOptions(
    const String& remote_address,
    const uint16_t remote_port,
    const TCPSocketOptions* options,
    ExceptionState& exception_state) {
  auto socket_options = mojom::blink::DirectSocketOptions::New();

  socket_options->remote_hostname = remote_address;
  socket_options->remote_port = remote_port;

  const bool has_full_local_address =
      options->hasLocalAddress() && options->hasLocalPort();

  if (const bool has_partial_local_address =
          options->hasLocalAddress() || options->hasLocalPort();
      has_partial_local_address && !has_full_local_address) {
    exception_state.ThrowTypeError("Incomplete local address specified.");
    return {};
  }

  if (!CheckSendReceiveBufferSize(options, exception_state)) {
    return {};
  }

  if (options->hasKeepAliveDelay() &&
      base::Milliseconds(options->keepAliveDelay()) < base::Seconds(1)) {
    exception_state.ThrowTypeError(
        "keepAliveDelay must be no less than 1,000 milliseconds.");
    return {};
  }

  // noDelay has a default value specified, therefore it's safe to call
  // ->noDelay() without checking ->hasNoDelay() first.
  socket_options->no_delay = options->noDelay();

  socket_options->keep_alive_options =
      network::mojom::blink::TCPKeepAliveOptions::New(
          /*enable=*/options->hasKeepAliveDelay() ? true : false,
          /*delay=*/options->hasKeepAliveDelay()
              ? base::Milliseconds(options->keepAliveDelay()).InSeconds()
              : 0);

  if (has_full_local_address) {
    socket_options->local_hostname = options->localAddress();
    socket_options->local_port = options->localPort();
  }

  if (options->hasSendBufferSize()) {
    socket_options->send_buffer_size = options->sendBufferSize();
  }
  if (options->hasReceiveBufferSize()) {
    socket_options->receive_buffer_size = options->receiveBufferSize();
  }

  return socket_options;
}

}  // namespace

// static
TCPSocket* TCPSocket::Create(ScriptState* script_state,
                             const String& remoteAddress,
                             const uint16_t remotePort,
                             const TCPSocketOptions* options,
                             ExceptionState& exception_state) {
  if (!Socket::CheckContextAndPermissions(script_state, exception_state)) {
    return nullptr;
  }

  auto* socket = MakeGarbageCollected<TCPSocket>(script_state);
  if (!socket->Open(remoteAddress, remotePort, options, exception_state)) {
    return nullptr;
  }
  return socket;
}

TCPSocket::TCPSocket(ScriptState* script_state)
    : Socket(script_state),
      tcp_socket_{GetExecutionContext()},
      socket_observer_{this, GetExecutionContext()} {}

TCPSocket::~TCPSocket() = default;

bool TCPSocket::Open(const String& remote_address,
                     const uint16_t remote_port,
                     const TCPSocketOptions* options,
                     ExceptionState& exception_state) {
  auto open_tcp_socket_options = CreateTCPSocketOptions(
      remote_address, remote_port, options, exception_state);

  if (exception_state.HadException()) {
    return false;
  }

  ConnectService();

  service_->get()->OpenTcpSocket(
      std::move(open_tcp_socket_options), GetTCPSocketReceiver(),
      GetTCPSocketObserver(),
      WTF::Bind(&TCPSocket::Init, WrapPersistent(this)));

  return true;
}

void TCPSocket::Init(int32_t result,
                     const absl::optional<net::IPEndPoint>& local_addr,
                     const absl::optional<net::IPEndPoint>& peer_addr,
                     mojo::ScopedDataPipeConsumerHandle receive_stream,
                     mojo::ScopedDataPipeProducerHandle send_stream) {
  if (result == net::OK && peer_addr) {
    auto close_callback = base::BarrierCallback<bool>(
        /*num_callbacks=*/2,
        WTF::Bind(&TCPSocket::OnBothStreamsClosed, WrapWeakPersistent(this)));

    readable_stream_wrapper_ = MakeGarbageCollected<TCPReadableStreamWrapper>(
        script_state_, close_callback, std::move(receive_stream));
    writable_stream_wrapper_ = MakeGarbageCollected<TCPWritableStreamWrapper>(
        script_state_, close_callback, std::move(send_stream));

    auto* open_info = TCPSocketOpenInfo::Create();

    open_info->setReadable(readable_stream_wrapper_->Readable());
    open_info->setWritable(writable_stream_wrapper_->Writable());

    open_info->setRemoteAddress(String{peer_addr->ToStringWithoutPort()});
    open_info->setRemotePort(peer_addr->port());

    open_info->setLocalAddress(String{local_addr->ToStringWithoutPort()});
    open_info->setLocalPort(local_addr->port());

    opened_resolver_->Resolve(open_info);
  } else {
    if (result != net::OK) {
      // Error codes are negative.
      base::UmaHistogramSparse(kTCPNetworkFailuresHistogramName, -result);
    }
    opened_resolver_->Reject(CreateDOMExceptionFromNetErrorCode(result));
    CloseServiceAndResetFeatureHandle();

    closed_resolver_->Reject();
  }

  opened_resolver_ = nullptr;
}

mojo::PendingReceiver<network::mojom::blink::TCPConnectedSocket>
TCPSocket::GetTCPSocketReceiver() {
  return tcp_socket_.BindNewPipeAndPassReceiver(
      GetExecutionContext()->GetTaskRunner(TaskType::kNetworking));
}

mojo::PendingRemote<network::mojom::blink::SocketObserver>
TCPSocket::GetTCPSocketObserver() {
  auto pending_remote = socket_observer_.BindNewPipeAndPassRemote(
      GetExecutionContext()->GetTaskRunner(TaskType::kNetworking));

  socket_observer_.set_disconnect_handler(
      WTF::Bind(&TCPSocket::OnSocketConnectionError, WrapPersistent(this)));

  return pending_remote;
}

void TCPSocket::OnSocketConnectionError() {
  if (!Initialized()) {
    return;
  }

  readable_stream_wrapper_->ErrorStream(net::ERR_CONNECTION_ABORTED);
  writable_stream_wrapper_->ErrorStream(net::ERR_CONNECTION_ABORTED);
}

void TCPSocket::OnServiceConnectionError() {
  if (opened_resolver_) {
    Init(net::ERR_CONTEXT_SHUT_DOWN, absl::nullopt, absl::nullopt,
         mojo::ScopedDataPipeConsumerHandle(),
         mojo::ScopedDataPipeProducerHandle());
  }
}

void TCPSocket::OnReadError(int32_t net_error) {
  // |net_error| equal to net::OK means EOF -- in this case the
  // stream is not really errored but rather closed gracefully.
  readable_stream_wrapper_->ErrorStream(net_error);
}

void TCPSocket::OnWriteError(int32_t net_error) {
  writable_stream_wrapper_->ErrorStream(net_error);
}

void TCPSocket::Trace(Visitor* visitor) const {
  visitor->Trace(tcp_socket_);
  visitor->Trace(socket_observer_);

  ScriptWrappable::Trace(visitor);
  Socket::Trace(visitor);
  ActiveScriptWrappable::Trace(visitor);
}

bool TCPSocket::HasPendingActivity() const {
  return Socket::HasPendingActivity();
}

void TCPSocket::OnBothStreamsClosed(std::vector<bool> args) {
  DCHECK_EQ(args.size(), 2U);
  // At least one callback was invoked with error = true.
  bool error = base::ranges::any_of(args, base::identity());

  tcp_socket_.reset();
  socket_observer_.reset();

  CloseServiceAndResetFeatureHandle();
  ResolveOrRejectClosed(error);
}

}  // namespace blink

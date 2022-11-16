// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/compute_pressure/pressure_manager_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/device/compute_pressure/cpu_probe.h"
#include "services/device/compute_pressure/pressure_sample.h"
#include "services/device/compute_pressure/pressure_sampler.h"
#include "services/device/public/mojom/pressure_state.mojom.h"

namespace device {

constexpr base::TimeDelta PressureManagerImpl::kDefaultSamplingInterval;

// static
std::unique_ptr<PressureManagerImpl> PressureManagerImpl::Create() {
  return base::WrapUnique(new PressureManagerImpl(
      CpuProbe::Create(), PressureManagerImpl::kDefaultSamplingInterval));
}

// static
std::unique_ptr<PressureManagerImpl> PressureManagerImpl::CreateForTesting(
    std::unique_ptr<CpuProbe> cpu_probe,
    base::TimeDelta sampling_interval) {
  return base::WrapUnique(
      new PressureManagerImpl(std::move(cpu_probe), sampling_interval));
}

PressureManagerImpl::PressureManagerImpl(std::unique_ptr<CpuProbe> cpu_probe,
                                         base::TimeDelta sampling_interval)
    // base::Unretained usage is safe here because the callback is only run
    // while `sampler_` is alive, and `sampler_` is owned by this instance.
    : sampler_(std::move(cpu_probe),
               sampling_interval,
               base::BindRepeating(&PressureManagerImpl::UpdateClients,
                                   base::Unretained(this))) {
  // base::Unretained use is safe because mojo guarantees the callback will not
  // be called after `clients_` is deallocated, and `clients_` is owned by
  // PressureManagerImpl.
  clients_.set_disconnect_handler(
      base::BindRepeating(&PressureManagerImpl::OnClientRemoteDisconnected,
                          base::Unretained(this)));
}

PressureManagerImpl::~PressureManagerImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void PressureManagerImpl::Bind(
    mojo::PendingReceiver<mojom::PressureManager> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  receivers_.Add(this, std::move(receiver));
}

void PressureManagerImpl::AddClient(
    mojo::PendingRemote<mojom::PressureClient> client,
    AddClientCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!sampler_.has_probe()) {
    std::move(callback).Run(false);
    return;
  }
  clients_.Add(std::move(client));
  sampler_.EnsureStarted();
  std::move(callback).Run(true);
}

void PressureManagerImpl::UpdateClients(PressureSample sample) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const mojom::PressureState state{sample.cpu_utilization};
  const base::Time timestamp = base::Time::Now();
  for (auto& client : clients_)
    client->PressureStateChanged(state.Clone(), timestamp);
}

void PressureManagerImpl::OnClientRemoteDisconnected(
    mojo::RemoteSetElementId /*id*/) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (clients_.empty())
    sampler_.Stop();
}

}  // namespace device

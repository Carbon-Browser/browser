// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_TETHER_HOST_SCAN_SCHEDULER_IMPL_H_
#define ASH_COMPONENTS_TETHER_HOST_SCAN_SCHEDULER_IMPL_H_

#include <memory>

#include "ash/components/tether/host_scan_scheduler.h"
#include "ash/components/tether/host_scanner.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chromeos/ash/components/network/network_state_handler.h"
#include "chromeos/ash/components/network/network_state_handler_observer.h"
// TODO(https://crbug.com/1164001): move to forward declaration
#include "chromeos/ash/components/network/network_type_pattern.h"
#include "components/session_manager/core/session_manager_observer.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace session_manager {
class SessionManager;
}  // namespace session_manager

namespace ash {

namespace tether {

// Concrete HostScanScheduler implementation. One of three events begin a scan
// attempt:
//   (1) NetworkStateHandler requests a Tether network scan.
//   (2) The device loses its Internet connection.
//   (3) The scan is explicitly requested via ScheduleScan().
class HostScanSchedulerImpl : public HostScanScheduler,
                              public NetworkStateHandlerObserver,
                              public HostScanner::Observer,
                              public session_manager::SessionManagerObserver {
 public:
  HostScanSchedulerImpl(NetworkStateHandler* network_state_handler,
                        HostScanner* host_scanner,
                        session_manager::SessionManager* session_manager);

  HostScanSchedulerImpl(const HostScanSchedulerImpl&) = delete;
  HostScanSchedulerImpl& operator=(const HostScanSchedulerImpl&) = delete;

  ~HostScanSchedulerImpl() override;

  // HostScanScheduler:
  void AttemptScanIfOffline() override;

 protected:
  // NetworkStateHandlerObserver:
  void DefaultNetworkChanged(const NetworkState* network) override;
  void ScanRequested(const NetworkTypePattern& type) override;

  // HostScanner::Observer:
  void ScanFinished() override;

  // session_manager::SessionManagerObserver:
  void OnSessionStateChanged() override;

 private:
  friend class HostScanSchedulerImplTest;

  void AttemptScan();
  bool IsTetherNetworkConnectingOrConnected();
  bool IsOnlineOrHasActiveTetherConnection(const NetworkState* default_network);
  void LogHostScanBatchMetric();

  void SetTestDoubles(
      std::unique_ptr<base::OneShotTimer> test_host_scan_batch_timer,
      base::Clock* test_clock,
      scoped_refptr<base::TaskRunner> test_task_runner);

  NetworkStateHandler* network_state_handler_;
  HostScanner* host_scanner_;
  session_manager::SessionManager* session_manager_;

  std::unique_ptr<base::OneShotTimer> host_scan_batch_timer_;
  base::Clock* clock_;
  scoped_refptr<base::TaskRunner> task_runner_;

  base::Time last_scan_batch_start_timestamp_;
  base::Time last_scan_end_timestamp_;
  bool is_screen_locked_;

  base::WeakPtrFactory<HostScanSchedulerImpl> weak_ptr_factory_{this};
};

}  // namespace tether

}  // namespace ash

#endif  // ASH_COMPONENTS_TETHER_HOST_SCAN_SCHEDULER_IMPL_H_

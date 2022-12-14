// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_SYSTEM_SIGNALS_WIN_WIN_SYSTEM_SIGNALS_SERVICE_H_
#define CHROME_SERVICES_SYSTEM_SIGNALS_WIN_WIN_SYSTEM_SIGNALS_SERVICE_H_

#include <memory>
#include <vector>

#include "base/win/scoped_com_initializer.h"
#include "components/device_signals/core/common/mojom/system_signals.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace device_signals {
class FileSystemService;
class WmiClient;
class WscClient;
}  // namespace device_signals

namespace system_signals {

class WinSystemSignalsService
    : public device_signals::mojom::SystemSignalsService {
 public:
  explicit WinSystemSignalsService(
      mojo::PendingReceiver<device_signals::mojom::SystemSignalsService>
          receiver);

  ~WinSystemSignalsService() override;

  WinSystemSignalsService(const WinSystemSignalsService&) = delete;
  WinSystemSignalsService& operator=(const WinSystemSignalsService&) = delete;

  // mojom::SystemSignalsService:
  void GetFileSystemSignals(
      const std::vector<device_signals::GetFileSystemInfoOptions>& requests,
      GetFileSystemSignalsCallback callback) override;
  void GetAntiVirusSignals(GetAntiVirusSignalsCallback callback) override;
  void GetHotfixSignals(GetHotfixSignalsCallback callback) override;

 private:
  friend class WinSystemSignalsServiceTest;

  // Constructor that can be used by tests to mock out dependencies.
  WinSystemSignalsService(
      mojo::PendingReceiver<device_signals::mojom::SystemSignalsService>
          receiver,
      std::unique_ptr<device_signals::FileSystemService> file_system_service,
      std::unique_ptr<device_signals::WmiClient> wmi_client,
      std::unique_ptr<device_signals::WscClient> wsc_client);

  mojo::Receiver<device_signals::mojom::SystemSignalsService> receiver_;
  std::unique_ptr<device_signals::FileSystemService> file_system_service_;
  std::unique_ptr<device_signals::WmiClient> wmi_client_;
  std::unique_ptr<device_signals::WscClient> wsc_client_;
  base::win::ScopedCOMInitializer scoped_com_initializer_;
};

}  // namespace system_signals

#endif  // CHROME_SERVICES_SYSTEM_SIGNALS_WIN_WIN_SYSTEM_SIGNALS_SERVICE_H_

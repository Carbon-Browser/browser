// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_STATE_INFORMER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_STATE_INFORMER_H_

#include <map>
#include <memory>
#include <string>

#include "base/cancelable_callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ash/login/screens/network_error.h"
#include "chrome/browser/ash/login/ui/captive_portal_window_proxy.h"
#include "chromeos/ash/components/network/network_state_handler.h"
#include "chromeos/ash/components/network/network_state_handler_observer.h"
#include "chromeos/ash/components/network/portal_detector/network_portal_detector.h"

namespace base {
class Value;
}

namespace chromeos {

// Class which observes network state changes and calls registered callbacks.
// State is considered changed if connection or the active network has been
// changed. Also, it answers to the requests about current network state.
class NetworkStateInformer : public chromeos::NetworkStateHandlerObserver,
                             public chromeos::NetworkPortalDetector::Observer,
                             public CaptivePortalWindowProxyDelegate,
                             public base::RefCounted<NetworkStateInformer> {
 public:
  enum State {
    OFFLINE = 0,
    ONLINE,
    CAPTIVE_PORTAL,
    CONNECTING,
    PROXY_AUTH_REQUIRED,
    UNKNOWN
  };

  class NetworkStateInformerObserver {
   public:
    NetworkStateInformerObserver() {}
    virtual ~NetworkStateInformerObserver() {}

    virtual void UpdateState(NetworkError::ErrorReason reason) = 0;
    virtual void OnNetworkReady() {}
  };

  NetworkStateInformer();

  void Init();

  // Adds observer to be notified when network state has been changed.
  void AddObserver(NetworkStateInformerObserver* observer);

  // Removes observer.
  void RemoveObserver(NetworkStateInformerObserver* observer);

  // NetworkStateHandlerObserver implementation:
  void DefaultNetworkChanged(const NetworkState* network) override;

  // NetworkPortalDetector::Observer implementation:
  void OnPortalDetectionCompleted(
      const NetworkState* network,
      const NetworkPortalDetector::CaptivePortalStatus status) override;

  // CaptivePortalWindowProxyDelegate implementation:
  void OnPortalDetected() override;

  State state() const { return state_; }
  std::string network_path() const { return network_path_; }

  static const char* StatusString(State state);
  static std::string GetNetworkName(const std::string& service_path);
  static bool IsOnline(State state, NetworkError::ErrorReason reason);
  static bool IsBehindCaptivePortal(State state,
                                    NetworkError::ErrorReason reason);
  static bool IsProxyError(State state, NetworkError::ErrorReason reason);

 private:
  friend class base::RefCounted<NetworkStateInformer>;

  ~NetworkStateInformer() override;

  bool UpdateState();
  bool UpdateProxyConfig();
  void UpdateStateAndNotify();
  void SendStateToObservers(NetworkError::ErrorReason reason);

  State state_;
  std::string network_path_;
  base::Value proxy_config_;

  base::ObserverList<NetworkStateInformerObserver>::Unchecked observers_;

  base::ScopedObservation<chromeos::NetworkStateHandler,
                          chromeos::NetworkStateHandlerObserver>
      network_state_handler_observer_{this};

  base::WeakPtrFactory<NetworkStateInformer> weak_ptr_factory_{this};
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove when moved to ash.
namespace ash {
using ::chromeos::NetworkStateInformer;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_STATE_INFORMER_H_

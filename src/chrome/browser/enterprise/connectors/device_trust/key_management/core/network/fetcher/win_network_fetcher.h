// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENTERPRISE_CONNECTORS_DEVICE_TRUST_KEY_MANAGEMENT_CORE_NETWORK_FETCHER_WIN_NETWORK_FETCHER_H_
#define CHROME_BROWSER_ENTERPRISE_CONNECTORS_DEVICE_TRUST_KEY_MANAGEMENT_CORE_NETWORK_FETCHER_WIN_NETWORK_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"

class GURL;

namespace enterprise_connectors {

// Interface for the object in charge of issuing a windows key network upload
// request.
class WinNetworkFetcher {
 public:
  // Network request completion callback. The single argument is the response
  // code of the network request.
  using FetchCompletedCallback = base::OnceCallback<void(int)>;

  virtual ~WinNetworkFetcher() = default;

  static std::unique_ptr<WinNetworkFetcher> Create(
      const GURL& url,
      const std::string& body,
      base::flat_map<std::string, std::string>& headers);

  static void SetInstanceForTesting(std::unique_ptr<WinNetworkFetcher> fetcher);

  // Sends a DeviceManagementRequest to the DM server and returns the HTTP
  // response to the `callback`.
  virtual void Fetch(FetchCompletedCallback callback) = 0;
};

}  // namespace enterprise_connectors

#endif  // CHROME_BROWSER_ENTERPRISE_CONNECTORS_DEVICE_TRUST_KEY_MANAGEMENT_CORE_NETWORK_FETCHER_WIN_NETWORK_FETCHER_H_

/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace adblock {

// Maintains a state of available Subscriptions on the UI thread and
// synchronizes it with disk-based storage. Allows adding, removing and querying
// Subscriptions.
class SubscriptionService : public KeyedService {
 public:
  using InstallationCallback = base::OnceCallback<void(bool success)>;
  using PingCallback = base::OnceCallback<void(bool success)>;
  // Subscriptions need to be loaded from storage, this is an asynchronous
  // operation. The service is considered not initialized until the first load
  // completes.
  virtual bool IsInitialized() const = 0;
  // Lets callers execute |task| shortly after the service becomes initialized.
  // The tasks are executed in FIFO order.
  virtual void RunWhenInitialized(base::OnceClosure task) = 0;
  // Returns currently available subscriptions. Includes subscriptions that are
  // still being downloaded.
  // Service must be initialized.
  virtual std::vector<scoped_refptr<Subscription>> GetCurrentSubscriptions()
      const = 0;
  // Returns a snapshot of subscriptions as present at the time of calling the
  // function that can be used to query filters.
  // The result may be passed between threads, even called
  // concurrently, and future changes to the installed subscriptions (ex.
  // AddInstalledSubscription, RemoveInstalledSubscription) will not impact it.
  // Service must be initialized.
  virtual std::unique_ptr<SubscriptionCollection> GetCurrentSnapshot()
      const = 0;
  // Downloads a subscription from |subscription_url| and installs it or updates
  // an existing one with the downloaded data. The subscription is stored
  // persistently. |on_finished| is called when installation finishes, providing
  // a status.
  virtual void DownloadAndInstallSubscription(
      const GURL& subscription_url,
      InstallationCallback on_finished) = 0;
  // Does an HEAD request on acceptable ads subscription with last known version
  // and stores received version
  // TODO(mpawlowski): Remove in DPD-1154.
  virtual void PingAcceptableAds(PingCallback on_finished) = 0;
  // Removes a previously installed subscription with GetSourceUrl() ==
  // |subscription_url| both from in-memory state and from persistent storage.
  virtual void UninstallSubscription(const GURL& subscription_url) = 0;
  // Assign custom filter list
  virtual void SetCustomFilters(const std::vector<std::string>& filters) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_SERVICE_H_

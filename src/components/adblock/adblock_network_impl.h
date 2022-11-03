/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_NETWORK_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_NETWORK_IMPL_H_

#include "third_party/libadblockplus/src/include/AdblockPlus/IWebRequest.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"

namespace network {
class SimpleURLLoader;

namespace mojom {
class URLLoaderFactory;
}
}  // namespace network

class GURL;

namespace adblock {
/**
 * @brief Implements IWebRequest from libadblockplus so that the core can have
 * access to network and download subscriptions. Lives in abp dedicated thread
 * in the browser process.
 */
class AdblockNetworkImpl final : public AdblockPlus::IWebRequest {
 public:
  AdblockNetworkImpl(scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
                     scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
                     network::mojom::URLLoaderFactory* loader_factory);
  ~AdblockNetworkImpl() final;
  AdblockNetworkImpl& operator=(const AdblockNetworkImpl& other) = delete;
  AdblockNetworkImpl(const AdblockNetworkImpl& other) = delete;
  AdblockNetworkImpl& operator=(AdblockNetworkImpl&& other) = delete;
  AdblockNetworkImpl(AdblockNetworkImpl&& other) = delete;

  // AdblockPlus::IWebRequest overrides:
  void GET(const std::string& url,
           const AdblockPlus::HeaderList& headers,
           const IWebRequest::RequestCallback& callback) final;

  void HEAD(const std::string& url,
            const AdblockPlus::HeaderList& headers,
            const IWebRequest::RequestCallback& callback) final;

 private:
  void DoRequest(const char* method,
                 const std::string& url,
                 const AdblockPlus::HeaderList& headers,
                 const IWebRequest::RequestCallback& callback);

  void NotifyResults(const AdblockPlus::IWebRequest::RequestCallback& callback,
                     const GURL& url,
                     const AdblockPlus::ServerResponse& response);

  scoped_refptr<base::SingleThreadTaskRunner> ui_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> abp_runner_;
  network::mojom::URLLoaderFactory* factory_{nullptr};
  base::WeakPtrFactory<AdblockNetworkImpl> weak_ptr_factory_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_NETWORK_IMPL_H_

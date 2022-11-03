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

#ifndef CHROME_COMMON_ADBLOCK_ADBLOCK_URL_LOADER_THROTTLE_H_
#define CHROME_COMMON_ADBLOCK_ADBLOCK_URL_LOADER_THROTTLE_H_

#include "components/adblock/adblock.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace adblock {

class AdblockURLLoaderThrottle final : public blink::URLLoaderThrottle {
 public:
  AdblockURLLoaderThrottle(
      adblock::mojom::AdblockInterface* adblock,
      int render_frame_id,
      base::RepeatingCallback<void(std::string*)> user_agent_getter);

  ~AdblockURLLoaderThrottle() final;

  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;
  void DetachFromCurrentSequence() override;

 private:
  void OnFilterMatchResult(const GURL& url,
                           int resource_type,
                           adblock::mojom::FilterMatchResult result);
  void OnProcessHeadersResult();

  adblock::mojom::AdblockInterface* adblock_;
  int render_frame_id_;
  mojo::PendingRemote<adblock::mojom::AdblockInterface> adblock_pending_remote_;
  mojo::Remote<adblock::mojom::AdblockInterface> adblock_remote_;
  base::RepeatingCallback<void(std::string*)> user_agent_getter_;
  base::WeakPtrFactory<AdblockURLLoaderThrottle> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // CHROME_COMMON_ADBLOCK_ADBLOCK_URL_LOADER_THROTTLE_H_

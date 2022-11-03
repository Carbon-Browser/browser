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

#ifndef CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_
#define CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_

#include "components/adblock/adblock.mojom.h"

#include "mojo/public/cpp/bindings/receiver_set.h"

class GURL;

namespace adblock {
/**
 * @brief Implements mojo interface so that renderer can call filter engine
 * through the browser process
 *
 */
class AdblockMojoInterfaceImpl final : public mojom::AdblockInterface {
 public:
  explicit AdblockMojoInterfaceImpl(int32_t render_process_id);
  ~AdblockMojoInterfaceImpl() final;
  void CheckFilterMatch(
      const GURL& request_url,
      int32_t resource_type,
      int32_t render_frame_id,
      mojom::AdblockInterface::CheckFilterMatchCallback cb) final;
  void Clone(mojo::PendingReceiver<mojom::AdblockInterface> receiver) final;
  void ProcessResponseHeaders(
      const GURL& request_url,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent,
      mojom::AdblockInterface::ProcessResponseHeadersCallback cb) final;

 private:
  int32_t render_process_id_;
  mojo::ReceiverSet<mojom::AdblockInterface> receivers_;
};

}  // namespace adblock

#endif  // CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_

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

#ifndef CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_
#define CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_

#include "components/adblock/content/browser/adblock_content_utils.h"
#include "components/adblock/content/common/mojom/adblock.mojom.h"

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
      int32_t render_frame_id,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent,
      mojom::AdblockInterface::ProcessResponseHeadersCallback cb) final;
  void CheckRewriteFilterMatch(
      const ::GURL& request_url,
      int32_t render_frame_id,
      mojom::AdblockInterface::CheckRewriteFilterMatchCallback callback) final;

 private:
  void OnRequestUrlClassified(
      const GURL& request_url,
      int32_t render_frame_id,
      int32_t resource_type,
      mojom::AdblockInterface::CheckFilterMatchCallback callback,
      adblock::mojom::FilterMatchResult result);
  void OnResponseHeadersClassified(
      const GURL& response_url,
      int32_t render_frame_id,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      const std::string& user_agent,
      mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
      adblock::mojom::FilterMatchResult result);
  void PostFilterMatchCallbackToUI(
      mojom::AdblockInterface::CheckFilterMatchCallback callback,
      mojom::FilterMatchResult result);
  void PostResponseHeadersCallbackToUI(
      mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
      mojom::FilterMatchResult result,
      network::mojom::ParsedHeadersPtr parsed_headers);
  void PostRewriteCallbackToUI(
      base::OnceCallback<void(const absl::optional<GURL>&)> callback,
      const absl::optional<GURL>& url);
  int32_t render_process_id_;
  mojo::ReceiverSet<mojom::AdblockInterface> receivers_;
  base::WeakPtrFactory<AdblockMojoInterfaceImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // CHROME_BROWSER_ADBLOCK_ADBLOCK_MOJO_INTERFACE_IMPL_H_

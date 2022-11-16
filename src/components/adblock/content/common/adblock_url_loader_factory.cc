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

#include "components/adblock/content/common/adblock_url_loader_factory.h"

#include "base/barrier_closure.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "components/adblock/core/features.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {

class AdblockURLLoaderFactory::InProgressRequest
    : public network::mojom::URLLoader,
      public network::mojom::URLLoaderClient {
 public:
  InProgressRequest(
      AdblockURLLoaderFactory* factory,
      mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      const std::string& user_agent_string,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);

  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const absl::optional<GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

  void OnReceiveEarlyHints(
      ::network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(::network::mojom::URLResponseHeadPtr head,
                         ::mojo::ScopedDataPipeConsumerHandle body) override;
  void OnReceiveRedirect(const ::net::RedirectInfo& redirect_info,
                         ::network::mojom::URLResponseHeadPtr head) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnReceiveCachedMetadata(::mojo_base::BigBuffer data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnComplete(const ::network::URLLoaderCompletionStatus& status) override;

 private:
  void OnBindingsClosed();
  void Start(int32_t request_id,
             uint32_t options,
             network::ResourceRequest request,
             const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
             mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
             mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
             const absl::optional<GURL>& rewrite);
  void OnFilterMatchResult(
      ::mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      ::mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      adblock::mojom::FilterMatchResult result);
  void OnProcessHeadersResult(::network::mojom::URLResponseHeadPtr head,
                              ::mojo::ScopedDataPipeConsumerHandle body,
                              adblock::mojom::FilterMatchResult result,
                              network::mojom::ParsedHeadersPtr parsed_headers);
  void OnRequestError(int error_code);

  const GURL request_url_;
  const std::string& user_agent_string_;
  const raw_ptr<AdblockURLLoaderFactory> factory_;
  // There are the mojo pipe endpoints between this proxy and the renderer.
  // Messages received by |client_receiver_| are forwarded to
  // |target_client_|.
  mojo::Remote<network::mojom::URLLoaderClient> target_client_;
  mojo::Receiver<network::mojom::URLLoader> loader_receiver_;
  // These are the mojo pipe endpoints between this proxy and the network
  // process. Messages received by |loader_receiver_| are forwarded to
  // |target_loader_|.
  mojo::Remote<network::mojom::URLLoader> target_loader_;
  mojo::Receiver<network::mojom::URLLoaderClient> client_receiver_{this};
  base::WeakPtrFactory<AdblockURLLoaderFactory::InProgressRequest>
      weak_factory_{this};
};

AdblockURLLoaderFactory::InProgressRequest::InProgressRequest(
    AdblockURLLoaderFactory* factory,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    const std::string& user_agent_string,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
    : request_url_(request.url),
      user_agent_string_(user_agent_string),
      factory_(factory),
      target_client_(std::move(client)),
      loader_receiver_(this, std::move(loader_receiver)) {
  factory_->adblock_interface_->CheckRewriteFilterMatch(
      request.url, factory_->frame_tree_node_id_,
      base::BindOnce(&InProgressRequest::Start, weak_factory_.GetWeakPtr(),
                     request_id, options, request, traffic_annotation,
                     target_loader_.BindNewPipeAndPassReceiver(),
                     client_receiver_.BindNewPipeAndPassRemote()));

  // Calls |OnBindingsClosed| only after both disconnect handlers have been run.
  base::RepeatingClosure closure = base::BarrierClosure(
      2, base::BindOnce(&InProgressRequest::OnBindingsClosed,
                        base::Unretained(this)));
  loader_receiver_.set_disconnect_handler(closure);
  client_receiver_.set_disconnect_handler(closure);
}

void AdblockURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  if (target_loader_.is_bound()) {
    target_loader_->FollowRedirect(removed_headers, modified_headers,
                                   modified_cors_exempt_headers, new_url);
  }
}

void AdblockURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound()) {
    target_loader_->SetPriority(priority, intra_priority_value);
  }
}

void AdblockURLLoaderFactory::InProgressRequest::PauseReadingBodyFromNet() {
  if (target_loader_.is_bound()) {
    target_loader_->PauseReadingBodyFromNet();
  }
}

void AdblockURLLoaderFactory::InProgressRequest::ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound()) {
    target_loader_->ResumeReadingBodyFromNet();
  }
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {
  target_client_->OnReceiveEarlyHints(std::move(early_hints));
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head,
    mojo::ScopedDataPipeConsumerHandle body) {
  if (net::IsLocalhost(request_url_) || (!request_url_.SchemeIsHTTPOrHTTPS() &&
                                         !request_url_.SchemeIsWSOrWSS())) {
    VLOG(1)
        << "[eyeo] Ignoring URL (local url or unsupported scheme), allowing "
           "load.";
    target_client_->OnReceiveResponse(std::move(head), std::move(body));
    return;
  }

  VLOG(1) << "[eyeo] Sending headers for processing: " << request_url_;
  client_receiver_.Pause();
  const scoped_refptr<net::HttpResponseHeaders>& headers = head->headers;
  factory_->adblock_interface_->ProcessResponseHeaders(
      request_url_, factory_->frame_tree_node_id_, headers,
      user_agent_string_,
      base::BindOnce(&InProgressRequest::OnProcessHeadersResult,
                     weak_factory_.GetWeakPtr(), std::move(head),
                     std::move(body)));
}

void AdblockURLLoaderFactory::InProgressRequest::OnProcessHeadersResult(
    ::network::mojom::URLResponseHeadPtr head,
    ::mojo::ScopedDataPipeConsumerHandle body,
    adblock::mojom::FilterMatchResult result,
    network::mojom::ParsedHeadersPtr parsed_headers) {
  if (result == adblock::mojom::FilterMatchResult::kBlockRule) {
    OnRequestError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }
  if (parsed_headers) {
    // Headers were modified by ProcessResponseHeaders(). Raw headers must match
    // parsed headers.
    // |new_response_head| already contains the modified raw headers, update the
    // parsed headers.
    head->parsed_headers = std::move(parsed_headers);
  }

  target_client_->OnReceiveResponse(std::move(head), std::move(body));
  client_receiver_.Resume();
}

void AdblockURLLoaderFactory::InProgressRequest::OnRequestError(
    int error_code) {
  target_client_->OnComplete(network::URLLoaderCompletionStatus(error_code));
  factory_->RemoveRequest(this);
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  target_client_->OnReceiveRedirect(redirect_info, std::move(head));
}

void AdblockURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void AdblockURLLoaderFactory::InProgressRequest::OnReceiveCachedMetadata(
    mojo_base::BigBuffer data) {
  target_client_->OnReceiveCachedMetadata(std::move(data));
}

void AdblockURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void AdblockURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  target_client_->OnComplete(status);
}

void AdblockURLLoaderFactory::InProgressRequest::OnBindingsClosed() {
  factory_->RemoveRequest(this);
}

void AdblockURLLoaderFactory::InProgressRequest::Start(
    int32_t request_id,
    uint32_t options,
    network::ResourceRequest request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
    const absl::optional<GURL>& rewrite) {
  if (rewrite) {
    constexpr int kInternalRedirectStatusCode = net::HTTP_TEMPORARY_REDIRECT;
    net::RedirectInfo redirect_info = net::RedirectInfo::ComputeRedirectInfo(
        request.method, request.url, request.site_for_cookies,
        request.update_first_party_url_on_redirect
            ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
            : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
        request.referrer_policy, request.referrer.spec(),
        kInternalRedirectStatusCode, rewrite.value(),
        absl::nullopt /* referrer_policy_header */,
        false /* insecure_scheme_was_upgraded */, false /* copy_fragment */,
        false /* is_signed_exchange_fallback_redirect */);
    redirect_info.bypass_redirect_checks = true;

    auto head = network::mojom::URLResponseHead::New();
    std::string headers = base::StringPrintf(
        "HTTP/1.1 %i Internal Redirect\n"
        "Location: %s\n"
        "Non-Authoritative-Reason: ABPC\n\n",
        kInternalRedirectStatusCode, rewrite.value().spec().c_str());
    head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(headers));
    head->encoded_data_length = 0;

    // Close the connection with the current URLLoader and inform the
    // client about redirect. New URLLoader will be recreated after redirect.
    client_receiver_.reset();
    target_loader_.reset();
    target_client_->OnReceiveRedirect(redirect_info, std::move(head));
    return;
  }

  if (!factory_->target_factory_.is_bound()) {
    return;
  }

  if (net::IsLocalhost(request_url_) || (!request_url_.SchemeIsHTTPOrHTTPS() &&
                                         !request_url_.SchemeIsWSOrWSS())) {
    VLOG(1)
        << "[eyeo] Ignoring URL (local url or unsupported scheme), allowing "
           "load.";
    factory_->target_factory_->CreateLoaderAndStart(
        std::move(target_loader), request_id, options, request,
        std::move(proxy_client), traffic_annotation);
    return;
  }

  VLOG(1) << "[eyeo] Checking filter match for: " << request.url << " ("
          << request.resource_type << ")";

  factory_->adblock_interface_->CheckFilterMatch(
      request.url, request.resource_type, factory_->frame_tree_node_id_,
      base::BindOnce(&InProgressRequest::OnFilterMatchResult,
                     weak_factory_.GetWeakPtr(), std::move(target_loader),
                     request_id, options, request, std::move(proxy_client),
                     traffic_annotation));
}

void AdblockURLLoaderFactory::InProgressRequest::OnFilterMatchResult(
    ::mojo::PendingReceiver<network::mojom::URLLoader> target_loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> proxy_client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    adblock::mojom::FilterMatchResult result) {
  if (!factory_->target_factory_.is_bound()) {
    return;
  }
  if (result == adblock::mojom::FilterMatchResult::kBlockRule) {
    OnRequestError(net::ERR_BLOCKED_BY_ADMINISTRATOR);
    return;
  }
  factory_->target_factory_->CreateLoaderAndStart(
      std::move(target_loader), request_id, options, request,
      std::move(proxy_client), traffic_annotation);
}

AdblockURLLoaderFactory::AdblockURLLoaderFactory(
    std::unique_ptr<mojom::AdblockInterface> adblock_interface,
    int frame_tree_node_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    std::string user_agent_string,
    DisconnectCallback on_disconnect)
    : adblock_interface_(std::move(adblock_interface)),
      frame_tree_node_id_(frame_tree_node_id),
      user_agent_string_(std::move(user_agent_string)),
      on_disconnect_(std::move(on_disconnect)) {
  target_factory_.Bind(std::move(target_factory));
  target_factory_.set_disconnect_handler(base::BindOnce(
      &AdblockURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));
  proxy_receivers_.Add(this, std::move(receiver));
  proxy_receivers_.set_disconnect_handler(base::BindRepeating(
      &AdblockURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));
}

AdblockURLLoaderFactory::~AdblockURLLoaderFactory() = default;

void AdblockURLLoaderFactory::CreateLoaderAndStart(
    ::mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    ::mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  requests_.insert(std::make_unique<InProgressRequest>(
      this, std::move(loader), request_id, options, request, user_agent_string_,
      std::move(client), traffic_annotation));
}

void AdblockURLLoaderFactory::Clone(
    ::mojo::PendingReceiver<URLLoaderFactory> factory) {
  proxy_receivers_.Add(this, std::move(factory));
}

void AdblockURLLoaderFactory::OnTargetFactoryError() {
  target_factory_.reset();
  proxy_receivers_.Clear();
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::OnProxyBindingError() {
  if (proxy_receivers_.empty())
    target_factory_.reset();
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::RemoveRequest(InProgressRequest* request) {
  auto it = requests_.find(request);
  DCHECK(it != requests_.end());
  requests_.erase(it);
  MaybeDestroySelf();
}

void AdblockURLLoaderFactory::MaybeDestroySelf() {
  if (!target_factory_.is_bound() && requests_.empty())
    std::move(on_disconnect_).Run(this);
}

}  // namespace adblock

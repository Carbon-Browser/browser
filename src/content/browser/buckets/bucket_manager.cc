// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/buckets/bucket_manager.h"

#include "content/browser/buckets/bucket_context.h"
#include "content/browser/buckets/bucket_manager_host.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"

namespace content {

BucketManager::BucketManager(
    scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy)
    : quota_manager_proxy_(std::move(quota_manager_proxy)) {}

BucketManager::~BucketManager() = default;

void BucketManager::BindReceiverForRenderFrame(
    const GlobalRenderFrameHostId& render_frame_host_id,
    mojo::PendingReceiver<blink::mojom::BucketManagerHost> receiver,
    mojo::ReportBadMessageCallback bad_message_callback) {
  RenderFrameHost* rfh = RenderFrameHost::FromID(render_frame_host_id);
  DCHECK(rfh);
  DoBindReceiver(
      BucketContext(render_frame_host_id, rfh->GetLastCommittedOrigin()),
      std::move(receiver), std::move(bad_message_callback));
}

void BucketManager::BindReceiverForWorker(
    int render_process_id,
    const url::Origin& origin,
    mojo::PendingReceiver<blink::mojom::BucketManagerHost> receiver,
    mojo::ReportBadMessageCallback bad_message_callback) {
  DoBindReceiver(BucketContext(render_process_id, origin), std::move(receiver),
                 std::move(bad_message_callback));
}

void BucketManager::DoBindReceiver(
    const BucketContext& context,
    mojo::PendingReceiver<blink::mojom::BucketManagerHost> receiver,
    mojo::ReportBadMessageCallback bad_message_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = hosts_.find(context.origin());
  if (it != hosts_.end()) {
    it->second->BindReceiver(std::move(receiver), context);
    return;
  }

  if (!network::IsOriginPotentiallyTrustworthy(context.origin())) {
    std::move(bad_message_callback)
        .Run("Called Buckets from an insecure context");
    return;
  }

  auto [insert_it, insert_succeeded] =
      hosts_.insert({context.origin(), std::make_unique<BucketManagerHost>(
                                           this, context.origin())});
  DCHECK(insert_succeeded);
  insert_it->second->BindReceiver(std::move(receiver), context);
}

void BucketManager::OnHostReceiverDisconnect(BucketManagerHost* host,
                                             base::PassKey<BucketManagerHost>) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(host != nullptr);
  DCHECK_GT(hosts_.count(host->origin()), 0u);
  DCHECK_EQ(hosts_[host->origin()].get(), host);

  if (host->has_connected_receivers())
    return;

  hosts_.erase(host->origin());
}

}  // namespace content

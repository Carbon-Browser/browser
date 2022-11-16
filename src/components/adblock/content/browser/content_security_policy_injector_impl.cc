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

#include "components/adblock/content/browser/content_security_policy_injector_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/subscription/subscription_collection.h"
#include "content/public/browser/network_service_instance.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {
namespace {

std::string GetCspInjection(
    const std::unique_ptr<SubscriptionCollection> subscription_collection,
    const GURL request_url,
    const std::vector<GURL> frame_hierarchy_chain) {
  TRACE_EVENT1("eyeo", "GetCspInjection", "url", request_url.spec());
  const auto injection = subscription_collection->GetCspInjection(
      request_url, frame_hierarchy_chain);
  if (!injection.empty()) {
    VLOG(1) << "[eyeo] Will attempt to inject CSP header \"" << injection
            << "\" for " << request_url;
  }
  return std::string(injection.data(), injection.size());
}
}  // namespace

ContentSecurityPolicyInjectorImpl::ContentSecurityPolicyInjectorImpl(
    SubscriptionService* subscription_service,
    std::unique_ptr<FrameHierarchyBuilder> frame_hierarchy_builder)
    : subscription_service_(subscription_service),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)) {}

ContentSecurityPolicyInjectorImpl::~ContentSecurityPolicyInjectorImpl() =
    default;

void ContentSecurityPolicyInjectorImpl::
    InsertContentSecurityPolicyHeadersIfApplicable(
        const GURL& request_url,
        content::GlobalRenderFrameHostId render_frame_host_id,
        const scoped_refptr<net::HttpResponseHeaders>& headers,
        InsertContentSecurityPolicyHeadersCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!subscription_service_->IsInitialized()) {
    subscription_service_->RunWhenInitialized(
        base::BindOnce(&ContentSecurityPolicyInjectorImpl::
                           InsertContentSecurityPolicyHeadersIfApplicable,
                       weak_ptr_factory.GetWeakPtr(), request_url,
                       render_frame_host_id, headers, std::move(callback)));
    return;
  }

  content::RenderFrameHost* host =
      frame_hierarchy_builder_->FindRenderFrameHost(
          render_frame_host_id.child_id, render_frame_host_id.frame_routing_id);
  if (host) {
    // GetCspInjection might take a while, let it run in the background.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {},
        base::BindOnce(&GetCspInjection,
                       subscription_service_->GetCurrentSnapshot(), request_url,
                       frame_hierarchy_builder_->BuildFrameHierarchy(host)),
        base::BindOnce(
            &ContentSecurityPolicyInjectorImpl::OnCspInjectionSearchFinished,
            weak_ptr_factory.GetWeakPtr(), request_url, headers,
            std::move(callback)));
  } else {
    // This request is likely dead, since there's no associated RenderFrameHost.
    // Post the callback to the current sequence to unblock any callers that
    // might be waiting for it.
    std::move(callback).Run(nullptr);
  }
}

void ContentSecurityPolicyInjectorImpl::OnCspInjectionSearchFinished(
    const GURL& request_url,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    InsertContentSecurityPolicyHeadersCallback callback,
    std::string csp_injection) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!csp_injection.empty()) {
    // Set the CSP header according to |csp_injection|.
    headers->SetHeader("Content-Security-Policy", csp_injection);
    // We need to ensure parsed headers match raw headers. Send the updated
    // raw headers to NetworkService for parsing.
    content::GetNetworkService()->ParseHeaders(
        request_url, headers,
        base::BindOnce(
            &ContentSecurityPolicyInjectorImpl::OnUpdatedHeadersParsed,
            weak_ptr_factory.GetWeakPtr(), std::move(callback)));
  } else {
    // No headers are injected, no need to update parsed headers.
    std::move(callback).Run(nullptr);
  }
}

void ContentSecurityPolicyInjectorImpl::OnUpdatedHeadersParsed(
    InsertContentSecurityPolicyHeadersCallback callback,
    network::mojom::ParsedHeadersPtr parsed_headers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(std::move(parsed_headers));
}

}  // namespace adblock

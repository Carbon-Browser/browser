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

#include "chrome/browser/adblock/adblock_mojo_interface_impl.h"
#include <memory>

#include "chrome/browser/adblock/content_security_policy_injector_factory.h"
#include "chrome/browser/adblock/element_hider_factory.h"
#include "chrome/browser/adblock/resource_classification_runner_factory.h"
#include "chrome/browser/adblock/sitekey_storage_factory.h"
#include "chrome/browser/adblock/subscription_service_factory.h"
#include "components/adblock/content/browser/adblock_content_utils.h"
#include "components/adblock/content/browser/content_security_policy_injector.h"
#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/content/browser/resource_classification_runner.h"
#include "components/adblock/core/sitekey_storage.h"
#include "components/adblock/core/subscription/subscription_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "services/network/public/mojom/parsed_headers.mojom.h"

namespace adblock {
namespace {

SubscriptionService* GetSubscriptionService(
    content::RenderProcessHost* process_host) {
  auto* subscription_service = SubscriptionServiceFactory::GetForBrowserContext(
      process_host->GetBrowserContext());
  DCHECK(subscription_service);
  return subscription_service;
}

// Without USER_BLOCKING priority - in session restore case - posted
// callbacks can take over a minute to trigger. This is why such high
// priority is applied everywhere.
const base::TaskPriority kTaskResponsePriority =
    base::TaskPriority::USER_BLOCKING;

}  // namespace

// We cannot call mojo callback in stack, hence this UIThread posting
void AdblockMojoInterfaceImpl::PostFilterMatchCallbackToUI(
    mojom::AdblockInterface::CheckFilterMatchCallback callback,
    mojom::FilterMatchResult result) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), result));
}

// We cannot call mojo callback in stack, hence this UIThread posting
void AdblockMojoInterfaceImpl::PostResponseHeadersCallbackToUI(
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
    mojom::FilterMatchResult result,
    network::mojom::ParsedHeadersPtr parsed_headers) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), result,
                                           std::move(parsed_headers)));
}

// We cannot call mojo callback in stack, hence this UIThread posting
void AdblockMojoInterfaceImpl::PostRewriteCallbackToUI(
    base::OnceCallback<void(const absl::optional<GURL>&)> callback,
    const absl::optional<GURL>& url) {
  content::GetUIThreadTaskRunner({kTaskResponsePriority})
      ->PostTask(FROM_HERE, base::BindOnce(std::move(callback), url));
}

AdblockMojoInterfaceImpl::AdblockMojoInterfaceImpl(int32_t render_process_id)
    : render_process_id_(render_process_id) {}

AdblockMojoInterfaceImpl::~AdblockMojoInterfaceImpl() = default;

void AdblockMojoInterfaceImpl::CheckFilterMatch(
    const GURL& request_url,
    int32_t resource_type,
    int32_t render_frame_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  auto* process_host = content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host) {
    PostFilterMatchCallbackToUI(std::move(callback),
                                mojom::FilterMatchResult::kNoRule);
    return;
  }

  auto* subscription_service = GetSubscriptionService(process_host);
  if (!subscription_service->IsInitialized()) {
    subscription_service->RunWhenInitialized(
        base::BindOnce(&AdblockMojoInterfaceImpl::CheckFilterMatch,
                       weak_ptr_factory_.GetWeakPtr(), request_url,
                       resource_type, render_frame_id, std::move(callback)));
    return;
  }

  auto* classification_runner =
      ResourceClassificationRunnerFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(classification_runner);

  classification_runner->CheckRequestFilterMatch(
      subscription_service->GetCurrentSnapshot(), request_url, resource_type,
      render_process_id_, render_frame_id,
      base::BindOnce(
          &AdblockMojoInterfaceImpl::OnRequestUrlClassified,
          weak_ptr_factory_.GetWeakPtr(), request_url, render_frame_id,
          resource_type,
          base::BindOnce(&AdblockMojoInterfaceImpl::PostFilterMatchCallbackToUI,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void AdblockMojoInterfaceImpl::ProcessResponseHeaders(
    const GURL& response_url,
    int32_t render_frame_id,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& user_agent,
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback) {
  auto* process_host = content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host) {
    PostResponseHeadersCallbackToUI(std::move(callback),
                                    mojom::FilterMatchResult::kNoRule, nullptr);
    return;
  }

  auto* subscription_service = GetSubscriptionService(process_host);
  if (!subscription_service->IsInitialized()) {
    subscription_service->RunWhenInitialized(base::BindOnce(
        &AdblockMojoInterfaceImpl::ProcessResponseHeaders,
        weak_ptr_factory_.GetWeakPtr(), response_url, render_frame_id, headers,
        user_agent, std::move(callback)));
    return;
  }

  auto* classification_runner =
      ResourceClassificationRunnerFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(classification_runner);
  classification_runner->CheckResponseFilterMatch(
      subscription_service->GetCurrentSnapshot(), response_url,
      render_process_id_, render_frame_id, headers,
      base::BindOnce(
          &AdblockMojoInterfaceImpl::OnResponseHeadersClassified,
          weak_ptr_factory_.GetWeakPtr(), response_url, render_frame_id,
          headers, user_agent,
          base::BindOnce(
              &AdblockMojoInterfaceImpl::PostResponseHeadersCallbackToUI,
              weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void AdblockMojoInterfaceImpl::OnRequestUrlClassified(
    const GURL& request_url,
    int32_t render_frame_id,
    int32_t resource_type,
    CheckFilterMatchCallback callback,
    adblock::mojom::FilterMatchResult result) {
  if (result == mojom::FilterMatchResult::kBlockRule) {
    content::RenderProcessHost* process_host =
        content::RenderProcessHost::FromID(render_process_id_);
    if (process_host) {
      adblock::ElementHider* element_hider =
          adblock::ElementHiderFactory::GetForBrowserContext(
              process_host->GetBrowserContext());
      const auto adblock_resource_type =
          utils::ConvertToAdblockResourceType(request_url, resource_type);
      if (element_hider->IsElementTypeHideable(adblock_resource_type)) {
        element_hider->HideBlockedElement(
            request_url, content::RenderFrameHost::FromID(render_process_id_,
                                                          render_frame_id));
      }
    }
  }
  PostFilterMatchCallbackToUI(std::move(callback), result);
}

void AdblockMojoInterfaceImpl::OnResponseHeadersClassified(
    const GURL& response_url,
    int32_t render_frame_id,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& user_agent,
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback,
    mojom::FilterMatchResult result) {
  content::RenderProcessHost* process_host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host || result == mojom::FilterMatchResult::kDisabled) {
    PostResponseHeadersCallbackToUI(std::move(callback), result, nullptr);
    return;
  }

  if (result == mojom::FilterMatchResult::kBlockRule) {
    adblock::ElementHider* element_hider =
        adblock::ElementHiderFactory::GetForBrowserContext(
            process_host->GetBrowserContext());
    auto adblock_resource_type = utils::DetectResourceType(response_url);
    if (element_hider->IsElementTypeHideable(adblock_resource_type)) {
      element_hider->HideBlockedElement(
          response_url, content::RenderFrameHost::FromID(render_process_id_,
                                                         render_frame_id));
    }
    PostResponseHeadersCallbackToUI(std::move(callback), result, nullptr);
    return;
  }

  auto* sitekey_storage = SitekeyStorageFactory::GetForBrowserContext(
      process_host->GetBrowserContext());
  DCHECK(sitekey_storage);
  sitekey_storage->ProcessResponseHeaders(response_url, headers, user_agent);

  auto* csp_injector =
      ContentSecurityPolicyInjectorFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(csp_injector);
  csp_injector->InsertContentSecurityPolicyHeadersIfApplicable(
      response_url,
      content::GlobalRenderFrameHostId(render_process_id_, render_frame_id),
      headers, base::BindOnce(std::move(callback), result));
}

void AdblockMojoInterfaceImpl::CheckRewriteFilterMatch(
    const GURL& request_url,
    int32_t render_frame_id,
    mojom::AdblockInterface::CheckRewriteFilterMatchCallback callback) {
  content::RenderProcessHost* process_host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host) {
    PostRewriteCallbackToUI(std::move(callback), absl::optional<GURL>{});
    return;
  }

  auto* subscription_service = GetSubscriptionService(process_host);
  if (!subscription_service->IsInitialized()) {
    subscription_service->RunWhenInitialized(
        base::BindOnce(&AdblockMojoInterfaceImpl::CheckRewriteFilterMatch,
                       weak_ptr_factory_.GetWeakPtr(), request_url,
                       render_frame_id, std::move(callback)));
    return;
  }

  auto* classification_runner =
      adblock::ResourceClassificationRunnerFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(classification_runner);
  classification_runner->CheckRewriteFilterMatch(
      subscription_service->GetCurrentSnapshot(), request_url,
      render_process_id_, render_frame_id,
      base::BindOnce(&AdblockMojoInterfaceImpl::PostRewriteCallbackToUI,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdblockMojoInterfaceImpl::Clone(
    mojo::PendingReceiver<mojom::AdblockInterface> receiver) {
  receivers_.Add(this, std::move(receiver));
}

}  // namespace adblock

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

#include "chrome/browser/adblock/adblock_mojo_interface_impl.h"

#include "chrome/browser/adblock/adblock_request_classifier_factory.h"
#include "chrome/browser/adblock/adblock_sitekey_storage_factory.h"
#include "components/adblock/adblock_request_classifier.h"
#include "components/adblock/adblock_sitekey_storage.h"
#include "content/public/browser/render_process_host.h"

namespace adblock {

AdblockMojoInterfaceImpl::AdblockMojoInterfaceImpl(int32_t render_process_id)
    : render_process_id_(render_process_id) {}

AdblockMojoInterfaceImpl::~AdblockMojoInterfaceImpl() = default;

void AdblockMojoInterfaceImpl::CheckFilterMatch(
    const GURL& request_url,
    int32_t resource_type,
    int32_t render_frame_id,
    mojom::AdblockInterface::CheckFilterMatchCallback callback) {
  content::RenderProcessHost* process_host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host) {
    std::move(callback).Run(mojom::FilterMatchResult::kNoRule);
    return;
  }

  auto* classifier =
      adblock::AdblockRequestClassifierFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(classifier);

  classifier->CheckFilterMatch(request_url, resource_type, render_process_id_,
                               render_frame_id, std::move(callback));
}

void AdblockMojoInterfaceImpl::ProcessResponseHeaders(
    const GURL& request_url,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const std::string& user_agent,
    mojom::AdblockInterface::ProcessResponseHeadersCallback callback) {
  content::RenderProcessHost* process_host =
      content::RenderProcessHost::FromID(render_process_id_);
  if (!process_host) {
    std::move(callback).Run();
    return;
  }

  auto* sitekey_storage =
      adblock::AdblockSitekeyStorageFactory::GetForBrowserContext(
          process_host->GetBrowserContext());
  DCHECK(sitekey_storage);

  sitekey_storage->ProcessResponseHeaders(request_url, headers, user_agent,
                                          std::move(callback));
}

void AdblockMojoInterfaceImpl::Clone(
    mojo::PendingReceiver<mojom::AdblockInterface> receiver) {
  receivers_.Add(this, std::move(receiver));
}

}  // namespace adblock

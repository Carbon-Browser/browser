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

#include "components/adblock/adblock_webcontents_observer.h"

#include "base/trace_event/trace_event.h"
#include "components/adblock/sitekey.h"
#include "content/public/browser/navigation_handle.h"
#include "third_party/blink/public/mojom/frame/frame_owner_element_type.mojom.h"

void TraceHandleLoadComplete(
    trace_event_internal::TraceID::LocalId trace_id,
    const adblock::AdblockElementHider::ElemhideEmulationInjectionData&) {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      "adblockplus", "AdblockWebContentObserver::HandleOnLoad", trace_id);
}

AdblockWebContentObserver::AdblockWebContentObserver(
    content::WebContents* web_contents,
    adblock::AdblockController* controller,
    adblock::AdblockElementHider* element_hider,
    adblock::AdblockSitekeyStorage* sitekey_storage,
    std::unique_ptr<adblock::AdblockFrameHierarchyBuilder>
        frame_hierarchy_builder)
    : content::WebContentsObserver(web_contents),
      controller_(controller),
      element_hider_(element_hider),
      sitekey_storage_(sitekey_storage),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)) {}

AdblockWebContentObserver::~AdblockWebContentObserver() {}

void AdblockWebContentObserver::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  HandleOnLoad(render_frame_host);
}

void AdblockWebContentObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  const GURL& url = navigation_handle->GetURL();
  LOG(INFO) << "[ABP] Finished navigation: URL=" << url
            << ", has_commited=" << navigation_handle->HasCommitted()
            << ", is_error=" << navigation_handle->IsErrorPage()
            << ", isInMainFrame=" << navigation_handle->IsInMainFrame();

  if (navigation_handle->HasCommitted()) {
    if (!navigation_handle->IsErrorPage() && !url.IsAboutBlank() &&
        url.SchemeIsHTTPOrHTTPS()) {
      DVLOG(3) << "[ABP] Ready to inject JS to " << url.spec();
      HandleOnLoad(navigation_handle->GetRenderFrameHost());
    } else {
      DVLOG(3) << "[ABP] Not suitable URL " << url.spec()
               << ", error = " << navigation_handle->GetNetErrorCode()
               << ", IsAboutSrcdoc = " << url.IsAboutSrcdoc();

      if (navigation_handle->GetNetErrorCode() ==
              net::ERR_BLOCKED_BY_ADMINISTRATOR &&
          navigation_handle->GetRenderFrameHost()->GetFrameOwnerElementType() ==
              blink::mojom::FrameOwnerElementType::kIframe) {
        // element hiding for blocked element
        DVLOG(3) << "[ABP] Subframe url=" << url.spec()
                 << ", isAboutSrcDoc = " << url.IsAboutSrcdoc();
        element_hider_->HideBlockedElement(
            url, navigation_handle->GetRenderFrameHost()->GetParent());
      }
    }
  }
}

void AdblockWebContentObserver::HandleOnLoad(
    content::RenderFrameHost* frame_host) {
  if (!controller_->IsAdblockEnabled()) {
    LOG(WARNING) << "[ABP] Adblocking is disabled, skip apply element hiding.";
    return;
  }

  DCHECK(frame_host);
  const GURL url =
      frame_hierarchy_builder_->FindUrlForFrame(frame_host, web_contents());
  const auto trace_id = TRACE_ID_LOCAL(frame_host);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("adblockplus",
                                    "AdblockWebContentObserver::HandleOnLoad",
                                    trace_id, "url", url.spec());

  auto referrers_chain =
      frame_hierarchy_builder_->BuildFrameHierarchy(frame_host);
  DLOG(INFO) << "[ABP] Got " << referrers_chain.size() << " referrers for "
             << url.spec();

  adblock::SiteKey site_key;
  auto url_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(referrers_chain);
  if (url_key_pair.has_value()) {
    site_key = url_key_pair->second;
    DVLOG(2) << "[ABP] Element hiding found siteKey: " << url_key_pair->second
             << " for url: " << url_key_pair->first;
  }

  element_hider_->ApplyElementHidingEmulationOnPage(
      std::move(url), std::move(referrers_chain), frame_host,
      std::move(site_key.value()),
      base::BindOnce(&TraceHandleLoadComplete, trace_id));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdblockWebContentObserver)

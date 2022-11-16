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

#include "components/adblock/content/browser/adblock_webcontents_observer.h"

#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/sitekey.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/frame/frame_owner_element_type.h"

void TraceHandleLoadComplete(
    trace_event_internal::TraceID::LocalId trace_id,
    const adblock::ElementHider::ElemhideInjectionData&) {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      "eyeo", "AdblockWebContentObserver::HandleOnLoad", trace_id);
}

AdblockWebContentObserver::AdblockWebContentObserver(
    content::WebContents* web_contents,
    adblock::AdblockController* controller,
    adblock::ElementHider* element_hider,
    adblock::SitekeyStorage* sitekey_storage,
    std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdblockWebContentObserver>(*web_contents),
      controller_(controller),
      element_hider_(element_hider),
      sitekey_storage_(sitekey_storage),
      frame_hierarchy_builder_(std::move(frame_hierarchy_builder)) {}

AdblockWebContentObserver::~AdblockWebContentObserver() {}

void AdblockWebContentObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  const GURL& url = navigation_handle->GetURL();
  VLOG(1) << "[eyeo] Finished navigation: URL=" << url
          << ", has_commited=" << navigation_handle->HasCommitted()
          << ", is_error=" << navigation_handle->IsErrorPage()
          << ", isInMainFrame=" << navigation_handle->IsInMainFrame();

  if (navigation_handle->HasCommitted()) {
    if (!navigation_handle->IsErrorPage()) {
      DVLOG(3) << "[eyeo] Ready to inject JS to " << url.spec();
      HandleOnLoad(navigation_handle->GetRenderFrameHost());
    }
    if (navigation_handle->IsErrorPage() || url.IsAboutBlank() ||
        !url.SchemeIsHTTPOrHTTPS()) {
      DVLOG(3) << "[eyeo] Not suitable URL " << url.spec()
               << ", error = " << navigation_handle->GetNetErrorCode()
               << ", IsAboutSrcdoc = " << url.IsAboutSrcdoc();

      if (navigation_handle->GetNetErrorCode() ==
              net::ERR_BLOCKED_BY_ADMINISTRATOR &&
          navigation_handle->GetRenderFrameHost()->GetFrameOwnerElementType() ==
              blink::FrameOwnerElementType::kIframe) {
        // element hiding for blocked element
        DVLOG(3) << "[eyeo] Subframe url=" << url.spec()
                 << ", isAboutSrcDoc = " << url.IsAboutSrcdoc();
        element_hider_->HideBlockedElement(
            url, navigation_handle->GetRenderFrameHost()->GetParent());
      }
    }
  }
}

void AdblockWebContentObserver::HandleOnLoad(
    content::RenderFrameHost* frame_host) {
  if (!controller_ || !controller_->IsAdblockEnabled()) {
    LOG(WARNING) << "[eyeo] Adblocking is disabled, skip apply element hiding.";
    return;
  }

  DCHECK(frame_host);
  const GURL url =
      frame_hierarchy_builder_->FindUrlForFrame(frame_host, web_contents());
  if (net::IsLocalhost(url)) {
    DVLOG(1) << "[eyeo] skipping element hiding on localhost URL";
    return;
  }
  const auto trace_id = TRACE_ID_LOCAL(frame_host);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("eyeo",
                                    "AdblockWebContentObserver::HandleOnLoad",
                                    trace_id, "url", url.spec());

  auto referrers_chain =
      frame_hierarchy_builder_->BuildFrameHierarchy(frame_host);
  DVLOG(1) << "[eyeo] Got " << referrers_chain.size() << " referrers for "
           << url.spec();

  adblock::SiteKey site_key;
  auto url_key_pair = sitekey_storage_->FindSiteKeyForAnyUrl(referrers_chain);
  if (url_key_pair.has_value()) {
    site_key = url_key_pair->second;
    DVLOG(2) << "[eyeo] Element hiding found siteKey: " << url_key_pair->second
             << " for url: " << url_key_pair->first;
  }

  element_hider_->ApplyElementHidingEmulationOnPage(
      std::move(url), std::move(referrers_chain), frame_host,
      std::move(site_key), base::BindOnce(&TraceHandleLoadComplete, trace_id));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdblockWebContentObserver);

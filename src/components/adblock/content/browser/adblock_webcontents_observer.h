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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_

#include "components/adblock/content/browser/element_hider.h"
#include "components/adblock/content/browser/frame_hierarchy_builder.h"
#include "components/adblock/core/adblock_controller.h"
#include "components/adblock/core/sitekey_storage.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class RenderFrameHost;
}  // namespace content

class GURL;
/**
 * @brief Listens to page load events to trigger frame-wide element hiding.
 * Responds to notifications about blocked resource loads to collapse the
 * empty space around them. Lives in browser process UI thread.
 *
 */
class AdblockWebContentObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<AdblockWebContentObserver> {
 public:
  AdblockWebContentObserver(
      content::WebContents* web_contents,
      adblock::AdblockController* controller,
      adblock::ElementHider* element_hider,
      adblock::SitekeyStorage* sitekey_storage,
      std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder);
  ~AdblockWebContentObserver() override;
  AdblockWebContentObserver(const AdblockWebContentObserver&) = delete;
  AdblockWebContentObserver& operator=(const AdblockWebContentObserver&) =
      delete;

  // WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  explicit AdblockWebContentObserver(content::WebContents* web_contents);
  void HandleOnLoad(content::RenderFrameHost* render_frame_host);

  friend class content::WebContentsUserData<AdblockWebContentObserver>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  adblock::AdblockController* controller_;
  adblock::ElementHider* element_hider_;
  adblock::SitekeyStorage* sitekey_storage_;

  std::unique_ptr<adblock::FrameHierarchyBuilder> frame_hierarchy_builder_;
};
#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ADBLOCK_WEBCONTENTS_OBSERVER_H_

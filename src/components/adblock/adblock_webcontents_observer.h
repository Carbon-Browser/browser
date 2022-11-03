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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_WEBCONTENTS_OBSERVER_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_WEBCONTENTS_OBSERVER_H_

#include "components/adblock/adblock_controller.h"
#include "components/adblock/adblock_element_hider.h"
#include "components/adblock/adblock_frame_hierarchy_builder.h"
#include "components/adblock/adblock_sitekey_storage.h"
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
      adblock::AdblockElementHider* element_hider,
      adblock::AdblockSitekeyStorage* sitekey_storage,
      std::unique_ptr<adblock::AdblockFrameHierarchyBuilder>
          frame_hierarchy_builder);
  ~AdblockWebContentObserver() override;
  AdblockWebContentObserver(const AdblockWebContentObserver&) = delete;
  AdblockWebContentObserver& operator=(const AdblockWebContentObserver&) =
      delete;

  // WebContentsObserver overrides.
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  explicit AdblockWebContentObserver(content::WebContents* web_contents);
  void HandleOnLoad(content::RenderFrameHost* render_frame_host);

  friend class content::WebContentsUserData<AdblockWebContentObserver>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  adblock::AdblockController* controller_;
  adblock::AdblockElementHider* element_hider_;
  adblock::AdblockSitekeyStorage* sitekey_storage_;

  std::unique_ptr<adblock::AdblockFrameHierarchyBuilder>
      frame_hierarchy_builder_;
};
#endif  // COMPONENTS_ADBLOCK_ADBLOCK_WEBCONTENTS_OBSERVER_H_

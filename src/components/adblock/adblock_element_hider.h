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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "components/adblock/adblock_content_type.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;

namespace content {
class RenderFrameHost;
}  // namespace content

namespace adblock {
/**
 * @brief Implements element hiding logic.
 * Element hiding includes injecting JavaScript code, CSS stylesheets and
 * predefined snippets into the context of a loaded page to hide unwanted
 * objects that could not be blocked from loading earlier.
 * Element hiding also collapses visible elements blocked during resource load.
 * Lives in browser process, UI thread.
 *
 */
class AdblockElementHider : public KeyedService {
 public:
  struct ElemhideEmulationInjectionData {
    std::string stylesheet;
    std::string elemhide_js;
    std::string snippet_js;
  };

  virtual void ApplyElementHidingEmulationOnPage(
      GURL url,
      std::vector<GURL> frame_hierarchy,
      content::RenderFrameHost* render_frame_host,
      std::string sitekey,
      base::OnceCallback<void(const ElemhideEmulationInjectionData&)>
          on_finished) = 0;

  virtual bool IsElementTypeHideable(
      ContentType adblock_resource_type) const = 0;

  virtual void HideBlockedElement(
      const GURL& url,
      content::RenderFrameHost* render_frame_host) = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_H_

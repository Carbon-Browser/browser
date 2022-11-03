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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_IMPL_H_

#include <vector>

#include "components/adblock/adblock_element_hider.h"
#include "components/adblock/adblock_platform_embedder.h"

namespace adblock {

class AdblockElementHiderImpl final : public AdblockElementHider {
 public:
  explicit AdblockElementHiderImpl(
      scoped_refptr<AdblockPlatformEmbedder> embedder);
  ~AdblockElementHiderImpl() final;

  void ApplyElementHidingEmulationOnPage(
      GURL url,
      std::vector<GURL> frame_hierarchy,
      content::RenderFrameHost* render_frame_host,
      std::string sitekey,
      base::OnceCallback<void(const ElemhideEmulationInjectionData&)>
          on_finished) final;

  bool IsElementTypeHideable(ContentType adblock_resource_type) const final;

  void HideBlockedElement(const GURL& url,
                          content::RenderFrameHost* render_frame_host) final;

 private:
  scoped_refptr<AdblockPlatformEmbedder> embedder_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_ELEMENT_HIDER_IMPL_H_

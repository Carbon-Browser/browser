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

#ifndef COMPONENTS_ADBLOCK_MOCK_ADBLOCK_ELEMENT_HIDER_H_
#define COMPONENTS_ADBLOCK_MOCK_ADBLOCK_ELEMENT_HIDER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "components/adblock/adblock_element_hider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockAdblockElementHider : public AdblockElementHider {
 public:
  MockAdblockElementHider();
  ~MockAdblockElementHider() override;
  MOCK_METHOD(
      void,
      ApplyElementHidingEmulationOnPage,
      (GURL url,
       std::vector<GURL> frame_hierarchy,
       content::RenderFrameHost* render_frame_host,
       std::string sitekey,
       base::OnceCallback<
           void(const AdblockElementHider::ElemhideEmulationInjectionData&)>),
      (override));

  MOCK_METHOD(bool, IsElementTypeHideable, (ContentType), (override, const));

  MOCK_METHOD(void,
              HideBlockedElement,
              (const GURL& url, content::RenderFrameHost* render_frame_host),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_MOCK_ADBLOCK_ELEMENT_HIDER_H_

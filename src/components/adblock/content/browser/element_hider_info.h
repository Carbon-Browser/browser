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

#ifndef COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_INFO_H_
#define COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_INFO_H_

#include "content/public/browser/document_user_data.h"

namespace adblock {

class ElementHiderInfo final
    : public content::DocumentUserData<ElementHiderInfo> {
 public:
  ~ElementHiderInfo() final;

  bool IsElementHidingDone() const;
  void SetElementHidingDone();

 private:
  explicit ElementHiderInfo(content::RenderFrameHost* rfh);

  bool element_hining_done_ = false;

  friend DocumentUserData;
  DOCUMENT_USER_DATA_KEY_DECL();
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_BROWSER_ELEMENT_HIDER_INFO_H_

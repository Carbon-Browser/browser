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

#include "components/adblock/content/browser/element_hider_info.h"

namespace adblock {

DOCUMENT_USER_DATA_KEY_IMPL(ElementHiderInfo);

ElementHiderInfo::ElementHiderInfo(content::RenderFrameHost* rfh)
    : content::DocumentUserData<ElementHiderInfo>(rfh) {}

ElementHiderInfo::~ElementHiderInfo() = default;

bool ElementHiderInfo::IsElementHidingDone() const {
  return element_hining_done_;
}

void ElementHiderInfo::SetElementHidingDone() {
  element_hining_done_ = true;
}

}  // namespace adblock

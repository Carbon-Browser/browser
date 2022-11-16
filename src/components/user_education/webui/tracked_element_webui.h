// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_USER_EDUCATION_WEBUI_TRACKED_ELEMENT_WEBUI_H_
#define COMPONENTS_USER_EDUCATION_WEBUI_TRACKED_ELEMENT_WEBUI_H_

#include "base/memory/raw_ptr.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/base/interaction/framework_specific_implementation.h"

namespace user_education {

class HelpBubbleHandlerBase;

class TrackedElementWebUI : public ui::TrackedElement {
 public:
  TrackedElementWebUI(HelpBubbleHandlerBase* handler,
                      ui::ElementIdentifier identifier,
                      ui::ElementContext context);
  ~TrackedElementWebUI() override;

  DECLARE_FRAMEWORK_SPECIFIC_METADATA()

  HelpBubbleHandlerBase* handler() { return handler_; }

 private:
  friend class HelpBubbleHandlerBase;

  void SetVisible(bool visible);
  bool visible() const { return visible_; }

  const base::raw_ptr<HelpBubbleHandlerBase> handler_;
  bool visible_ = false;
};

}  // namespace user_education

#endif  // COMPONENTS_USER_EDUCATION_WEBUI_TRACKED_ELEMENT_WEBUI_H_
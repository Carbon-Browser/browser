// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_UI_EDUCATIONAL_VIEW_H_
#define CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_UI_EDUCATIONAL_VIEW_H_

#include <memory>

#include "ash/public/cpp/view_shadow.h"
#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace ash {
class PillButton;
}  // namespace ash

namespace arc {
namespace input_overlay {
class DisplayOverlayController;

// Educational view that is displayed on the first run per app/game, it contains
// information on how to use the feature.
class EducationalView : public views::View {
 public:
  static EducationalView* Show(
      DisplayOverlayController* display_overlay_controller,
      views::View* parent);

  EducationalView(DisplayOverlayController* display_overlay_controller,
                  int parent_width);

  EducationalView(const EducationalView&) = delete;
  EducationalView& operator=(const EducationalView&) = delete;
  ~EducationalView() override;

 private:
  void Init(const gfx::Size& parent_size);
  void OnAcceptedPressed();
  // Shadow has to be added after the view is added to its parent to display the
  // shadow correctly.
  void AddShadow();

  raw_ptr<ash::PillButton> accept_button_ = nullptr;
  // View shadow for this view.
  std::unique_ptr<ash::ViewShadow> view_shadow_;
  // Whether or not phone specs should be used.
  bool portrait_mode_ = false;

  // DisplayOverlayController owns this class, no need to deallocate.
  const raw_ptr<DisplayOverlayController> display_overlay_controller_ = nullptr;
};
}  // namespace input_overlay
}  // namespace arc

#endif  // CHROME_BROWSER_ASH_ARC_INPUT_OVERLAY_UI_EDUCATIONAL_VIEW_H_

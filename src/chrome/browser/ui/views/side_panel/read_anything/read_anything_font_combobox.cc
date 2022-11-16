// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_font_combobox.h"

#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_model.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/combobox/combobox_menu_model.h"

// Adapts a ui::ComboboxModel for Read Anything.
class ReadAnythingFontCombobox::MenuModel : public ComboboxMenuModel {
 public:
  MenuModel(Combobox* owner, ui::ComboboxModel* model)
      : ComboboxMenuModel(owner, model) {}
  MenuModel(const MenuModel&) = delete;
  MenuModel& operator&(const MenuModel&) = delete;
  ~MenuModel() override = default;

  // Overridden from ComboboxMenuModel:
 private:
  // The Read Anything font combobox will not have icons on any platform.
  bool HasIcons() const override { return false; }

  // The Read Anything font combobox will use a different FontList for each
  // item in the menu. This will give a preview of the font to the user.
  const gfx::FontList* GetLabelFontListAt(size_t index) const override {
    return new gfx::FontList(static_cast<ReadAnythingFontModel*>(GetModel())
                                 ->GetLabelFontListAt(index));
  }
};

ReadAnythingFontCombobox::ReadAnythingFontCombobox(
    ReadAnythingFontCombobox::Delegate* delegate)
    : Combobox(std::move(delegate->GetFontComboboxModel())),
      delegate_(std::move(delegate)) {
  SetTooltipTextAndAccessibleName(
      l10n_util::GetStringUTF16(IDS_READ_ANYTHING_FONT_NAME_COMBOBOX_LABEL));
  SetCallback(
      base::BindRepeating(&ReadAnythingFontCombobox::FontNameChangedCallback,
                          weak_pointer_factory_.GetWeakPtr()));

  std::unique_ptr<ComboboxMenuModel> new_model =
      std::make_unique<MenuModel>(this, GetModel());

  SetMenuModel(std::move(new_model));
}

void ReadAnythingFontCombobox::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  Combobox::GetAccessibleNodeData(node_data);
  node_data->SetDescription(
      GetModel()->GetDropDownTextAt(GetSelectedIndex().value()));
}

void ReadAnythingFontCombobox::FontNameChangedCallback() {
  if (delegate_)
    delegate_->OnFontChoiceChanged(GetSelectedIndex().value());
}

ReadAnythingFontCombobox::~ReadAnythingFontCombobox() = default;

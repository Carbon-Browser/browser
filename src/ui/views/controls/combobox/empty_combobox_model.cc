// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/combobox/empty_combobox_model.h"

#include <string>

#include "base/notreached.h"

namespace views {
namespace internal {

EmptyComboboxModel::EmptyComboboxModel() = default;
EmptyComboboxModel::~EmptyComboboxModel() = default;

size_t EmptyComboboxModel::GetItemCount() const {
  return 0;
}

std::u16string EmptyComboboxModel::GetItemAt(size_t index) const {
  NOTREACHED();
  return std::u16string();
}

absl::optional<size_t> EmptyComboboxModel::GetDefaultIndex() const {
  return absl::nullopt;
}

}  // namespace internal
}  // namespace views

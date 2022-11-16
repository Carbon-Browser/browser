// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/crosapi/input_method_test_interface_ash.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/ash/ime_bridge.h"
#include "ui/base/ime/ash/input_method_ash.h"

namespace crosapi {
namespace {

ui::InputMethodAsh* GetInputMethod() {
  const ui::IMEBridge* bridge = ui::IMEBridge::Get();
  if (!bridge)
    return nullptr;

  ui::IMEInputContextHandlerInterface* handler =
      bridge->GetInputContextHandler();
  if (!handler)
    return nullptr;

  // Guaranteed to be an ui::InputMethodAsh*.
  return static_cast<ui::InputMethodAsh*>(handler->GetInputMethod());
}

}  // namespace

InputMethodTestInterfaceAsh::InputMethodTestInterfaceAsh()
    : input_method_(GetInputMethod()) {
  DCHECK(input_method_);
  input_method_observation_.Observe(input_method_);
}

InputMethodTestInterfaceAsh::~InputMethodTestInterfaceAsh() = default;

void InputMethodTestInterfaceAsh::WaitForFocus(WaitForFocusCallback callback) {
  // If `GetTextInputClient` is not null, then it's already focused.
  if (input_method_->GetTextInputClient()) {
    std::move(callback).Run();
    return;
  }

  // `callback` is assumed to outlive this class.
  focus_callbacks_.AddUnsafe(std::move(callback));
}

void InputMethodTestInterfaceAsh::CommitText(const std::string& text,
                                             CommitTextCallback callback) {
  input_method_->CommitText(
      base::UTF8ToUTF16(text),
      ui::TextInputClient::InsertTextCursorBehavior::kMoveCursorAfterText);
  std::move(callback).Run();
}

void InputMethodTestInterfaceAsh::OnTextInputStateChanged(
    const ui::TextInputClient* client) {
  // Focus is actually propagated via OnTextInputStateChanged, not
  // OnFocus/OnBlur (which are only used for unit tests).
  if (!client)
    return;

  focus_callbacks_.Notify();
}

}  // namespace crosapi

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/launcher/glic_background_mode_manager.h"

#include <memory>

#include "base/check.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/glic/launcher/glic_controller.h"
#include "chrome/browser/glic/launcher/glic_launcher_configuration.h"
#include "chrome/browser/glic/launcher/glic_status_icon.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/global_accelerator_listener/global_accelerator_listener.h"

namespace glic {

GlicBackgroundModeManager::GlicBackgroundModeManager(StatusTray* status_tray)
    : configuration_(std::make_unique<GlicLauncherConfiguration>(this)),
      controller_(std::make_unique<GlicController>()),
      status_tray_(status_tray),
      enabled_(configuration_->IsEnabled()),
      expected_registered_hotkey_(configuration_->GetGlobalHotkey()) {
  UpdateState();
}

GlicBackgroundModeManager::~GlicBackgroundModeManager() = default;

void GlicBackgroundModeManager::OnEnabledChanged(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }

  enabled_ = enabled;
  UpdateState();
  EnableLaunchOnStartup(enabled_);
}

void GlicBackgroundModeManager::OnGlobalHotkeyChanged(ui::Accelerator hotkey) {
  if (expected_registered_hotkey_ == hotkey) {
    return;
  }

  expected_registered_hotkey_ = hotkey;
  UpdateState();
}

void GlicBackgroundModeManager::OnKeyPressed(
    const ui::Accelerator& accelerator) {
  CHECK(accelerator == actual_registered_hotkey_);
  CHECK(actual_registered_hotkey_ == expected_registered_hotkey_);
  controller_->Show();
}

void GlicBackgroundModeManager::ExecuteCommand(
    const std::string& accelerator_group_id,
    const std::string& command_id) {
  // TODO(crbug.com/385194502): Handle Linux.
}

void GlicBackgroundModeManager::EnterBackgroundMode() {
  if (!keep_alive_) {
    keep_alive_ = std::make_unique<ScopedKeepAlive>(
        KeepAliveOrigin::GLIC_LAUNCHER, KeepAliveRestartOption::ENABLED);
  }

  if (!status_icon_) {
    status_icon_ =
        std::make_unique<GlicStatusIcon>(controller_.get(), status_tray_);
  }
}

void GlicBackgroundModeManager::ExitBackgroundMode() {
  status_icon_.reset();
  keep_alive_.reset();
}

void GlicBackgroundModeManager::EnableLaunchOnStartup(bool should_launch) {
  // TODO(crbug.com/378140958): Implement function
}

void GlicBackgroundModeManager::RegisterHotkey(ui::Accelerator updated_hotkey) {
  CHECK(!updated_hotkey.IsEmpty());
  auto* const global_accelerator_listener =
      ui::GlobalAcceleratorListener::GetInstance();
  if (global_accelerator_listener &&
      global_accelerator_listener->RegisterAccelerator(updated_hotkey, this)) {
    actual_registered_hotkey_ = updated_hotkey;
  }
}

void GlicBackgroundModeManager::UnregisterHotkey() {
  auto* const global_accelerator_listener =
      ui::GlobalAcceleratorListener::GetInstance();
  if (global_accelerator_listener && !actual_registered_hotkey_.IsEmpty()) {
    global_accelerator_listener->UnregisterAccelerator(
        actual_registered_hotkey_, this);
  }
  actual_registered_hotkey_ = ui::Accelerator();
}

void GlicBackgroundModeManager::UpdateState() {
  UnregisterHotkey();
  if (enabled_) {
    EnterBackgroundMode();
    if (!expected_registered_hotkey_.IsEmpty()) {
      RegisterHotkey(expected_registered_hotkey_);
    }
  } else {
    ExitBackgroundMode();
  }

  if (status_icon_) {
    status_icon_->UpdateHotkey(actual_registered_hotkey_);
  }
}

}  // namespace glic

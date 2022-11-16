// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/system_extensions/api/window_management/window_management_impl.h"

#include <utility>

#include "ash/wm/window_state.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "chrome/browser/apps/app_service/app_service_proxy.h"
#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/services/app_service/public/cpp/instance.h"
#include "components/services/app_service/public/mojom/types.mojom-shared.h"
#include "components/services/app_service/public/mojom/types.mojom.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "third_party/blink/public/mojom/chromeos/system_extensions/window_management/cros_window_management.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/env.h"
#include "ui/base/ui_base_types.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

WindowManagementImpl::WindowManagementImpl(
    int32_t render_process_host_id,
    mojo::PendingAssociatedRemote<
        blink::mojom::CrosWindowManagementStartObserver>
        pending_associated_remote)
    : render_process_host_id_(render_process_host_id),
      observer_(std::move(pending_associated_remote)) {
  // Let WindowManagementImpl be a PreTargetHandler on aura::Env to ensure that
  // it receives KeyEvents.
  aura::Env::GetInstance()->AddPreTargetHandler(
      this, ui::EventTarget::Priority::kAccessibility);
}

WindowManagementImpl::~WindowManagementImpl() {
  aura::Env::GetInstance()->RemovePreTargetHandler(this);
}

void WindowManagementImpl::DispatchStartEvent() {
  observer_->DispatchStartEvent();
}

void WindowManagementImpl::OnKeyEvent(ui::KeyEvent* event) {
  DCHECK(event->type() == ui::EventType::ET_KEY_PRESSED ||
         event->type() == ui::EventType::ET_KEY_RELEASED);
  // TODO(b/238578914): Eventually we will allow System Extensions to register
  // their accelerators, but for prototyping, we just send any key press with a
  // modifier.
  std::vector<std::string> keys;
  if (event->IsControlDown()) {
    keys.push_back(
        ui::KeycodeConverter::DomKeyToKeyString(ui::DomKey::CONTROL));
  }
  if (event->IsAltDown()) {
    keys.push_back(ui::KeycodeConverter::DomKeyToKeyString(ui::DomKey::ALT));
  }

  // No modifiers pressed.
  if (keys.size() == 0)
    return;

  // Only modifiers pressed.
  const std::string key =
      ui::KeycodeConverter::DomKeyToKeyString(event->GetDomKey());
  if (base::Contains(keys, key)) {
    return;
  }
  keys.push_back(key);

  blink::mojom::AcceleratorEventPtr event_ptr =
      blink::mojom::AcceleratorEvent::New();
  event_ptr->type = event->type() == ui::EventType::ET_KEY_PRESSED
                        ? blink::mojom::AcceleratorEvent::Type::kDown
                        : blink::mojom::AcceleratorEvent::Type::kUp;
  event_ptr->accelerator_name = base::JoinString(keys, " ");
  event_ptr->repeat = event->is_repeat();

  observer_->DispatchAcceleratorEvent(std::move(event_ptr));
}

void WindowManagementImpl::GetAllWindows(GetAllWindowsCallback callback) {
  std::vector<blink::mojom::CrosWindowInfoPtr> windows;

  Profile* profile = GetProfile();
  if (!profile) {
    std::move(callback).Run(std::move(windows));
    return;
  }

  apps::AppServiceProxy* proxy =
      apps::AppServiceProxyFactory::GetForProfile(profile);
  proxy->InstanceRegistry().ForEachInstance(
      [&windows](const apps::InstanceUpdate& update) {
        auto window = blink::mojom::CrosWindowInfo::New();
        aura::Window* target = update.Window()->GetToplevelWindow();
        views::Widget* widget =
            views::Widget::GetTopLevelWidgetForNativeView(target);
        if (!target || !widget) {
          return;
        }
        window->id = update.InstanceId();
        window->title =
            base::UTF16ToUTF8(widget->widget_delegate()->GetWindowTitle());
        window->app_id = update.AppId();
        window->bounds = target->bounds();

        // Set window state (states are mutually exclusive)
        if (widget->IsFullscreen()) {
          window->window_state = blink::mojom::WindowState::kFullscreen;
        } else if (widget->IsMaximized()) {
          window->window_state = blink::mojom::WindowState::kMaximized;
        } else if (widget->IsMinimized()) {
          window->window_state = blink::mojom::WindowState::kMinimized;
        } else {
          window->window_state = blink::mojom::WindowState::kNormal;
        }
        // Instance registry references the activatable component of a window
        // which itself does not have focus but contains the child focusable. To
        // detect focus on the window, we assert that the focused window has our
        // activatable as its top level parent
        window->is_focused = target == aura::client::GetFocusClient(target)
                                           ->GetFocusedWindow()
                                           ->GetToplevelWindow();
        window->visibility_state = widget->IsVisible()
                                       ? blink::mojom::VisibilityState::kShown
                                       : blink::mojom::VisibilityState::kHidden;
        windows.push_back(std::move(window));
      });
  std::move(callback).Run(std::move(windows));
}

void WindowManagementImpl::MoveTo(const base::UnguessableToken& id,
                                  int32_t x,
                                  int32_t y,
                                  MoveToCallback callback) {
  aura::Window* target = GetWindow(id);
  // TODO(crbug.com/1253318): Ensure this works with multiple screens.
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  target->SetBounds(
      gfx::Rect(x, y, target->bounds().width(), target->bounds().height()));
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::MoveBy(const base::UnguessableToken& id,
                                  int32_t delta_x,
                                  int32_t delta_y,
                                  MoveByCallback callback) {
  aura::Window* target = GetWindow(id);
  // TODO(crbug.com/1253318): Ensure this works with multiple screens.
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  target->SetBounds(
      gfx::Rect(target->bounds().x() + delta_x, target->bounds().y() + delta_y,
                target->bounds().width(), target->bounds().height()));
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::ResizeTo(const base::UnguessableToken& id,
                                    int32_t width,
                                    int32_t height,
                                    ResizeToCallback callback) {
  aura::Window* target = GetWindow(id);
  // TODO(crbug.com/1253318): Ensure this works with multiple screens.
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  target->SetBounds(
      gfx::Rect(target->bounds().x(), target->bounds().y(), width, height));
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::ResizeBy(const base::UnguessableToken& id,
                                    int32_t delta_width,
                                    int32_t delta_height,
                                    ResizeByCallback callback) {
  aura::Window* target = GetWindow(id);
  // TODO(crbug.com/1253318): Ensure this works with multiple screens.
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  target->SetBounds(gfx::Rect(target->bounds().x(), target->bounds().y(),
                              target->bounds().width() + delta_width,
                              target->bounds().height() + delta_height));
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::SetFullscreen(const base::UnguessableToken& id,
                                         bool fullscreen,
                                         SetFullscreenCallback callback) {
  views::Widget* widget = GetWidget(id);
  if (!widget) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNoWidget);
    return;
  }
  widget->SetFullscreen(fullscreen);
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::Maximize(const base::UnguessableToken& id,
                                    MaximizeCallback callback) {
  aura::Window* target = GetWindow(id);
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  // Returns null when id points to non top level window or is null itself.
  WindowState* state = WindowState::Get(target);
  if (!state) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNoWindowState);
    return;
  }

  state->Maximize();
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::Minimize(const base::UnguessableToken& id,
                                    MinimizeCallback callback) {
  aura::Window* target = GetWindow(id);
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }

  // Returns null when id points to non top level window or is null itself.
  WindowState* state = WindowState::Get(target);
  if (!state) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNoWindowState);
    return;
  }

  state->Minimize();
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::Focus(const base::UnguessableToken& id,
                                 FocusCallback callback) {
  aura::Window* target = GetWindow(id);
  if (!target) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNotFound);
    return;
  }
  target->Focus();
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

void WindowManagementImpl::Close(const base::UnguessableToken& id,
                                 CloseCallback callback) {
  views::Widget* widget = GetWidget(id);
  if (!widget) {
    std::move(callback).Run(
        blink::mojom::CrosWindowManagementStatus::kWindowNoWidget);
    return;
  }
  widget->Close();
  // TODO(crbug.com/232703960): Scope into close function and refactor for
  // error handling.
  std::move(callback).Run(blink::mojom::CrosWindowManagementStatus::kSuccess);
}

Profile* WindowManagementImpl::GetProfile() {
  content::RenderProcessHost* render_process_host =
      content::RenderProcessHost::FromID(render_process_host_id_);
  if (!render_process_host)
    return nullptr;

  return Profile::FromBrowserContext(render_process_host->GetBrowserContext());
}

aura::Window* WindowManagementImpl::GetWindow(
    const base::UnguessableToken& id) {
  aura::Window* target = nullptr;

  Profile* profile = GetProfile();
  if (!profile) {
    return nullptr;
  }

  apps::AppServiceProxy* proxy =
      apps::AppServiceProxyFactory::GetForProfile(profile);
  proxy->InstanceRegistry().ForOneInstance(
      id, [&target](const apps::InstanceUpdate& update) {
        target = update.Window()->GetToplevelWindow();
      });

  return target;
}

views::Widget* WindowManagementImpl::GetWidget(
    const base::UnguessableToken& id) {
  aura::Window* target = GetWindow(id);

  if (!target) {
    return nullptr;
  }

  views::Widget* widget = views::Widget::GetTopLevelWidgetForNativeView(target);
  return widget;
}

void WindowManagementImpl::GetAllScreens(GetAllScreensCallback callback) {
  std::vector<blink::mojom::CrosScreenInfoPtr> screens;

  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    auto screen = blink::mojom::CrosScreenInfo::New();
    screen->work_area = display.work_area();
    screen->bounds = display.bounds();
    screen->is_primary =
        display.id() == display::Screen::GetScreen()->GetPrimaryDisplay().id();
    screens.push_back(std::move(screen));
  }

  std::move(callback).Run(std::move(screens));
}

}  // namespace ash

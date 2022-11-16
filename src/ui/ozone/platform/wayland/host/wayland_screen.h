// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SCREEN_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SCREEN_H_

#include <set>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/display/display_list.h"
#include "ui/display/display_observer.h"
#include "ui/display/tablet_state.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/geometry/point.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"
#include "ui/ozone/public/platform_screen.h"

namespace gfx {
class Rect;
}

namespace ui {

class WaylandConnection;

#if defined(USE_DBUS)
class OrgGnomeMutterIdleMonitor;
#endif

// A PlatformScreen implementation for Wayland.
class WaylandScreen : public PlatformScreen {
 public:
  explicit WaylandScreen(WaylandConnection* connection);
  WaylandScreen(const WaylandScreen&) = delete;
  WaylandScreen& operator=(const WaylandScreen&) = delete;
  ~WaylandScreen() override;

  void OnOutputAddedOrUpdated(uint32_t output_id,
                              const gfx::Point& origin,
                              const gfx::Size& logical_size,
                              const gfx::Size& physical_size,
                              const gfx::Insets& insets,
                              float scale,
                              int32_t panel_transform,
                              int32_t logical_transform,
                              const std::string& label);
  void OnOutputRemoved(uint32_t output_id);

  void OnTabletStateChanged(display::TabletState tablet_state);

  base::WeakPtr<WaylandScreen> GetWeakPtr();

  // PlatformScreen overrides:
  const std::vector<display::Display>& GetAllDisplays() const override;
  display::Display GetPrimaryDisplay() const override;
  display::Display GetDisplayForAcceleratedWidget(
      gfx::AcceleratedWidget widget) const override;
  gfx::Point GetCursorScreenPoint() const override;
  gfx::AcceleratedWidget GetAcceleratedWidgetAtScreenPoint(
      const gfx::Point& point) const override;
  gfx::AcceleratedWidget GetLocalProcessWidgetAtPoint(
      const gfx::Point& point,
      const std::set<gfx::AcceleratedWidget>& ignore) const override;
  display::Display GetDisplayNearestPoint(
      const gfx::Point& point) const override;
  display::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const override;
  std::unique_ptr<PlatformScreen::PlatformScreenSaverSuspender>
  SuspendScreenSaver() override;
  bool IsScreenSaverActive() const override;
  base::TimeDelta CalculateIdleTime() const override;
  void AddObserver(display::DisplayObserver* observer) override;
  void RemoveObserver(display::DisplayObserver* observer) override;
  base::Value::List GetGpuExtraInfo(
      const gfx::GpuExtraInfo& gpu_extra_info) override;

 protected:
  // Suspends or un-suspends the platform-specific screensaver, and returns
  // whether the operation was successful. Can be called more than once with the
  // same value for |suspend|, but those states should not stack: the first
  // alternating value should toggle the state of the suspend.
  bool SetScreenSaverSuspended(bool suspend);

 private:
  class WaylandScreenSaverSuspender
      : public PlatformScreen::PlatformScreenSaverSuspender {
   public:
    WaylandScreenSaverSuspender(const WaylandScreenSaverSuspender&) = delete;
    WaylandScreenSaverSuspender& operator=(const WaylandScreenSaverSuspender&) =
        delete;

    ~WaylandScreenSaverSuspender() override;

    static std::unique_ptr<WaylandScreenSaverSuspender> Create(
        WaylandScreen& screen);

   private:
    explicit WaylandScreenSaverSuspender(WaylandScreen& screen);

    base::WeakPtr<WaylandScreen> screen_;
    bool is_suspending_ = false;
  };

  // All parameters are in DIP screen coordinates/units except |physical_size|,
  // which is in physical pixels.
  void AddOrUpdateDisplay(uint32_t output_id,
                          const gfx::Point& origin,
                          const gfx::Size& logical_size,
                          const gfx::Size& physical_size,
                          const gfx::Insets& insets,
                          float scale,
                          int32_t panel_transform,
                          int32_t logical_transform,
                          const std::string& label);

  raw_ptr<WaylandConnection> connection_ = nullptr;

  display::DisplayList display_list_;

  base::ObserverList<display::DisplayObserver> observers_;

  absl::optional<gfx::BufferFormat> image_format_alpha_;
  absl::optional<gfx::BufferFormat> image_format_no_alpha_;
  absl::optional<gfx::BufferFormat> image_format_hdr_;

#if defined(USE_DBUS)
  mutable std::unique_ptr<OrgGnomeMutterIdleMonitor>
      org_gnome_mutter_idle_monitor_;
#endif

  wl::Object<zwp_idle_inhibitor_v1> idle_inhibitor_;
  uint32_t screen_saver_suspension_count_ = 0;

  base::WeakPtrFactory<WaylandScreen> weak_factory_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SCREEN_H_

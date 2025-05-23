// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/wayland_subsurface.h"

#include <cstdint>

#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/ozone/platform/wayland/common/wayland_util.h"
#include "ui/ozone/platform/wayland/host/wayland_buffer_manager_host.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace {

// Returns DIP bounds of the subsurface relative to the parent surface.
gfx::RectF AdjustSubsurfaceBounds(const gfx::RectF& bounds_px,
                                  const gfx::RectF& parent_bounds_px,
                                  float buffer_scale) {
  const auto bounds_dip = gfx::ScaleRect(bounds_px, 1.0f / buffer_scale);
  const auto parent_bounds_dip =
      gfx::ScaleRect(parent_bounds_px, 1.0f / buffer_scale);
  return wl::TranslateBoundsToParentCoordinatesF(bounds_dip, parent_bounds_dip);
}

const wl_fixed_t kMinusOne = wl_fixed_from_int(-1);

}  // namespace

namespace ui {

WaylandSubsurface::WaylandSubsurface(WaylandConnection* connection,
                                     WaylandWindow* parent)
    : wayland_surface_(connection, parent),
      connection_(connection),
      parent_(parent) {
  DCHECK(parent_);
  DCHECK(connection_);
  if (!surface()) {
    LOG(ERROR) << "Failed to create wl_surface";
    return;
  }
  wayland_surface_.Initialize();
}

WaylandSubsurface::~WaylandSubsurface() = default;

gfx::AcceleratedWidget WaylandSubsurface::GetWidget() const {
  return wayland_surface_.get_widget();
}

bool WaylandSubsurface::Show() {
  if (visible_) {
    return false;
  }

  if (subsurface_) {
    ResetSubsurface();
  }

  CreateSubsurface();
  visible_ = true;
  return true;
}

void WaylandSubsurface::Hide() {
  if (!IsVisible() || !subsurface_) {
    return;
  }

  // Remove it from the stack.
  RemoveFromList();
  visible_ = false;
}

void WaylandSubsurface::ResetSubsurface() {
  subsurface_.reset();
  wayland_surface_.UnsetRootWindow();
}

bool WaylandSubsurface::IsVisible() const {
  return visible_;
}

void WaylandSubsurface::CreateSubsurface() {
  DCHECK(parent_);
  wayland_surface_.SetRootWindow(parent_);

  wl_subcompositor* subcompositor = connection_->subcompositor();
  DCHECK(subcompositor);
  subsurface_ = wayland_surface()->CreateSubsurface(parent_->root_surface());
  position_dip_ = {0, 0};

  // A new sub-surface is initially added as the top-most in the stack.
  parent_->subsurface_stack_committed()->Append(this);

  DCHECK(subsurface_);
  wl_subsurface_set_sync(subsurface_.get());

  // Subsurfaces don't need to trap input events. Its display rect is fully
  // contained in |parent_|'s. Setting input_region to empty allows |parent_| to
  // dispatch all of the input to platform window.
  const std::vector<gfx::Rect> kEmptyRegionPx{{}};
  wayland_surface()->set_input_region(kEmptyRegionPx);
}

bool WaylandSubsurface::ConfigureAndShowSurface(
    const gfx::RectF& bounds_px,
    const gfx::RectF& parent_bounds_px,
    float buffer_scale,
    WaylandSubsurface* new_below,
    WaylandSubsurface* new_above) {
  bool needs_commit = Show();

  // Chromium positions quads in display::Display coordinates in physical
  // pixels, but Wayland requires them to be in local surface coordinates a.k.a
  // relative to parent window.
  auto bounds_dip_in_parent_surface =
      AdjustSubsurfaceBounds(bounds_px, parent_bounds_px, buffer_scale);
  if (bounds_dip_in_parent_surface.origin() != position_dip_) {
    position_dip_ = bounds_dip_in_parent_surface.origin();
    gfx::Point origin_in_parent =
        gfx::ToCeiledPoint(bounds_dip_in_parent_surface.origin());
    wl_subsurface_set_position(subsurface_.get(), origin_in_parent.x(),
                               origin_in_parent.y());
    // TODO(crbug.com/40946960): This commit might not be needed. Changes to the
    // position depend on the sync mode of the parent surface.
    needs_commit = true;
  }

  // Setup the stacking order of this subsurface.
  DCHECK(!new_above || !new_below);
  if (new_below && new_below != previous()) {
    DCHECK_EQ(parent_, new_below->parent_);
    RemoveFromList();
    InsertAfter(new_below);
    wl_subsurface_place_above(subsurface_.get(), new_below->surface());
    // TODO(crbug.com/40946960): This commit might not be needed. Changes to the
    // stacking order depend on the sync mode of the parent surface.
    needs_commit = true;
  } else if (new_above && new_above != next()) {
    DCHECK_EQ(parent_, new_above->parent_);
    RemoveFromList();
    InsertBefore(new_above);
    wl_subsurface_place_below(subsurface_.get(), new_above->surface());
    // TODO(crbug.com/40946960): This commit might not be needed. Changes to the
    // stacking order depend on the sync mode of the parent surface.
    needs_commit = true;
  }

  return needs_commit;
}

}  // namespace ui

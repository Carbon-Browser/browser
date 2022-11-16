// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_container.h"

#include "base/bits.h"
#include "base/containers/adapters.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_group_header.h"
#include "chrome/browser/ui/views/tabs/tab_group_highlight.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/browser/ui/views/tabs/z_orderable_tab_container_element.h"
#include "chrome/grit/theme_resources.h"
#include "components/tab_groups/tab_group_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/display/screen.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/mouse_watcher_view_host.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/view_utils.h"

namespace {

// Size of the drop indicator.
int g_drop_indicator_width = 0;
int g_drop_indicator_height = 0;

gfx::ImageSkia* GetDropArrowImage(bool is_down) {
  return ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      is_down ? IDR_TAB_DROP_DOWN : IDR_TAB_DROP_UP);
}

// Provides the ability to monitor when a tab's bounds have been animated. Used
// to hook callbacks to adjust things like tabstrip preferred size and tab group
// underlines.
class TabSlotAnimationDelegate : public gfx::AnimationDelegate {
 public:
  TabSlotAnimationDelegate(TabContainer* tab_container, TabSlotView* slot_view);
  TabSlotAnimationDelegate(const TabSlotAnimationDelegate&) = delete;
  TabSlotAnimationDelegate& operator=(const TabSlotAnimationDelegate&) = delete;
  ~TabSlotAnimationDelegate() override;

  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 protected:
  TabContainer* tab_container() { return tab_container_; }
  TabSlotView* slot_view() { return slot_view_; }

 private:
  const raw_ptr<TabContainer> tab_container_;
  const raw_ptr<TabSlotView> slot_view_;
};

TabSlotAnimationDelegate::TabSlotAnimationDelegate(TabContainer* tab_container,
                                                   TabSlotView* slot_view)
    : tab_container_(tab_container), slot_view_(slot_view) {
  slot_view_->set_animating(true);
}

TabSlotAnimationDelegate::~TabSlotAnimationDelegate() = default;

void TabSlotAnimationDelegate::AnimationProgressed(
    const gfx::Animation* animation) {
  tab_container_->OnTabSlotAnimationProgressed(slot_view_);
}

void TabSlotAnimationDelegate::AnimationEnded(const gfx::Animation* animation) {
  slot_view_->set_animating(false);
  AnimationProgressed(animation);
  slot_view_->Layout();
}

void TabSlotAnimationDelegate::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

// Helper class that manages the tab scrolling animation.
class TabScrollingAnimation : public gfx::LinearAnimation,
                              public gfx::AnimationDelegate {
 public:
  explicit TabScrollingAnimation(
      views::View* contents_view,
      gfx::AnimationContainer* bounds_animator_container,
      base::TimeDelta duration,
      const gfx::Rect start_visible_rect,
      const gfx::Rect end_visible_rect)
      : gfx::LinearAnimation(duration,
                             gfx::LinearAnimation::kDefaultFrameRate,
                             this),
        contents_view_(contents_view),
        start_visible_rect_(start_visible_rect),
        end_visible_rect_(end_visible_rect) {
    SetContainer(bounds_animator_container);
  }
  TabScrollingAnimation(const TabScrollingAnimation&) = delete;
  TabScrollingAnimation& operator=(const TabScrollingAnimation&) = delete;
  ~TabScrollingAnimation() override = default;

  void AnimateToState(double state) override {
    gfx::Rect intermediary_rect(
        start_visible_rect_.x() +
            (end_visible_rect_.x() - start_visible_rect_.x()) * state,
        start_visible_rect_.y(), start_visible_rect_.width(),
        start_visible_rect_.height());

    contents_view_->ScrollRectToVisible(intermediary_rect);
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    contents_view_->ScrollRectToVisible(end_visible_rect_);
  }

 private:
  const raw_ptr<views::View> contents_view_;
  const gfx::Rect start_visible_rect_;
  const gfx::Rect end_visible_rect_;
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// TabContainer::RemoveTabDelegate
//
// AnimationDelegate used when removing a tab. Does the necessary cleanup when
// done.
class TabContainer::RemoveTabDelegate : public TabSlotAnimationDelegate {
 public:
  RemoveTabDelegate(TabContainer* tab_container, Tab* tab);
  RemoveTabDelegate(const RemoveTabDelegate&) = delete;
  RemoveTabDelegate& operator=(const RemoveTabDelegate&) = delete;

  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;
};

TabContainer::RemoveTabDelegate::RemoveTabDelegate(TabContainer* tab_container,
                                                   Tab* tab)
    : TabSlotAnimationDelegate(tab_container, tab) {}

void TabContainer::RemoveTabDelegate::AnimationEnded(
    const gfx::Animation* animation) {
  tab_container()->OnTabCloseAnimationCompleted(static_cast<Tab*>(slot_view()));
}

void TabContainer::RemoveTabDelegate::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

TabContainer::TabContainer(TabStripController* controller,
                           TabHoverCardController* hover_card_controller,
                           TabDragContextBase* drag_context,
                           TabSlotController* tab_slot_controller,
                           views::View* scroll_contents_view)
    : controller_(controller),
      hover_card_controller_(hover_card_controller),
      drag_context_(drag_context),
      tab_slot_controller_(tab_slot_controller),
      scroll_contents_view_(scroll_contents_view),
      bounds_animator_(this),
      layout_helper_(std::make_unique<TabStripLayoutHelper>(
          controller,
          base::BindRepeating(&TabContainer::tabs_view_model,
                              base::Unretained(this)))) {
  SetEventTargeter(std::make_unique<views::ViewTargeter>(this));

  if (!gfx::Animation::ShouldRenderRichAnimation())
    bounds_animator().SetAnimationDuration(base::TimeDelta());

  bounds_animator().AddObserver(this);

  if (g_drop_indicator_width == 0) {
    // Direction doesn't matter, both images are the same size.
    gfx::ImageSkia* drop_image = GetDropArrowImage(true);
    g_drop_indicator_width = drop_image->width();
    g_drop_indicator_height = drop_image->height();
  }
}

TabContainer::~TabContainer() {
  // The animations may reference the tabs or group views. Shut down the
  // animation before we destroy any animated views.
  CancelAnimation();

  // Since TabGroupViews expects be able to remove the views it creates, clear
  // |group_views_| before removing the remaining children below.
  group_views_.clear();

  // Make sure we unhook ourselves as a message loop observer so that we don't
  // crash in the case where the user closes the window after closing a tab
  // but before moving the mouse.
  RemoveMessageLoopObserver();

  RemoveAllChildViews();
}

void TabContainer::SetAvailableWidthCallback(
    base::RepeatingCallback<int()> available_width_callback) {
  available_width_callback_ = available_width_callback;
}

Tab* TabContainer::AddTab(std::unique_ptr<Tab> tab,
                          int model_index,
                          TabPinned pinned) {
  Tab* tab_ptr = AddChildView(std::move(tab));
  tabs_view_model_.Add(tab_ptr, model_index);
  layout_helper_->InsertTabAt(model_index, tab_ptr, pinned);
  OrderTabSlotView(tab_ptr);

  // Don't animate the first tab, it looks weird, and don't animate anything
  // if the containing window isn't visible yet.
  if (GetTabCount() > 1 && GetWidget() && GetWidget()->IsVisible()) {
    StartInsertTabAnimation(model_index);
  } else {
    CompleteAnimationAndLayout();
  }

  UpdateAccessibleTabIndices();

  return tab_ptr;
}

void TabContainer::MoveTab(int from_model_index, int to_model_index) {
  Tab* tab = GetTabAtModelIndex(from_model_index);
  tabs_view_model_.Move(from_model_index, to_model_index);
  layout_helper_->MoveTab(tab->group(), from_model_index, to_model_index);
  OrderTabSlotView(tab);

  layout_helper()->SetTabPinned(to_model_index, tab->data().pinned
                                                    ? TabPinned::kPinned
                                                    : TabPinned::kUnpinned);

  StartBasicAnimation();

  UpdateAccessibleTabIndices();
}

void TabContainer::RemoveTab(int model_index, bool was_active) {
  UpdateClosingModeOnRemovedTab(model_index, was_active);

  Tab* tab = GetTabAtModelIndex(model_index);
  tab->SetClosing(true);

  RemoveTabFromViewModel(model_index);

  StartRemoveTabAnimation(tab, model_index);

  UpdateAccessibleTabIndices();
}

void TabContainer::SetTabPinned(int model_index, TabPinned pinned) {
  layout_helper()->SetTabPinned(model_index, pinned);

  if (GetWidget() && GetWidget()->IsVisible()) {
    ExitTabClosingMode();

    StartBasicAnimation();
  } else {
    CompleteAnimationAndLayout();
  }
}

void TabContainer::StoppedDraggingView(TabSlotView* view) {
  AddChildView(view);
  Tab* tab = views::AsViewClass<Tab>(view);
  if (tab && tab->closing()) {
    // This tab was closed during the drag. It's already been removed from our
    // other data structures in RemoveTab(), and TabDragContext animated it
    // closed for us, so we can just destroy it.
    OnTabCloseAnimationCompleted(tab);
    return;
  }
  OrderTabSlotView(view);

  if (view->group())
    UpdateTabGroupVisuals(view->group().value());
}

void TabContainer::ScrollTabToVisible(int model_index) {
  absl::optional<gfx::Rect> visible_content_rect = GetVisibleContentRect();

  if (!visible_content_rect.has_value())
    return;

  // If the tab strip won't be scrollable after the current tabstrip animations
  // complete, scroll animation wouldn't be meaningful.
  if (tabs_view_model_.ideal_bounds(GetTabCount() - 1).right() <=
      GetAvailableWidthForTabContainer())
    return;

  gfx::Rect active_tab_ideal_bounds =
      tabs_view_model_.ideal_bounds(model_index);

  if ((active_tab_ideal_bounds.x() >= visible_content_rect->x()) &&
      (active_tab_ideal_bounds.right() <= visible_content_rect->right())) {
    return;
  }

  bool scroll_left = active_tab_ideal_bounds.x() < visible_content_rect->x();
  if (scroll_left) {
    // Scroll the left edge of |visible_content_rect| to show the left edge of
    // the tab at |model_index|. We can leave the width entirely up to the
    // ScrollView.
    int start_left_edge(visible_content_rect->x());
    int target_left_edge(active_tab_ideal_bounds.x());

    AnimateScrollToShowXCoordinate(start_left_edge, target_left_edge);
  } else {
    // Scroll the right edge of |visible_content_rect| to show the right edge
    // of the tab at |model_index|. We can leave the width entirely up to the
    // ScrollView.
    int start_right_edge(visible_content_rect->right());
    int target_right_edge(active_tab_ideal_bounds.right());
    AnimateScrollToShowXCoordinate(start_right_edge, target_right_edge);
  }
}

void TabContainer::ScrollTabContainerByOffset(int offset) {
  absl::optional<gfx::Rect> visible_content_rect = GetVisibleContentRect();
  if (!visible_content_rect.has_value() || offset == 0)
    return;

  // If tabcontainer is scrolled towards trailing tab, the start edge should
  // have the x coordinate of the right bound. If it is scrolled towards the
  // leading tab it should have the x coordinate of the left bound.
  int start_edge =
      (offset > 0) ? visible_content_rect->right() : visible_content_rect->x();

  AnimateScrollToShowXCoordinate(start_edge, start_edge + offset);
}

void TabContainer::AnimateScrollToShowXCoordinate(const int start_edge,
                                                  const int target_edge) {
  if (tab_scrolling_animation_)
    tab_scrolling_animation_->Stop();

  gfx::Rect start_rect(start_edge, 0, 0, 0);
  gfx::Rect target_rect(target_edge, 0, 0, 0);

  tab_scrolling_animation_ = std::make_unique<TabScrollingAnimation>(
      scroll_contents_view_, bounds_animator().container(),
      bounds_animator().GetAnimationDuration(), start_rect, target_rect);
  tab_scrolling_animation_->Start();
}

absl::optional<gfx::Rect> TabContainer::GetVisibleContentRect() {
  views::ScrollView* scroll_container =
      views::ScrollView::GetScrollViewForContents(scroll_contents_view_);
  if (!scroll_container)
    return absl::nullopt;

  return scroll_container->GetVisibleRect();
}

void TabContainer::OnGroupCreated(const tab_groups::TabGroupId& group) {
  auto group_view = std::make_unique<TabGroupViews>(
      this, drag_context_, tab_slot_controller_, group);
  layout_helper()->InsertGroupHeader(group, group_view->header());
  group_views()[group] = std::move(group_view);
}

void TabContainer::OnGroupEditorOpened(const tab_groups::TabGroupId& group) {
  // The context menu relies on a Browser object which is not provided in
  // TabStripTest.
  if (controller_->GetBrowser()) {
    group_views()[group]->header()->ShowContextMenuForViewImpl(
        this, gfx::Point(), ui::MENU_SOURCE_NONE);
  }
}

void TabContainer::OnGroupContentsChanged(const tab_groups::TabGroupId& group) {
  // If a tab was removed, the underline bounds might be stale.
  group_views()[group]->UpdateBounds();

  // The group header may be in the wrong place if the tab didn't actually
  // move in terms of model indices.
  OnGroupMoved(group);
  StartBasicAnimation();
}

void TabContainer::OnGroupMoved(const tab_groups::TabGroupId& group) {
  DCHECK(group_views()[group]);

  layout_helper()->UpdateGroupHeaderIndex(group);

  OrderTabSlotView(group_views()[group]->header());
}

void TabContainer::OnGroupClosed(const tab_groups::TabGroupId& group) {
  bounds_animator().StopAnimatingView(group_views().at(group).get()->header());
  layout_helper()->RemoveGroupHeader(group);
  group_views().erase(group);

  StartBasicAnimation();
}

void TabContainer::UpdateTabGroupVisuals(tab_groups::TabGroupId group_id) {
  const auto group_views = group_views_.find(group_id);
  if (group_views != group_views_.end())
    group_views->second->UpdateBounds();
}

// TODO(pkasting): This should really return an optional<size_t>
int TabContainer::GetModelIndexOf(const TabSlotView* slot_view) const {
  const absl::optional<size_t> index =
      tabs_view_model_.GetIndexOfView(slot_view);
  return index.has_value() ? static_cast<int>(index.value()) : -1;
}

Tab* TabContainer::GetTabAtModelIndex(int index) const {
  return tabs_view_model_.view_at(index);
}

int TabContainer::GetTabCount() const {
  return tabs_view_model_.view_size();
}

void TabContainer::UpdateHoverCard(
    Tab* tab,
    TabSlotController::HoverCardUpdateType update_type) {
  // Some operations (including e.g. starting a drag) can cause the tab focus
  // to change at the same time as the tabstrip is starting to animate; the
  // hover card should not be visible at this time.
  // See crbug.com/1220840 for an example case.
  if (IsAnimating()) {
    tab = nullptr;
    update_type = TabSlotController::HoverCardUpdateType::kAnimating;
  }

  if (!hover_card_controller_)
    return;

  hover_card_controller_->UpdateHoverCard(tab, update_type);
}

void TabContainer::HandleLongTap(ui::GestureEvent* event) {
  event->target()->ConvertEventToTarget(this, event);
  gfx::Point local_point = event->location();
  Tab* tab = FindTabHitByPoint(local_point);
  if (tab) {
    ConvertPointToScreen(this, &local_point);
    controller_->ShowContextMenuForTab(tab, local_point, ui::MENU_SOURCE_TOUCH);
  }
}

bool TabContainer::IsRectInWindowCaption(const gfx::Rect& rect) {
  // If there is no control at this location, the hit is in the caption area.
  const views::View* v = GetEventHandlerForRect(rect);
  if (v == this)
    return true;

  // When the window has a top drag handle, a thin strip at the top of inactive
  // tabs and the new tab button is treated as part of the window drag handle,
  // to increase draggability.  This region starts 1 DIP above the top of the
  // separator.
  const int drag_handle_extension = TabStyle::GetDragHandleExtension(height());

  // Disable drag handle extension when tab shapes are visible.
  bool extend_drag_handle = !controller_->IsFrameCondensed() &&
                            !controller_->EverHasVisibleBackgroundTabShapes();

  // A hit on the tab is not in the caption unless it is in the thin strip
  // mentioned above.
  const absl::optional<size_t> tab_index = tabs_view_model_.GetIndexOfView(v);
  if (tab_index.has_value() && IsValidModelIndex(tab_index.value())) {
    Tab* tab = GetTabAtModelIndex(tab_index.value());
    gfx::Rect tab_drag_handle = tab->GetMirroredBounds();
    tab_drag_handle.set_height(drag_handle_extension);
    return extend_drag_handle && !tab->IsActive() &&
           tab_drag_handle.Intersects(rect);
  }

  // |v| is some other view (e.g. a close button in a tab) and therefore |rect|
  // is in client area.
  return false;
}

void TabContainer::OnTabSlotAnimationProgressed(TabSlotView* view) {
  // The rightmost tab moving might have changed the tab container's preferred
  // width.
  PreferredSizeChanged();
  if (view->group())
    UpdateTabGroupVisuals(view->group().value());
}

void TabContainer::StartBasicAnimation() {
  UpdateIdealBounds();
  AnimateToIdealBounds();
}

void TabContainer::InvalidateIdealBounds() {
  last_layout_size_ = gfx::Size();
}

bool TabContainer::IsAnimating() const {
  return bounds_animator_.IsAnimating() ||
         (drag_context_ && drag_context_->IsEndingDrag());
}

void TabContainer::CancelAnimation() {
  drag_context_->FinishEndingDrag();
  bounds_animator_.Cancel();
}

void TabContainer::CompleteAnimationAndLayout() {
  last_available_width_ = GetAvailableWidthForTabContainer();
  last_layout_size_ = size();

  CancelAnimation();

  UpdateIdealBounds();
  SnapToIdealBounds();

  SetTabSlotVisibility();
  SchedulePaint();
}

int TabContainer::GetAvailableWidthForTabContainer() const {
  // Falls back to views::View::GetAvailableSize() when
  // |available_width_callback_| is not defined, e.g. when tab scrolling is
  // disabled.
  return available_width_callback_
             ? available_width_callback_.Run()
             : parent()->GetAvailableSize(this).width().value();
}

void TabContainer::EnterTabClosingMode(absl::optional<int> override_width,
                                       CloseTabSource source) {
  in_tab_close_ = true;
  override_available_width_for_tabs_ = override_width;

  resize_layout_timer_.Stop();
  if (source == CLOSE_TAB_FROM_TOUCH)
    StartResizeLayoutTabsFromTouchTimer();
  else
    AddMessageLoopObserver();
}

void TabContainer::ExitTabClosingMode() {
  in_tab_close_ = false;
  override_available_width_for_tabs_.reset();
}

void TabContainer::SetTabSlotVisibility() {
  bool last_tab_visible = false;
  absl::optional<tab_groups::TabGroupId> last_tab_group = absl::nullopt;
  std::vector<Tab*> tabs = layout_helper()->GetTabs();
  for (Tab* tab : base::Reversed(tabs)) {
    absl::optional<tab_groups::TabGroupId> current_group = tab->group();
    if (current_group != last_tab_group && last_tab_group.has_value()) {
      TabGroupViews* group_view =
          group_views().at(last_tab_group.value()).get();
      group_view->header()->SetVisible(last_tab_visible);
      // Hide underlines if they would underline an invisible tab, but don't
      // show underlines if they're hidden during a header drag session.
      if (!group_view->header()->dragging())
        group_view->underline()->SetVisible(last_tab_visible);
    }
    last_tab_visible = ShouldTabBeVisible(tab);
    last_tab_group = tab->closing() ? absl::nullopt : current_group;

    // Collapsed tabs disappear once they've reached their minimum size. This
    // is different than very small non-collapsed tabs, because in that case
    // the tab (and its favicon) must still be visible.
    bool is_collapsed = (current_group.has_value() &&
                         controller_->IsGroupCollapsed(current_group.value()) &&
                         tab->bounds().width() <= TabStyle::GetTabOverlap());
    tab->SetVisible(is_collapsed ? false : last_tab_visible);
  }
}

void TabContainer::Layout() {
  if (base::FeatureList::IsEnabled(features::kScrollableTabStrip)) {
    // With tab scrolling, the tab container is solely responsible for its own
    // width.
    // It should never be larger than its preferred width.
    const int max_width = CalculatePreferredSize().width();
    // It should never be smaller than its minimum width.
    const int min_width = GetMinimumSize().width();
    // If it can, it should fit within the tab strip region.
    const int available_width = GetAvailableWidthForTabContainer();
    // It should be as wide as possible subject to the above constraints.
    const int width = std::min(max_width, std::max(min_width, available_width));
    SetBounds(0, 0, width, GetLayoutConstant(TAB_HEIGHT));
    SetTabSlotVisibility();
  }

  if (IsAnimating()) {
    // Hide tabs that have animated at least partially out of the clip region.
    SetTabSlotVisibility();
    return;
  }

  // Only do a layout if our size or the available width changed.
  const int available_width = GetAvailableWidthForTabContainer();
  if (last_layout_size_ == size() && last_available_width_ == available_width)
    return;
  if (IsDragSessionActive())
    return;
  CompleteAnimationAndLayout();
}

void TabContainer::PaintChildren(const views::PaintInfo& paint_info) {
  std::vector<ZOrderableTabContainerElement> orderable_children;
  for (views::View* child : children())
    orderable_children.emplace_back(child);

  // Sort in ascending order by z-value. Stable sort breaks ties by child index.
  std::stable_sort(orderable_children.begin(), orderable_children.end());

  for (const ZOrderableTabContainerElement& child : orderable_children)
    child.view()->Paint(paint_info);
}

gfx::Size TabContainer::GetMinimumSize() const {
  int minimum_width = layout_helper_->CalculateMinimumWidth();

  return gfx::Size(minimum_width, GetLayoutConstant(TAB_HEIGHT));
}

gfx::Size TabContainer::CalculatePreferredSize() const {
  int preferred_width;
  // The tab container needs to always exactly fit the bounds of the tabs so
  // that NTB can be laid out just to the right of the rightmost tab. When the
  // tabs aren't at their ideal bounds (i.e. during animation or a drag), we
  // need to size ourselves to exactly fit wherever the tabs *currently* are.
  if (IsAnimating() || IsDragSessionActive()) {
    // The visual order of the tabs can be out of sync with the logical order,
    // so we have to check all of them to find the visually trailing-most one.
    int max_x = 0;
    for (views::View* child : children())
      max_x = std::max(max_x, child->bounds().right());

    // We also need to check each tab, in case the rightmost tab is currently
    // being dragged. Group headers don't need such treatment, since any drag
    // session including such a header must also include a tab to its right.
    for (Tab* tab : layout_helper_->GetTabs())
      max_x = std::max(max_x, tab->bounds().right());

    // The tabs span from 0 to |max_x|, so |max_x| is the current width
    // occupied by tabs. We report the current width as our preferred width so
    // that the tab strip is sized to exactly fit the current position of the
    // tabs.
    preferred_width = max_x;
  } else {
    preferred_width = override_available_width_for_tabs_.value_or(
        layout_helper_->CalculatePreferredWidth());
  }

  return gfx::Size(preferred_width, GetLayoutConstant(TAB_HEIGHT));
}

views::View* TabContainer::GetTooltipHandlerForPoint(const gfx::Point& point) {
  if (!HitTestPoint(point))
    return nullptr;

  // Return any view that isn't a Tab or this TabContainer immediately. We don't
  // want to interfere.
  views::View* v = View::GetTooltipHandlerForPoint(point);
  if (v && v != this && !views::IsViewClass<Tab>(v))
    return v;

  views::View* tab = FindTabHitByPoint(point);
  if (tab)
    return tab;

  return this;
}

BrowserRootView::DropIndex TabContainer::GetDropIndex(
    const ui::DropTargetEvent& event) {
  // Force animations to stop, otherwise it makes the index calculation tricky.
  CompleteAnimationAndLayout();

  // If the UI layout is right-to-left, we need to mirror the mouse
  // coordinates since we calculate the drop index based on the
  // original (and therefore non-mirrored) positions of the tabs.
  const int x = GetMirroredXInView(event.x());

  std::vector<TabSlotView*> views = layout_helper()->GetTabSlotViews();

  // Loop until we find a tab or group header that intersects |event|'s
  // location.
  for (TabSlotView* view : views) {
    const int max_x = view->x() + view->width();
    if (x >= max_x)
      continue;

    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      Tab* const tab = static_cast<Tab*>(view);
      // Closing tabs should be skipped.
      if (tab->closing())
        continue;

      // GetModelIndexOf is an O(n) operation. Since we will definitely
      // return from the loop at this point, it is only called once.
      // Hence the loop is still O(n). Calling this every loop iteration
      // must be avoided since it will become O(n^2).
      const int model_index = GetModelIndexOf(tab);
      const bool first_in_group =
          tab->group().has_value() &&
          model_index == controller_->GetFirstTabInGroup(tab->group().value());

      // When hovering over the left or right quarter of a tab, the drop
      // indicator will point between tabs.
      const int hot_width = tab->width() / 4;

      if (x >= (max_x - hot_width))
        return {model_index + 1, true /* drop_before */,
                false /* drop_in_group */};
      else if (x < tab->x() + hot_width)
        return {model_index, true /* drop_before */, first_in_group};
      else
        return {model_index, false /* drop_before */,
                false /* drop_in_group */};
    } else {
      TabGroupHeader* const group_header = static_cast<TabGroupHeader*>(view);
      const int first_tab_index =
          controller_->GetFirstTabInGroup(group_header->group().value())
              .value();

      if (x < max_x - group_header->width() / 2)
        return {first_tab_index, true /* drop_before */,
                false /* drop_in_group */};
      else
        return {first_tab_index, true /* drop_before */,
                true /* drop_in_group */};
    }
  }

  // The drop isn't over a tab, add it to the end.
  return {GetTabCount(), true, false};
}

views::View* TabContainer::GetViewForDrop() {
  return this;
}

BrowserRootView::DropTarget* TabContainer::GetDropTarget(
    gfx::Point loc_in_local_coords) {
  if (IsDrawn()) {
    // Allow the drop as long as the mouse is over tab container or vertically
    // before it.
    if (loc_in_local_coords.y() < height())
      return this;
  }

  return nullptr;
}

void TabContainer::HandleDragUpdate(
    const absl::optional<BrowserRootView::DropIndex>& index) {
  SetDropArrow(index);
}

void TabContainer::HandleDragExited() {
  SetDropArrow({});
}

views::View* TabContainer::TargetForRect(views::View* root,
                                         const gfx::Rect& rect) {
  CHECK_EQ(root, this);

  if (!views::UsePointBasedTargeting(rect))
    return views::ViewTargeterDelegate::TargetForRect(root, rect);
  const gfx::Point point(rect.CenterPoint());

  // Return any view that isn't a Tab or this TabStrip immediately. We don't
  // want to interfere.
  views::View* v = views::ViewTargeterDelegate::TargetForRect(root, rect);
  if (v && v != this && !views::IsViewClass<Tab>(v))
    return v;

  views::View* tab = FindTabHitByPoint(point);
  if (tab)
    return tab;

  return this;
}

void TabContainer::MouseMovedOutOfHost() {
  ResizeLayoutTabs();
}

void TabContainer::OnBoundsAnimatorDone(views::BoundsAnimator* animator) {
  // Send the Container a message to simulate a mouse moved event at the current
  // mouse position. This tickles the Tab the mouse is currently over to show
  // the "hot" state of the close button, or to show the hover card, etc.  Note
  // that this is not required (and indeed may crash!) during a drag session.
  if (!IsDragSessionActive()) {
    // The widget can apparently be null during shutdown.
    views::Widget* widget = GetWidget();
    if (widget)
      widget->SynthesizeMouseMoveEvent();
  }
}

// TabContainer::DropArrow:
// ----------------------------------------------------------

TabContainer::DropArrow::DropArrow(const BrowserRootView::DropIndex& index,
                                   bool point_down,
                                   views::Widget* context)
    : index_(index), point_down_(point_down) {
  arrow_window_ = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.z_order = ui::ZOrderLevel::kFloatingUIElement;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.accept_events = false;
  params.bounds = gfx::Rect(g_drop_indicator_width, g_drop_indicator_height);
  params.context = context->GetNativeWindow();
  arrow_window_->Init(std::move(params));
  arrow_view_ =
      arrow_window_->SetContentsView(std::make_unique<views::ImageView>());
  arrow_view_->SetImage(GetDropArrowImage(point_down_));
  scoped_observation_.Observe(arrow_window_.get());

  arrow_window_->Show();
}

TabContainer::DropArrow::~DropArrow() {
  // Close eventually deletes the window, which deletes arrow_view too.
  if (arrow_window_)
    arrow_window_->Close();
}

void TabContainer::DropArrow::SetPointDown(bool down) {
  if (point_down_ == down)
    return;

  point_down_ = down;
  arrow_view_->SetImage(GetDropArrowImage(point_down_));
}

void TabContainer::DropArrow::SetWindowBounds(const gfx::Rect& bounds) {
  arrow_window_->SetBounds(bounds);
}

void TabContainer::DropArrow::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(scoped_observation_.IsObservingSource(arrow_window_.get()));
  scoped_observation_.Reset();
  arrow_window_ = nullptr;
}

void TabContainer::UpdateIdealBounds() {
  if (GetTabCount() == 0)
    return;  // Should only happen during creation/destruction, ignore.

  // Update |last_available_width_| in case there is a different amount of
  // available width than there was in the last layout (e.g. if the tabstrip
  // is currently hidden).
  last_available_width_ = GetAvailableWidthForTabContainer();

  const int available_width_for_tabs = CalculateAvailableWidthForTabs();
  layout_helper()->UpdateIdealBounds(available_width_for_tabs);
}

void TabContainer::AnimateToIdealBounds() {
  UpdateHoverCard(nullptr, TabSlotController::HoverCardUpdateType::kAnimating);

  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    const gfx::Rect& target_bounds = tabs_view_model_.ideal_bounds(i);

    AnimateTabSlotViewTo(tab, target_bounds);
  }

  for (const auto& header_pair : group_views()) {
    TabGroupHeader* const header = header_pair.second->header();
    const gfx::Rect& target_bounds =
        layout_helper()->group_header_ideal_bounds().at(header_pair.first);

    AnimateTabSlotViewTo(header, target_bounds);
  }

  // Because the preferred size of the tabstrip depends on the IsAnimating()
  // condition, but starting an animation doesn't necessarily invalidate the
  // existing preferred size and layout (which may now be incorrect), we need to
  // signal this explicitly.
  PreferredSizeChanged();
}

void TabContainer::AnimateTabSlotViewTo(TabSlotView* tab_slot_view,
                                        const gfx::Rect& target_bounds) {
  // If the slot is being dragged, we don't own it - let TabDragContext decide
  // what to do.
  if (tab_slot_view->dragging()) {
    drag_context_->UpdateAnimationTarget(tab_slot_view, target_bounds);
    return;
  }

  // Also skip slots already being animated to the same ideal bounds.  Calling
  // AnimateViewTo() again restarts the animation, which changes the timing of
  // how the slot animates, leading to hitches.
  if (bounds_animator().GetTargetBounds(tab_slot_view) == target_bounds)
    return;

  bounds_animator().AnimateViewTo(
      tab_slot_view, target_bounds,
      std::make_unique<TabSlotAnimationDelegate>(this, tab_slot_view));
}

void TabContainer::SnapToIdealBounds() {
  for (int i = 0; i < GetTabCount(); ++i)
    GetTabAtModelIndex(i)->SetBoundsRect(tabs_view_model_.ideal_bounds(i));

  for (const auto& header_pair : group_views()) {
    header_pair.second->header()->SetBoundsRect(
        layout_helper()->group_header_ideal_bounds().at(header_pair.first));
    header_pair.second->UpdateBounds();
  }

  PreferredSizeChanged();
}

int TabContainer::CalculateAvailableWidthForTabs() const {
  return override_available_width_for_tabs_.value_or(
      GetAvailableWidthForTabContainer());
}

void TabContainer::StartInsertTabAnimation(int model_index) {
  ExitTabClosingMode();

  gfx::Rect bounds = GetTabAtModelIndex(model_index)->bounds();
  bounds.set_height(GetLayoutConstant(TAB_HEIGHT));

  // Adjust the starting bounds of the new tab.
  const int tab_overlap = TabStyle::GetTabOverlap();
  if (model_index > 0) {
    // If we have a tab to our left, start at its right edge.
    bounds.set_x(GetTabAtModelIndex(model_index - 1)->bounds().right() -
                 tab_overlap);
  } else if (model_index + 1 < GetTabCount()) {
    // Otherwise, if we have a tab to our right, start at its left edge.
    bounds.set_x(GetTabAtModelIndex(model_index + 1)->bounds().x());
  } else {
    NOTREACHED() << "First tab inserted into the tabstrip should not animate.";
  }

  // Start at the width of the overlap in order to animate at the same speed
  // the surrounding tabs are moving, since at this width the subsequent tab
  // is naturally positioned at the same X coordinate.
  bounds.set_width(tab_overlap);
  GetTabAtModelIndex(model_index)->SetBoundsRect(bounds);

  // Animate in to the full width.
  StartBasicAnimation();
}

void TabContainer::StartRemoveTabAnimation(Tab* tab, int former_model_index) {
  if (in_tab_close_ && GetTabCount() > 0 &&
      override_available_width_for_tabs_ >
          tabs_view_model_.ideal_bounds(GetTabCount() - 1).right()) {
    // Tab closing mode is no longer constraining tab widths - they're at full
    // size. Exit tab closing mode so that it doesn't artificially inflate our
    // bounds.
    ExitTabClosingMode();
  }

  StartBasicAnimation();

  gfx::Rect target_bounds =
      GetTargetBoundsForClosingTab(tab, former_model_index);

  // If the tab is being dragged, we don't own it, and can't run animations on
  // it. We need to take it back first.
  if (tab->dragging()) {
    // Don't bother animating if the tab has been detached rather than closed -
    // i.e. it's being moved to another tabstrip. At this point it's safe to
    // just destroy the tab immediately.
    if (tab->detached()) {
      OnTabCloseAnimationCompleted(tab);
      return;
    }

    DCHECK(drag_context_->IsEndingDrag());
    // Notify |drag_context_| of the new animation target, since we can't
    // animate |tab| ourselves.
    drag_context_->UpdateAnimationTarget(tab, target_bounds);
    return;
  }

  // TODO(pkasting): When closing multiple tabs, we get repeated RemoveTabAt()
  // calls, each of which closes a new tab and thus generates different ideal
  // bounds.  We should update the animations of any other tabs that are
  // currently being closed to reflect the new ideal bounds, or else change from
  // removing one tab at a time to animating the removal of all tabs at once.

  bounds_animator().AnimateViewTo(
      tab, target_bounds, std::make_unique<RemoveTabDelegate>(this, tab));
}

gfx::Rect TabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  const int tab_overlap = TabStyle::GetTabOverlap();

  // Compute the target bounds for animating this tab closed.  The tab's left
  // edge should stay joined to the right edge of the previous tab, if any.
  gfx::Rect target_bounds = tab->bounds();
  target_bounds.set_x(
      (former_model_index > 0)
          ? (tabs_view_model_.ideal_bounds(former_model_index - 1).right() -
             tab_overlap)
          : 0);

  // The tab should animate to the width of the overlap in order to close at the
  // same speed the surrounding tabs are moving, since at this width the
  // subsequent tab is naturally positioned at the same X coordinate.
  target_bounds.set_width(tab_overlap);

  return target_bounds;
}

void TabContainer::RemoveTabFromViewModel(int index) {
  Tab* tab = GetTabAtModelIndex(index);
  bool tab_was_active = tab->IsActive();

  UpdateHoverCard(nullptr, TabSlotController::HoverCardUpdateType::kTabRemoved);

  tabs_view_model_.Remove(index);
  layout_helper_->RemoveTabAt(index, tab);

  if (tab_was_active)
    tab->ActiveStateChanged();
}

void TabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  DCHECK(tab->closing());

  std::unique_ptr<Tab> deleter(tab);
  layout_helper_->OnTabDestroyed(tab);
}

void TabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                 bool was_active) {
  // The tab at |model_index| has already been removed from the model, but is
  // still in |tabs_view_model_|.  Index math with care!
  const int model_count = GetTabCount() - 1;
  const int tab_overlap = TabStyle::GetTabOverlap();
  if (in_tab_close() && model_count > 0 && model_index != model_count) {
    // The user closed a tab other than the last tab. Set
    // override_available_width_for_tabs_ so that as the user closes tabs with
    // the mouse a tab continues to fall under the mouse.
    int next_active_index = controller_->GetActiveIndex();
    DCHECK(IsValidModelIndex(next_active_index));
    if (model_index <= next_active_index) {
      // At this point, model's internal state has already been updated.
      // |contents| has been detached from model and the active index has been
      // updated. But the tab for |contents| isn't removed yet. Thus, we need to
      // fix up next_active_index based on it.
      next_active_index++;
    }
    Tab* next_active_tab = GetTabAtModelIndex(next_active_index);
    Tab* tab_being_removed = GetTabAtModelIndex(model_index);

    int size_delta = tab_being_removed->width();
    // When removing an active, non-pinned tab, the next active tab will be
    // given the active width (unless it is pinned). Thus the width being
    // removed from the container is really the current width of whichever
    // inactive tab will be made active.
    if (was_active && !tab_being_removed->data().pinned &&
        !next_active_tab->data().pinned &&
        layout_helper_->active_tab_width() >
            layout_helper_->inactive_tab_width()) {
      size_delta = next_active_tab->width();
    }

    override_available_width_for_tabs_ =
        tabs_view_model_.ideal_bounds(model_count).right() - size_delta +
        tab_overlap;
  }
}

void TabContainer::ResizeLayoutTabs() {
  // We've been called back after the TabStrip has been emptied out (probably
  // just prior to the window being destroyed). We need to do nothing here or
  // else GetTabAt below will crash.
  if (GetTabCount() == 0)
    return;

  // It is critically important that this is unhooked here, otherwise we will
  // keep spying on messages forever.
  RemoveMessageLoopObserver();

  ExitTabClosingMode();
  int pinned_tab_count = layout_helper_->GetPinnedTabCount();
  if (pinned_tab_count == GetTabCount()) {
    // Only pinned tabs, we know the tab widths won't have changed (all
    // pinned tabs have the same width), so there is nothing to do.
    return;
  }
  // Don't try and avoid layout based on tab sizes. If tabs are small enough
  // then the width of the active tab may not change, but other widths may
  // have. This is particularly important if we've overflowed (all tabs are at
  // the min).
  StartBasicAnimation();
}

void TabContainer::ResizeLayoutTabsFromTouch() {
  // Don't resize if the user is interacting with the tabstrip.
  if (!IsDragSessionActive())
    ResizeLayoutTabs();
  else
    StartResizeLayoutTabsFromTouchTimer();
}

void TabContainer::StartResizeLayoutTabsFromTouchTimer() {
  // Amount of time we delay before resizing after a close from a touch.
  constexpr auto kTouchResizeLayoutTime = base::Seconds(2);

  resize_layout_timer_.Stop();
  resize_layout_timer_.Start(FROM_HERE, kTouchResizeLayoutTime, this,
                             &TabContainer::ResizeLayoutTabsFromTouch);
}

bool TabContainer::IsDragSessionActive() const {
  // |drag_context_| may be null in tests.
  return drag_context_ && drag_context_->IsDragSessionActive();
}

void TabContainer::AddMessageLoopObserver() {
  if (!mouse_watcher_) {
    // Expand the watched region downwards below the bottom of the tabstrip.
    // This allows users to move the cursor horizontally, to another tab,
    // without accidentally exiting closing mode if they drift verticaally
    // slightly out of the tabstrip.
    constexpr int kTabStripAnimationVSlop = 40;
    // Expand the watched region to the right to cover the NTB. This prevents
    // the scenario where the user goes to click on the NTB while they're in
    // closing mode, and closing mode exits just as they reach the NTB.
    constexpr int kTabStripAnimationHSlop = 60;
    mouse_watcher_ = std::make_unique<views::MouseWatcher>(
        std::make_unique<views::MouseWatcherViewHost>(
            this, gfx::Insets::TLBR(
                      0, base::i18n::IsRTL() ? kTabStripAnimationHSlop : 0,
                      kTabStripAnimationVSlop,
                      base::i18n::IsRTL() ? 0 : kTabStripAnimationHSlop)),
        this);
  }
  mouse_watcher_->Start(GetWidget()->GetNativeWindow());
}

void TabContainer::RemoveMessageLoopObserver() {
  mouse_watcher_ = nullptr;
}

void TabContainer::OrderTabSlotView(TabSlotView* slot_view) {
  if (slot_view->parent() != this)
    return;

  // |slot_view| is in the wrong place in children(). Fix it.
  std::vector<TabSlotView*> slots = layout_helper_->GetTabSlotViews();
  size_t target_slot_index =
      std::find(slots.begin(), slots.end(), slot_view) - slots.begin();
  // Find the index in children() that corresponds to |target_slot_index|.
  size_t view_index = 0;
  for (size_t slot_index = 0; slot_index < target_slot_index; slot_index++) {
    // If we don't own this view, skip it *without* advancing in children().
    if (slots[slot_index]->parent() != this)
      continue;
    if (view_index == children().size())
      break;
    view_index++;
  }

  ReorderChildView(slot_view, view_index);
}

bool TabContainer::IsPointInTab(Tab* tab,
                                const gfx::Point& point_in_tabstrip_coords) {
  if (!tab->GetVisible())
    return false;
  if (tab->parent() != this)
    return false;

  gfx::Point point_in_tab_coords(point_in_tabstrip_coords);
  View::ConvertPointToTarget(this, tab, &point_in_tab_coords);
  return tab->HitTestPoint(point_in_tab_coords);
}

Tab* TabContainer::FindTabHitByPoint(const gfx::Point& point) {
  // Check all tabs, even closing tabs. Mouse events need to reach closing tabs
  // for users to be able to rapidly middle-click close several tabs.
  std::vector<Tab*> all_tabs = layout_helper_->GetTabs();

  // The display order doesn't necessarily match the child order, so we iterate
  // in display order.
  for (size_t i = 0; i < all_tabs.size(); ++i) {
    // If we don't first exclude points outside the current tab, the code below
    // will return the wrong tab if the next tab is selected, the following tab
    // is active, and |point| is in the overlap region between the two.
    Tab* tab = all_tabs[i];
    if (!IsPointInTab(tab, point))
      continue;

    // Selected tabs render atop unselected ones, and active tabs render atop
    // everything.  Check whether the next tab renders atop this one and |point|
    // is in the overlap region.
    Tab* next_tab = i < (all_tabs.size() - 1) ? all_tabs[i + 1] : nullptr;
    if (next_tab &&
        (next_tab->IsActive() ||
         (next_tab->IsSelected() && !tab->IsSelected())) &&
        IsPointInTab(next_tab, point))
      return next_tab;

    // This is the topmost tab for this point.
    return tab;
  }

  return nullptr;
}

bool TabContainer::ShouldTabBeVisible(const Tab* tab) const {
  // When the tabstrip is scrollable, it can grow to accommodate any number of
  // tabs, so tabs can never become clipped.
  // N.B. Tabs can still be not-visible because they're in a collapsed group,
  // but that's handled elsewhere.
  // N.B. This is separate from the tab being potentially scrolled offscreen -
  // this solely determines whether the tab should be clipped for the
  // pre-scrolling overflow behavior.
  if (base::FeatureList::IsEnabled(features::kScrollableTabStrip))
    return true;

  // Detached tabs should always be invisible (as they close).
  if (tab->detached())
    return false;

  // If the tab would be clipped by the trailing edge of the strip, even if the
  // tabstrip were resized to its greatest possible width, it shouldn't be
  // visible.
  int right_edge = tab->bounds().right();
  const int tabstrip_right = tab->dragging()
                                 ? drag_context_->GetTabDragAreaWidth()
                                 : GetAvailableWidthForTabContainer();
  if (right_edge > tabstrip_right)
    return false;

  // Non-clipped dragging tabs should always be visible.
  if (tab->dragging())
    return true;

  // Let all non-clipped closing tabs be visible.  These will probably finish
  // closing before the user changes the active tab, so there's little reason to
  // try and make the more complex logic below apply.
  if (tab->closing())
    return true;

  // Now we need to check whether the tab isn't currently clipped, but could
  // become clipped if we changed the active tab, widening either this tab or
  // the tabstrip portion before it.

  // Pinned tabs don't change size when activated, so any tab in the pinned tab
  // region is safe.
  if (tab->data().pinned)
    return true;

  // If the active tab is on or before this tab, we're safe.
  if (controller_->GetActiveIndex() <= GetModelIndexOf(tab))
    return true;

  // We need to check what would happen if the active tab were to move to this
  // tab or before. If animating, we want to use the target bounds in this
  // calculation.
  if (IsAnimating())
    right_edge = bounds_animator_.GetTargetBounds(tab).right();
  return (right_edge + layout_helper_->active_tab_width() -
          layout_helper_->inactive_tab_width()) <= tabstrip_right;
}

gfx::Rect TabContainer::GetDropBounds(int drop_index,
                                      bool drop_before,
                                      bool drop_in_group,
                                      bool* is_beneath) {
  DCHECK_NE(drop_index, -1);

  // The X location the indicator points to.
  int center_x = -1;

  if (GetTabCount() == 0) {
    // If the tabstrip is empty, it doesn't matter where the drop arrow goes.
    // The tabstrip can only be transiently empty, e.g. during shutdown.
    return gfx::Rect();
  }

  Tab* tab = GetTabAtModelIndex(std::min(drop_index, GetTabCount() - 1));
  const bool first_in_group =
      drop_index < GetTabCount() && tab->group().has_value() &&
      GetModelIndexOf(tab) ==
          controller_->GetFirstTabInGroup(tab->group().value());

  const int overlap = TabStyle::GetTabOverlap();
  if (!drop_before || !first_in_group || drop_in_group) {
    // Dropping between tabs, or between a group header and the group's first
    // tab.
    center_x = tab->x();
    const int width = tab->width();
    if (drop_index < GetTabCount())
      center_x += drop_before ? (overlap / 2) : (width / 2);
    else
      center_x += width - (overlap / 2);
  } else {
    // Dropping before a group header.
    TabGroupHeader* const header = group_views_[tab->group().value()]->header();
    center_x = header->x() + overlap / 2;
  }

  // Mirror the center point if necessary.
  center_x = GetMirroredXInView(center_x);

  // Determine the screen bounds.
  gfx::Point drop_loc(center_x - g_drop_indicator_width / 2,
                      -g_drop_indicator_height);
  ConvertPointToScreen(this, &drop_loc);
  gfx::Rect drop_bounds(drop_loc.x(), drop_loc.y(), g_drop_indicator_width,
                        g_drop_indicator_height);

  // If the rect doesn't fit on the monitor, push the arrow to the bottom.
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display = screen->GetDisplayMatching(drop_bounds);
  *is_beneath = !display.bounds().Contains(drop_bounds);
  if (*is_beneath)
    drop_bounds.Offset(0, drop_bounds.height() + height());

  return drop_bounds;
}

void TabContainer::SetDropArrow(
    const absl::optional<BrowserRootView::DropIndex>& index) {
  if (!index) {
    controller_->OnDropIndexUpdate(-1, false);
    drop_arrow_.reset();
    return;
  }

  // Let the controller know of the index update.
  controller_->OnDropIndexUpdate(index->value, index->drop_before);

  if (drop_arrow_ && (index == drop_arrow_->index()))
    return;

  bool is_beneath;
  gfx::Rect drop_bounds = GetDropBounds(index->value, index->drop_before,
                                        index->drop_in_group, &is_beneath);

  if (!drop_arrow_) {
    drop_arrow_ = std::make_unique<DropArrow>(*index, !is_beneath, GetWidget());
  } else {
    drop_arrow_->set_index(*index);
    drop_arrow_->SetPointDown(!is_beneath);
  }

  // Reposition the window.
  drop_arrow_->SetWindowBounds(drop_bounds);
}

void TabContainer::UpdateAccessibleTabIndices() {
  const int num_tabs = GetTabCount();
  for (int i = 0; i < num_tabs; ++i) {
    GetTabAtModelIndex(i)->GetViewAccessibility().OverridePosInSet(i + 1,
                                                                   num_tabs);
  }
}

bool TabContainer::IsValidModelIndex(int model_index) const {
  return controller_->IsValidIndex(model_index);
}

BEGIN_METADATA(TabContainer, views::View)
ADD_READONLY_PROPERTY_METADATA(int, AvailableWidthForTabContainer)
END_METADATA

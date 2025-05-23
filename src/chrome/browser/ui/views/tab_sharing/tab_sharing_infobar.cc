// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tab_sharing/tab_sharing_infobar.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "build/build_config.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/view_class_properties.h"

namespace {
using TabRole = ::TabSharingInfoBarDelegate::TabRole;

constexpr auto kCapturedSurfaceControlIndicatorButtonInsets =
    gfx::Insets::VH(4, 8);

std::u16string GetMessageTextCastingNoSinkName(
    bool shared_tab,
    const std::u16string& shared_tab_name) {
  if (shared_tab) {
    return l10n_util::GetStringUTF16(
        IDS_TAB_CASTING_INFOBAR_CASTING_CURRENT_TAB_NO_DEVICE_NAME_LABEL);
  }
  return shared_tab_name.empty()
             ? l10n_util::GetStringUTF16(
                   IDS_TAB_CASTING_INFOBAR_CASTING_ANOTHER_UNTITLED_TAB_NO_DEVICE_NAME_LABEL)
             : l10n_util::GetStringFUTF16(
                   IDS_TAB_CASTING_INFOBAR_CASTING_ANOTHER_TAB_NO_DEVICE_NAME_LABEL,
                   shared_tab_name);
}

std::u16string GetMessageTextCasting(bool shared_tab,
                                     const std::u16string& shared_tab_name,
                                     const std::u16string& sink_name) {
  if (sink_name.empty()) {
    return GetMessageTextCastingNoSinkName(shared_tab, shared_tab_name);
  }

  if (shared_tab) {
    return l10n_util::GetStringFUTF16(
        IDS_TAB_CASTING_INFOBAR_CASTING_CURRENT_TAB_LABEL, sink_name);
  }
  return shared_tab_name.empty()
             ? l10n_util::GetStringFUTF16(
                   IDS_TAB_CASTING_INFOBAR_CASTING_ANOTHER_UNTITLED_TAB_LABEL,
                   sink_name)
             : l10n_util::GetStringFUTF16(
                   IDS_TAB_CASTING_INFOBAR_CASTING_ANOTHER_TAB_LABEL,
                   shared_tab_name, sink_name);
}

std::u16string GetMessageTextCapturing(bool shared_tab,
                                       const std::u16string& shared_tab_name,
                                       const std::u16string& app_name) {
  if (shared_tab) {
    return l10n_util::GetStringFUTF16(
        IDS_TAB_SHARING_INFOBAR_SHARING_CURRENT_TAB_LABEL, app_name);
  }
  return !shared_tab_name.empty()
             ? l10n_util::GetStringFUTF16(
                   IDS_TAB_SHARING_INFOBAR_SHARING_ANOTHER_TAB_LABEL,
                   shared_tab_name, app_name)
             : l10n_util::GetStringFUTF16(
                   IDS_TAB_SHARING_INFOBAR_SHARING_ANOTHER_UNTITLED_TAB_LABEL,
                   app_name);
}

bool IsCapturedTab(TabRole role) {
  switch (role) {
    case TabRole::kCapturingTab:
    case TabRole::kOtherTab:
      return false;
    case TabRole::kCapturedTab:
    case TabRole::kSelfCapturingTab:
      return true;
  }
  NOTREACHED();
}

}  // namespace

TabSharingInfoBar::TabSharingInfoBar(
    std::unique_ptr<TabSharingInfoBarDelegate> delegate,
    const std::u16string& shared_tab_name,
    const std::u16string& capturer_name,
    TabSharingInfoBarDelegate::TabRole role,
    TabSharingInfoBarDelegate::TabShareType capture_type)
    : InfoBarView(std::move(delegate)),
      shared_tab_name_(std::move(shared_tab_name)),
      capturer_name_(std::move(capturer_name)),
      role_(role),
      capture_type_(capture_type) {
  auto* delegate_ptr = GetDelegate();
  label_ = AddChildView(CreateLabel(GetMessageText()));
  label_->SetElideBehavior(gfx::ELIDE_TAIL);

  const int buttons = delegate_ptr->GetButtons();
  const auto create_button =
      [&](TabSharingInfoBarDelegate::TabSharingInfoBarButton type,
          void (TabSharingInfoBar::*click_function)(),
          int button_context = views::style::CONTEXT_BUTTON_MD) {
        const bool use_text_color_for_icon =
            type != TabSharingInfoBarDelegate::kCapturedSurfaceControlIndicator;
        auto* button = AddChildView(std::make_unique<views::MdTextButton>(
            base::BindRepeating(click_function, base::Unretained(this)),
            delegate_ptr->GetButtonLabel(type), button_context,
            use_text_color_for_icon));
        button->SetProperty(
            views::kMarginsKey,
            gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                                DISTANCE_TOAST_CONTROL_VERTICAL),
                            0));

        const bool is_default_button =
            type == buttons || type == TabSharingInfoBarDelegate::kStop;
        button->SetStyle(is_default_button ? ui::ButtonStyle::kProminent
                                           : ui::ButtonStyle::kTonal);
        button->SetImageModel(views::Button::STATE_NORMAL,
                              delegate_ptr->GetButtonImage(type));
        button->SetEnabled(delegate_ptr->IsButtonEnabled(type));
        button->SetTooltipText(delegate_ptr->GetButtonTooltip(type));
        return button;
      };

  if (buttons & TabSharingInfoBarDelegate::kStop) {
    stop_button_ = create_button(TabSharingInfoBarDelegate::kStop,
                                 &TabSharingInfoBar::StopButtonPressed);
  }

  if (buttons & TabSharingInfoBarDelegate::kShareThisTabInstead) {
    share_this_tab_instead_button_ =
        create_button(TabSharingInfoBarDelegate::kShareThisTabInstead,
                      &TabSharingInfoBar::ShareThisTabInsteadButtonPressed);
  }

  if (buttons & TabSharingInfoBarDelegate::kQuickNav) {
    quick_nav_button_ =
        create_button(TabSharingInfoBarDelegate::kQuickNav,
                      &TabSharingInfoBar::QuickNavButtonPressed);
  }

  if (buttons & TabSharingInfoBarDelegate::kCapturedSurfaceControlIndicator) {
    csc_indicator_button_ = create_button(
        TabSharingInfoBarDelegate::kCapturedSurfaceControlIndicator,
        &TabSharingInfoBar::OnCapturedSurfaceControlActivityIndicatorPressed,
        CONTEXT_OMNIBOX_PRIMARY);
    csc_indicator_button_->SetStyle(ui::ButtonStyle::kDefault);
    csc_indicator_button_->SetCornerRadius(
        GetLayoutConstant(TOOLBAR_CORNER_RADIUS));
    csc_indicator_button_->SetCustomPadding(
        kCapturedSurfaceControlIndicatorButtonInsets);
    csc_indicator_button_->SetTextColorId(
        views::Button::ButtonState::STATE_NORMAL, ui::kColorSysOnSurface);
  }

  // TODO(crbug.com/378107817): It seems like link_ isn't always needed, but
  // it's added regardless. See about only adding when necessary.
  link_ = AddChildView(CreateLink(delegate_ptr->GetLinkText()));
}

TabSharingInfoBar::~TabSharingInfoBar() = default;

void TabSharingInfoBar::Layout(PassKey) {
  LayoutSuperclass<InfoBarView>(this);

  if (stop_button_) {
    stop_button_->SizeToPreferredSize();
  }

  if (share_this_tab_instead_button_) {
    share_this_tab_instead_button_->SizeToPreferredSize();
  }

  if (quick_nav_button_) {
    quick_nav_button_->SizeToPreferredSize();
  }

  if (csc_indicator_button_) {
    csc_indicator_button_->SizeToPreferredSize();
  }

  int x = GetStartX();
  Views views;
  views.push_back(label_.get());
  views.push_back(link_.get());
  AssignWidths(&views, std::max(0, GetEndX() - x - NonLabelWidth()));

  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  label_->SetPosition(gfx::Point(x, OffsetY(label_)));
  if (!label_->GetText().empty()) {
    x = label_->bounds().right() +
        layout_provider->GetDistanceMetric(
            DISTANCE_INFOBAR_HORIZONTAL_ICON_LABEL_PADDING);
  }

  // Add buttons into a vector to be displayed in an ordered row.
  // Depending on the PlatformStyle, reverse the vector so the stop button will
  // be on the correct leading style.
  std::vector<views::MdTextButton*> order_of_buttons;
  if (stop_button_) {
    order_of_buttons.push_back(stop_button_);
  }
  if (share_this_tab_instead_button_) {
    order_of_buttons.push_back(share_this_tab_instead_button_);
  }
  if (quick_nav_button_) {
    order_of_buttons.push_back(quick_nav_button_);
  }
  if (csc_indicator_button_) {
    order_of_buttons.push_back(csc_indicator_button_);
  }

  if constexpr (!views::PlatformStyle::kIsOkButtonLeading) {
    base::ranges::reverse(order_of_buttons);
  }

  for (views::MdTextButton* button : order_of_buttons) {
    button->SetPosition(gfx::Point(x, OffsetY(button)));
    x = button->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
  }

  link_->SetPosition(gfx::Point(GetEndX() - link_->width(), OffsetY(link_)));
}

void TabSharingInfoBar::StopButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  GetDelegate()->Stop();
}

void TabSharingInfoBar::ShareThisTabInsteadButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  GetDelegate()->ShareThisTabInstead();
}

void TabSharingInfoBar::QuickNavButtonPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  GetDelegate()->QuickNav();
}

void TabSharingInfoBar::OnCapturedSurfaceControlActivityIndicatorPressed() {
  if (!owner()) {
    return;  // We're closing; don't call anything, it might access the owner.
  }
  GetDelegate()->OnCapturedSurfaceControlActivityIndicatorPressed();
}

TabSharingInfoBarDelegate* TabSharingInfoBar::GetDelegate() {
  return static_cast<TabSharingInfoBarDelegate*>(delegate());
}

std::u16string TabSharingInfoBar::GetMessageText() const {
  switch (capture_type_) {
    case TabSharingInfoBarDelegate::TabShareType::CAST:
      return GetMessageTextCasting(IsCapturedTab(role_), shared_tab_name_,
                                   capturer_name_);
    case TabSharingInfoBarDelegate::TabShareType::CAPTURE:
      return GetMessageTextCapturing(IsCapturedTab(role_), shared_tab_name_,
                                     capturer_name_);
  }
  NOTREACHED();
}

int TabSharingInfoBar::GetContentMinimumWidth() const {
  return label_->GetMinimumSize().width() + link_->GetMinimumSize().width() +
         NonLabelWidth();
}

int TabSharingInfoBar::NonLabelWidth() const {
  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  const int label_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_LABEL_HORIZONTAL);
  const int button_spacing = layout_provider->GetDistanceMetric(
      views::DISTANCE_RELATED_BUTTON_HORIZONTAL);

  const int button_count =
      (stop_button_ ? 1 : 0) + (share_this_tab_instead_button_ ? 1 : 0) +
      (quick_nav_button_ ? 1 : 0) + (csc_indicator_button_ ? 1 : 0);

  int width =
      (label_->GetText().empty() || button_count == 0) ? 0 : label_spacing;

  width += std::max(0, button_spacing * (button_count - 1));

  width += stop_button_ ? stop_button_->width() : 0;
  width += share_this_tab_instead_button_
               ? share_this_tab_instead_button_->width()
               : 0;
  width += quick_nav_button_ ? quick_nav_button_->width() : 0;
  width += csc_indicator_button_ ? csc_indicator_button_->width() : 0;

  return width + ((width && !link_->GetText().empty()) ? label_spacing : 0);
}

std::unique_ptr<infobars::InfoBar> CreateTabSharingInfoBar(
    std::unique_ptr<TabSharingInfoBarDelegate> delegate,
    const std::u16string& shared_tab_name,
    const std::u16string& capturer_name,
    TabSharingInfoBarDelegate::TabRole role,
    TabSharingInfoBarDelegate::TabShareType capture_type) {
  return std::make_unique<TabSharingInfoBar>(
      std::move(delegate), shared_tab_name, capturer_name, role, capture_type);
}

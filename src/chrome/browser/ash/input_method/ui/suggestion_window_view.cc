// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/input_method/ui/suggestion_window_view.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ash/input_method/assistive_window_properties.h"
#include "chrome/browser/ash/input_method/ui/assistive_delegate.h"
#include "chrome/browser/ash/input_method/ui/border_factory.h"
#include "chrome/browser/ash/input_method/ui/colors.h"
#include "chrome/browser/ash/input_method/ui/completion_suggestion_view.h"
#include "chrome/browser/ash/input_method/ui/suggestion_details.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/ui_base_types.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_properties.h"

namespace ui {
namespace ime {

namespace {

bool ShouldHighlight(const views::Button& button) {
  return button.GetState() == views::Button::STATE_HOVERED ||
         button.GetState() == views::Button::STATE_PRESSED;
}

// TODO(b/1101669): Create abstract HighlightableButton for learn_more button,
// setting_link_, suggestion_view and undo_view.
void SetHighlighted(views::View& view, bool highlighted) {
  if (!!view.background() != highlighted) {
    view.SetBackground(highlighted
                           ? views::CreateSolidBackground(
                                 ResolveSemanticColor(kButtonHighlightColor))
                           : nullptr);
  }
}

}  // namespace

// static
SuggestionWindowView* SuggestionWindowView::Create(gfx::NativeView parent,
                                                   AssistiveDelegate* delegate,
                                                   Orientation orientation) {
  auto* const view = new SuggestionWindowView(parent, delegate, orientation);
  views::Widget* const widget =
      views::BubbleDialogDelegateView::CreateBubble(view);
  wm::SetWindowVisibilityAnimationTransition(widget->GetNativeView(),
                                             wm::ANIMATE_NONE);
  return view;
}

std::unique_ptr<views::NonClientFrameView>
SuggestionWindowView::CreateNonClientFrameView(views::Widget* widget) {
  std::unique_ptr<views::NonClientFrameView> frame =
      views::BubbleDialogDelegateView::CreateNonClientFrameView(widget);
  static_cast<views::BubbleFrameView*>(frame.get())
      ->SetBubbleBorder(GetBorderForWindow(WindowBorderType::Suggestion));
  return frame;
}

void SuggestionWindowView::Show(const SuggestionDetails& details) {
  ResizeCandidateArea(0);

  completion_view_->SetVisible(true);
  completion_view_->SetView(details);
  if (details.show_setting_link)
    completion_view_->SetMinWidth(setting_link_->GetPreferredSize().width());

  setting_link_->SetVisible(details.show_setting_link);

  MakeVisible();
}

void SuggestionWindowView::ShowMultipleCandidates(
    const ash::input_method::AssistiveWindowProperties& properties) {
  const std::vector<std::u16string>& candidates = properties.candidates;
  completion_view_->SetVisible(false);
  ResizeCandidateArea(candidates.size());
  for (size_t i = 0; i < candidates.size(); ++i) {
    auto* const candidate = static_cast<views::LabelButton*>(
        multiple_candidate_area_->children()[i]);
    candidate->SetText(candidates[i]);
  }

  learn_more_button_->SetVisible(properties.show_setting_link);

  MakeVisible();
}

void SuggestionWindowView::SetButtonHighlighted(
    const AssistiveWindowButton& button,
    bool highlighted) {
  if (button.id == ButtonId::kSuggestion) {
    if (completion_view_->GetVisible()) {
      completion_view_->SetHighlighted(highlighted);
    } else {
      const views::View::Views& candidates =
          multiple_candidate_area_->children();
      if (button.index < candidates.size()) {
        SetCandidateHighlighted(
            static_cast<views::LabelButton*>(candidates[button.index]),
            highlighted);
      }
    }
  } else if (button.id == ButtonId::kSmartInputsSettingLink) {
    SetHighlighted(*setting_link_, highlighted);
  } else if (button.id == ButtonId::kLearnMore) {
    SetHighlighted(*learn_more_button_, highlighted);
  }
}

gfx::Rect SuggestionWindowView::GetBubbleBounds() {
  // The bubble bounds must be shifted to align with the anchor if there is a
  // completion view.
  const gfx::Point anchor_origin = completion_view_->GetVisible()
                                       ? completion_view_->GetAnchorOrigin()
                                       : gfx::Point(0, 0);
  return BubbleDialogDelegateView::GetBubbleBounds() -
         anchor_origin.OffsetFromOrigin();
}

void SuggestionWindowView::OnThemeChanged() {
  BubbleDialogDelegateView::OnThemeChanged();

  const auto* const color_provider = GetColorProvider();
  learn_more_button_->SetBorder(views::CreatePaddedBorder(
      views::CreateSolidSidedBorder(
          gfx::Insets::TLBR(1, 0, 0, 0),
          color_provider->GetColor(ui::kColorBubbleFooterBorder)),
      views::LayoutProvider::Get()->GetInsetsMetric(
          views::INSETS_VECTOR_IMAGE_BUTTON)));

  // TODO(crbug.com/1099044): Update and use cros colors.
  learn_more_button_->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(vector_icons::kHelpOutlineIcon,
                                     ui::kColorIconSecondary));
}

SuggestionWindowView::SuggestionWindowView(gfx::NativeView parent,
                                           AssistiveDelegate* delegate,
                                           Orientation orientation)
    : delegate_(delegate) {
  DCHECK(parent);

  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetCanActivate(false);
  set_parent_window(parent);
  set_margins(gfx::Insets());

  views::BoxLayout::Orientation layout_orientation;
  switch (orientation) {
    case Orientation::kVertical: {
      layout_orientation = views::BoxLayout::Orientation::kVertical;
      break;
    }
    case Orientation::kHorizontal: {
      layout_orientation = views::BoxLayout::Orientation::kHorizontal;
      break;
    }
    default: {
      // Unimplemented orientation.
      NOTREACHED();
      break;
    }
  }

  SetLayoutManager(std::make_unique<views::BoxLayout>(layout_orientation));
  completion_view_ = AddChildView(
      std::make_unique<CompletionSuggestionView>(base::BindRepeating(
          &AssistiveDelegate::AssistiveWindowButtonClicked,
          base::Unretained(delegate_),
          AssistiveWindowButton{.id = ui::ime::ButtonId::kSuggestion,
                                .index = 0})));
  completion_view_->SetVisible(false);
  multiple_candidate_area_ = AddChildView(std::make_unique<views::View>());
  multiple_candidate_area_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(layout_orientation));
  multiple_candidate_area_->SetVisible(false);

  setting_link_ = AddChildView(std::make_unique<views::Link>(
      l10n_util::GetStringUTF16(IDS_SUGGESTION_LEARN_MORE)));
  setting_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  // TODO(crbug/1102215): Implement proper UI layout using Insets constant.
  constexpr auto insets = gfx::Insets::TLBR(0, kPadding, kPadding, kPadding);
  setting_link_->SetBorder(views::CreateEmptyBorder(insets));
  constexpr int kSettingLinkFontSize = 11;
  setting_link_->SetFontList(gfx::FontList({kFontStyle}, gfx::Font::ITALIC,
                                           kSettingLinkFontSize,
                                           gfx::Font::Weight::NORMAL));
  const auto on_setting_link_clicked = [](AssistiveDelegate* delegate) {
    delegate->AssistiveWindowButtonClicked(
        {.id = ButtonId::kSmartInputsSettingLink});
  };
  setting_link_->SetCallback(
      base::BindRepeating(on_setting_link_clicked, delegate_));
  setting_link_->SetVisible(false);

  learn_more_button_ =
      AddChildView(std::make_unique<views::ImageButton>(base::BindRepeating(
          &AssistiveDelegate::AssistiveWindowButtonClicked,
          base::Unretained(delegate_),
          AssistiveWindowButton{
              .id = ui::ime::ButtonId::kLearnMore,
              .window_type = ui::ime::AssistiveWindowType::kEmojiSuggestion})));
  learn_more_button_->SetImageHorizontalAlignment(
      views::ImageButton::ALIGN_CENTER);
  learn_more_button_->SetImageVerticalAlignment(
      views::ImageButton::ALIGN_MIDDLE);
  learn_more_button_->SetTooltipText(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  const auto update_button_highlight = [](views::Button* button) {
    SetHighlighted(*button, ShouldHighlight(*button));
  };
  auto subscription =
      learn_more_button_->AddStateChangedCallback(base::BindRepeating(
          update_button_highlight, base::Unretained(learn_more_button_)));
  subscriptions_.insert({learn_more_button_, std::move(subscription)});
  learn_more_button_->SetVisible(false);
}

SuggestionWindowView::~SuggestionWindowView() = default;

void SuggestionWindowView::ResizeCandidateArea(size_t size) {
  const views::View::Views& candidates = multiple_candidate_area_->children();
  while (candidates.size() > size) {
    subscriptions_.erase(
        multiple_candidate_area_->RemoveChildViewT(candidates.back()).get());
  }

  for (size_t index = candidates.size(); index < size; ++index) {
    // TODO(b/217560706): Separate this into a CandidateView that will follow
    // specs and contain an index.
    auto* const candidate = multiple_candidate_area_->AddChildView(
        std::make_unique<views::LabelButton>(
            base::BindRepeating(
                &AssistiveDelegate::AssistiveWindowButtonClicked,
                base::Unretained(delegate_),
                AssistiveWindowButton{.id = ui::ime::ButtonId::kSuggestion,
                                      .index = index}),
            u""));
    candidate->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(6, 10)));

    auto subscription = candidate->AddStateChangedCallback(base::BindRepeating(
        [](SuggestionWindowView* window, views::LabelButton* button) {
          window->SetCandidateHighlighted(button, ShouldHighlight(*button));
        },
        base::Unretained(this), base::Unretained(candidate)));
    subscriptions_.insert({candidate, std::move(subscription)});
  }
}

void SuggestionWindowView::MakeVisible() {
  multiple_candidate_area_->SetVisible(true);
  SizeToContents();
}

void SuggestionWindowView::SetCandidateHighlighted(views::LabelButton* view,
                                                   bool highlighted) {
  // Clear all highlights if any exists.
  for (auto* candidate : multiple_candidate_area_->children()) {
    SetHighlighted(*candidate, false);
  }

  if (highlighted)
    SetHighlighted(*view, highlighted);
}

BEGIN_METADATA(SuggestionWindowView, views::BubbleDialogDelegateView)
END_METADATA

}  // namespace ime
}  // namespace ui

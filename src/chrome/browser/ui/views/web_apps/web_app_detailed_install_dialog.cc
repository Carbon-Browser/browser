// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <numeric>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/numerics/safe_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/browser/ui/views/web_apps/web_app_detailed_install_dialog.h"
#include "chrome/browser/ui/views/web_apps/web_app_info_image_source.h"
#include "chrome/browser/web_applications/web_app_helpers.h"
#include "chrome/browser/web_applications/web_app_prefs_utils.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/webapps/common/constants.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/dialog_model.h"
#include "ui/base/models/dialog_model_field.h"
#include "ui/base/models/image_model.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_dialog_model_host.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/layout_manager_base.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/layout/proposed_layout.h"
#include "ui/views/view.h"

namespace {

constexpr int kIconSize = 32;
constexpr int kSpacingBetweenImages = 8;

// Custom layout that sets host_size to be same as the child view's size.
class ImageCarouselLayoutManager : public views::LayoutManagerBase {
 public:
  ImageCarouselLayoutManager() = default;
  ~ImageCarouselLayoutManager() override = default;

  int GetActualFullWidth() const {
    views::View* const inner_container = host_view()->children().front();

    return inner_container->GetPreferredSize().width();
  }

 protected:
  // LayoutManagerBase:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override {
    views::ProposedLayout layout;
    views::View* const inner_container = host_view()->children().front();

    const gfx::Size item_size(inner_container->GetPreferredSize());

    layout.child_layouts.push_back({inner_container, true,
                                    gfx::Rect(gfx::Point(0, 0), item_size),
                                    views::SizeBounds(item_size)});

    layout.host_size = item_size;
    return layout;
  }
};

class ImageCarouselView : public views::View {
 public:
  explicit ImageCarouselView(const std::vector<SkBitmap>& screenshots)
      : screenshots_(screenshots) {
    DCHECK(screenshots.size());

    // Use a fill layout to draw the buttons container on
    // top of the image carousel.
    SetUseDefaultFillLayout(true);

    // Screenshots are sanitized by `InstallableManager::OnScreenshotFetched`
    // and should all have the same aspect ratio.
#if DCHECK_IS_ON()
    for (const auto& screenshot : screenshots) {
      DCHECK(screenshot.width() * screenshots_[0].height() ==
             screenshot.height() * screenshots_[0].width());
    }
#endif

    image_padding_ = ChromeLayoutProvider::Get()->GetDistanceMetric(
        views::DISTANCE_RELATED_BUTTON_HORIZONTAL);
    image_container_ = AddChildView(std::make_unique<views::View>());

    image_inner_container_ = image_container_->AddChildView(
        std::make_unique<views::BoxLayoutView>());
    image_inner_container_->SetBetweenChildSpacing(image_padding_);

    for (size_t i = 0; i < screenshots_.size(); i++) {
      image_views_.push_back(image_inner_container_->AddChildView(
          std::make_unique<views::ImageView>()));
    }

    image_container_->SetLayoutManager(
        std::make_unique<ImageCarouselLayoutManager>());

    bounds_animator_ =
        std::make_unique<views::BoundsAnimator>(image_container_, false);
    bounds_animator_->SetAnimationDuration(base::Seconds(0.5));

    auto leading_button_container = std::make_unique<views::BoxLayoutView>();

    leading_button_container->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);
    leading_button_ = leading_button_container->AddChildView(CreateScrollButton(
        ButtonType::LEADING,
        base::BindRepeating(&ImageCarouselView::OnScrollButtonClicked,
                            base::Unretained(this), ButtonType::LEADING)));
    leading_button_container_ =
        AddChildView(std::move(leading_button_container));
    leading_button_->SetVisible(false);

    auto trailing_button_container = std::make_unique<views::BoxLayoutView>();
    trailing_button_container->SetCrossAxisAlignment(
        views::BoxLayout::CrossAxisAlignment::kCenter);

    trailing_button_ =
        trailing_button_container->AddChildView(CreateScrollButton(
            ButtonType::TRAILING,
            base::BindRepeating(&ImageCarouselView::OnScrollButtonClicked,
                                base::Unretained(this), ButtonType::TRAILING)));
    trailing_button_container_ =
        AddChildView(std::move(trailing_button_container));
  }

  void AddedToWidget() override {
    const display::Screen* const screen = display::Screen::GetScreen();

    float current_scale =
        screen->GetDisplayNearestView(GetWidget()->GetNativeView())
            .device_scale_factor();
    for (size_t i = 0; i < screenshots_.size(); i++) {
      image_views_[i]->SetImage(ui::ImageModel::FromImageSkia(
          gfx::ImageSkia::CreateFromBitmap(screenshots_[i], current_scale)));
    }
  }

  void Layout() override {
    // Use a fixed height that guarantees to fit the screenshot with max ratio
    // and still show a clip for the next screenshot.
    const int fixed_height = base::checked_cast<int>(
        base::checked_cast<float>(width() - image_padding_ * 2) /
        webapps::kMaximumScreenshotRatio);
    image_container_->SetBounds(0, 0, width(), height());

    // Only setup the initial visibility and screenshots size once based on
    // container width & max screenshot ratio, the visibility is later updated
    // by `OnScrollButtonClicked` based on image carousel animation.
    if (!trailing_button_visibility_set_up_) {
      for (size_t i = 0; i < screenshots_.size(); i++) {
        const int item_width = base::checked_cast<int>(
            screenshots_[i].width() * (base::checked_cast<float>(fixed_height) /
                                       screenshots_[i].height()));
        image_views_[i]->SetImageSize({item_width, fixed_height});
      }
      image_carousel_full_width_ =
          image_inner_container_->GetPreferredSize().width();
      trailing_button_->SetVisible(image_carousel_full_width_ > width());
      trailing_button_visibility_set_up_ = true;
    }

    leading_button_container_->SetBounds(kSpacingBetweenImages, 0, kIconSize,
                                         fixed_height);

    trailing_button_container_->SetBounds(
        width() - kSpacingBetweenImages - kIconSize, 0, kIconSize,
        fixed_height);
  }

 private:
  enum class ButtonType { LEADING, TRAILING };

  void OnScrollButtonClicked(ButtonType button_type) {
    DCHECK(image_inner_container_->children().size());

    int image_width =
        image_inner_container_->children().front()->bounds().width() +
        image_padding_;
    int container_width = image_container_->bounds().width();

    // Scroll past all the fully visible images
    int delta = image_width * (container_width / image_width);

    if (button_type == ButtonType::TRAILING)
      delta = -delta;

    const gfx::Rect& bounds = image_inner_container_->bounds();
    int x = bounds.x() + delta;

    // Bound the position so there is no empty space drawn before the first
    // image or after last image.
    x = std::min(x, 0);
    x = std::max(x, (container_width - image_carousel_full_width_));

    leading_button_->SetVisible(x < 0);

    trailing_button_->SetVisible(x + image_carousel_full_width_ >
                                 container_width);

    bounds_animator_->AnimateViewTo(
        image_inner_container_,
        gfx::Rect(x, bounds.y(), bounds.width(), bounds.height()));
  }

  std::unique_ptr<views::ImageButton> CreateScrollButton(
      ButtonType button_type,
      views::Button::PressedCallback callback) {
    auto scroll_button = views::CreateVectorImageButton(std::move(callback));
    scroll_button->SetBackground(views::CreateThemedRoundedRectBackground(
        kColorPwaScrollButtonBackground, kIconSize));
    scroll_button->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
    views::HighlightPathGenerator::Install(
        scroll_button.get(),
        std::make_unique<views::CircleHighlightPathGenerator>(gfx::Insets()));

    scroll_button->SetAccessibleName(l10n_util::GetStringUTF16(
        button_type == ButtonType::LEADING
            ? IDS_ACCNAME_WEB_APP_DETAILED_INSTALL_DIALOG_LEADING_SCROLL_BUTTON
            : IDS_ACCNAME_WEB_APP_DETAILED_INSTALL_DIALOG_TRAILING_SCROLL_BUTTON));

    scroll_button->SetImageModel(
        views::Button::ButtonState::STATE_NORMAL,
        button_type == ButtonType::LEADING
            ? ui::ImageModel::FromVectorIcon(kLeadingScrollIcon, ui::kColorIcon)
            : ui::ImageModel::FromVectorIcon(kTrailingScrollIcon,
                                             ui::kColorIcon));

    views::InkDrop::Get(scroll_button.get())
        ->SetBaseColorCallback(base::BindRepeating(
            [](views::ImageButton* host) {
              return host->GetColorProvider()->GetColor(
                  kColorPwaScrollButtonBackground);
            },
            scroll_button.get()));
    views::InkDrop::Get(scroll_button.get())->SetVisibleOpacity(1);
    views::InkDrop::Get(scroll_button.get())->SetHighlightOpacity(1);
    scroll_button->SetFocusBehavior(FocusBehavior::ALWAYS);
    return scroll_button;
  }

  const std::vector<SkBitmap>& screenshots_;
  std::unique_ptr<views::BoundsAnimator> bounds_animator_;
  views::View* image_container_ = nullptr;
  views::BoxLayoutView* image_inner_container_ = nullptr;
  std::vector<views::ImageView*> image_views_;
  views::View* leading_button_ = nullptr;
  views::View* trailing_button_ = nullptr;
  views::View* leading_button_container_ = nullptr;
  views::View* trailing_button_container_ = nullptr;
  int image_carousel_full_width_ = 0;
  int image_padding_ = 0;
  bool trailing_button_visibility_set_up_ = false;
};

}  // namespace

namespace chrome {

void ShowWebAppDetailedInstallDialog(
    content::WebContents* web_contents,
    std::unique_ptr<WebAppInstallInfo> install_info,
    chrome::AppInstallationAcceptanceCallback callback,
    const std::vector<SkBitmap>& screenshots,
    chrome::PwaInProductHelpState iph_state) {
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  PrefService* const prefs =
      Profile::FromBrowserContext(browser_context)->GetPrefs();

  feature_engagement::Tracker* const tracker =
      feature_engagement::TrackerFactory::GetForBrowserContext(browser_context);

  gfx::ImageSkia icon_image(std::make_unique<WebAppInfoImageSource>(
                                kIconSize, install_info->icon_bitmaps.any),
                            gfx::Size(kIconSize, kIconSize));

  auto title = install_info->title;
  auto description = install_info->description;

  auto delegate =
      std::make_unique<web_app::WebAppDetailedInstallDialogDelegate>(
          web_contents, std::move(install_info), std::move(callback),
          std::move(iph_state), prefs, tracker);
  auto* delegate_ptr = delegate.get();
  auto dialog_model =
      ui::DialogModel::Builder(std::move(delegate))
          .SetIcon(ui::ImageModel::FromImageSkia(icon_image))
          .SetTitle(title)  // TODO(pbos): Add secondary-title support for
                            // base::UTF8ToUTF16(install_info->start_url.host())
          .AddBodyText(ui::DialogModelLabel(description))
          .AddOkButton(
              base::BindOnce(
                  &web_app::WebAppDetailedInstallDialogDelegate::OnAccept,
                  base::Unretained(delegate_ptr)),
              l10n_util::GetStringUTF16(IDS_INSTALL))
          .AddCancelButton(base::BindOnce(
              &web_app::WebAppDetailedInstallDialogDelegate::OnCancel,
              base::Unretained(delegate_ptr)))
          .AddCustomField(
              std::make_unique<views::BubbleDialogModelHost::CustomView>(
                  std::make_unique<ImageCarouselView>(screenshots),
                  views::BubbleDialogModelHost::FieldType::kControl))
          .Build();

  auto dialog = views::BubbleDialogModelHost::CreateModal(
      std::move(dialog_model), ui::MODAL_TYPE_CHILD);

  constrained_window::ShowWebModalDialogViews(dialog.release(), web_contents);
  base::RecordAction(base::UserMetricsAction("WebAppDetailedInstallShown"));
}

}  // namespace chrome

namespace web_app {

WebAppDetailedInstallDialogDelegate::WebAppDetailedInstallDialogDelegate(
    content::WebContents* web_contents,
    std::unique_ptr<WebAppInstallInfo> web_app_info,
    chrome::AppInstallationAcceptanceCallback callback,
    chrome::PwaInProductHelpState iph_state,
    PrefService* prefs,
    feature_engagement::Tracker* tracker)
    : web_contents_(web_contents),
      install_info_(std::move(web_app_info)),
      callback_(std::move(callback)),
      iph_state_(std::move(iph_state)),
      prefs_(prefs),
      tracker_(tracker) {
  DCHECK(install_info_);
  DCHECK(prefs_);
}

WebAppDetailedInstallDialogDelegate::~WebAppDetailedInstallDialogDelegate() {
  // TODO(crbug.com/1327363): move this to dialog->SetHighlightedButton.
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  if (!browser)
    return;

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  // Dehighlight the install icon when this dialog is closed.
  browser_view->toolbar_button_provider()
      ->GetPageActionIconView(PageActionIconType::kPwaInstall)
      ->SetHighlighted(false);
}

void WebAppDetailedInstallDialogDelegate::OnAccept() {
  base::RecordAction(base::UserMetricsAction("WebAppDetailedInstallAccepted"));
  if (iph_state_ == chrome::PwaInProductHelpState::kShown) {
    web_app::AppId app_id = web_app::GenerateAppId(install_info_->manifest_id,
                                                   install_info_->start_url);
    web_app::RecordInstallIphInstalled(prefs_, app_id);
    tracker_->NotifyEvent(feature_engagement::events::kDesktopPwaInstalled);
  }

  std::move(callback_).Run(true, std::move(install_info_));
}

void WebAppDetailedInstallDialogDelegate::OnCancel() {
  base::RecordAction(base::UserMetricsAction("WebAppDetailedInstallCancelled"));
  if (iph_state_ == chrome::PwaInProductHelpState::kShown && install_info_) {
    web_app::AppId app_id = web_app::GenerateAppId(install_info_->manifest_id,
                                                   install_info_->start_url);
    web_app::RecordInstallIphIgnored(prefs_, app_id, base::Time::Now());
  }

  std::move(callback_).Run(false, std::move(install_info_));
}

}  // namespace web_app

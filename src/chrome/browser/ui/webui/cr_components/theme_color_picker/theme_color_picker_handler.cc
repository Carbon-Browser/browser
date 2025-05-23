// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"

#include <limits>

#include "base/containers/fixed_flat_map.h"
#include "chrome/browser/new_tab_page/chrome_colors/generated_colors_info.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/customize_chrome_colors.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/themes/autogenerated_theme_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/mojom/themes.mojom.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/dynamic_color/palette_factory.h"
#include "ui/native_theme/native_theme.h"

namespace {

ui::ColorProviderKey::SchemeVariant GetSchemeVariant(
    ui::mojom::BrowserColorVariant color_variant) {
  using BCV = ui::mojom::BrowserColorVariant;
  using SV = ui::ColorProviderKey::SchemeVariant;
  static constexpr auto kSchemeVariantMap = base::MakeFixedFlatMap<BCV, SV>({
      {BCV::kTonalSpot, SV::kTonalSpot},
      {BCV::kNeutral, SV::kNeutral},
      {BCV::kVibrant, SV::kVibrant},
      {BCV::kExpressive, SV::kExpressive},
  });
  return kSchemeVariantMap.at(color_variant);
}

// Create a GM3 chrome color that takes color scheme into account.
theme_color_picker::mojom::ChromeColorPtr CreateDynamicChromeColor(
    DynamicColorInfo color_info,
    bool is_dark_mode) {
  auto color = theme_color_picker::mojom::ChromeColor::New();
  auto color_palette = ui::GeneratePalette(
      color_info.color, GetSchemeVariant(color_info.variant));
  color->name = l10n_util::GetStringUTF8(color_info.label_id);
  color->seed = color_info.color;
  color->background = is_dark_mode ? color_palette->primary().get(80)
                                   : color_palette->primary().get(40);
  color->foreground = is_dark_mode ? color_palette->primary().get(30)
                                   : color_palette->primary().get(90);
  color->base = is_dark_mode ? color_palette->secondary().get(50)
                             : color_palette->primary().get(80);
  color->variant = color_info.variant;
  return color;
}
}  // namespace

ThemeColorPickerHandler::ThemeColorPickerHandler(
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        pending_handler,
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        pending_client,
    NtpCustomBackgroundService* ntp_custom_background_service,
    content::WebContents* web_contents)
    : ntp_custom_background_service_(ntp_custom_background_service),
      profile_(Profile::FromBrowserContext(web_contents->GetBrowserContext())),
      web_contents_(web_contents),
      theme_service_(ThemeServiceFactory::GetForProfile(profile_)),
      client_(std::move(pending_client)),
      receiver_(this, std::move(pending_handler)) {
  CHECK(ntp_custom_background_service_);
  CHECK(theme_service_);
  native_theme_observation_.Observe(ui::NativeTheme::GetInstanceForNativeUi());
  theme_service_observation_.Observe(theme_service_);

  ntp_custom_background_service_observation_.Observe(
      ntp_custom_background_service_.get());
}

ThemeColorPickerHandler::~ThemeColorPickerHandler() = default;

// static
void ThemeColorPickerHandler::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kSeedColorChangeCount, 0);
}

void ThemeColorPickerHandler::SetDefaultColor() {
  theme_service_->SetUserColor(SK_ColorTRANSPARENT);
  theme_service_->UseDeviceTheme(false);
  MaybeIncrementSeedColorChangeCount();
}

void ThemeColorPickerHandler::SetGreyDefaultColor() {
  theme_service_->SetIsGrayscale(true);
  theme_service_->UseDeviceTheme(false);
  MaybeIncrementSeedColorChangeCount();
}

void ThemeColorPickerHandler::SetSeedColor(
    SkColor seed_color,
    ui::mojom::BrowserColorVariant variant) {
  theme_service_->SetUserColorAndBrowserColorVariant(seed_color, variant);
  theme_service_->UseDeviceTheme(false);
  MaybeIncrementSeedColorChangeCount();
}

void ThemeColorPickerHandler::SetSeedColorFromHue(float hue) {
  SetSeedColor(HueToSkColor(hue), ui::mojom::BrowserColorVariant::kTonalSpot);
}

void ThemeColorPickerHandler::GetChromeColors(
    bool is_dark_mode,
    GetChromeColorsCallback callback) {
  std::vector<theme_color_picker::mojom::ChromeColorPtr> colors;
  for (const auto& color_info : kDynamicCustomizeChromeColors) {
    colors.push_back(CreateDynamicChromeColor(color_info, is_dark_mode));
  }
  std::move(callback).Run(std::move(colors));
}

void ThemeColorPickerHandler::RemoveBackgroundImage() {
  if (ntp_custom_background_service_) {
    ntp_custom_background_service_->ResetCustomBackgroundInfo();
  }
}

void ThemeColorPickerHandler::UpdateTheme() {
  if (ntp_custom_background_service_) {
    ntp_custom_background_service_->RefreshBackgroundIfNeeded();
  }
  auto theme = theme_color_picker::mojom::Theme::New();
  auto custom_background =
      ntp_custom_background_service_
          ? ntp_custom_background_service_->GetCustomBackground()
          : std::nullopt;
  theme->has_background_image = custom_background.has_value();
  if (custom_background.has_value() &&
      custom_background->custom_background_main_color.has_value()) {
    theme->background_image_main_color =
        *custom_background->custom_background_main_color;
  }

  theme->follow_device_theme = theme_service_->UsingDeviceTheme();

  auto* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
  CHECK(native_theme);

  auto user_color = theme->follow_device_theme ? native_theme->user_color()
                                               : theme_service_->GetUserColor();
  // If a user has a GM2 theme set they are in a limbo state between the 2 theme
  // types. We need to get the color of their theme with
  // GetAutogeneratedThemeColor still until they set a GM3 theme, use the old
  // way of detecting default, and use the old color tokens to keep an accurate
  // representation of what the user is seeing.
  if (user_color.has_value()) {
    theme->seed_color = user_color.value();
    theme->background_color =
        web_contents_->GetColorProvider().GetColor(ui::kColorSysInversePrimary);
    theme->color_picker_icon_color =
        web_contents_->GetColorProvider().GetColor(ui::kColorSysOnSurface);
    if (theme->seed_color != SK_ColorTRANSPARENT) {
      theme->foreground_color = theme->background_color;
    }
  } else {
    theme->seed_color = theme_service_->GetAutogeneratedThemeColor();
    theme->background_color =
        web_contents_->GetColorProvider().GetColor(kColorNewTabPageBackground);
    if (!theme_service_->UsingDefaultTheme() &&
        !theme_service_->UsingSystemTheme()) {
      theme->foreground_color =
          web_contents_->GetColorProvider().GetColor(ui::kColorFrameActive);
    }
    theme->color_picker_icon_color =
        web_contents_->GetColorProvider().GetColor(kColorNewTabPageText);
  }
  color_utils::HSL hsl;
  color_utils::SkColorToHSL(theme->seed_color, &hsl);
  theme->seed_color_hue = hsl.h * 360;
  theme->is_grey_baseline = theme_service_->GetIsGrayscale();
  theme->browser_color_variant = theme_service_->GetBrowserColorVariant();

  if (!theme_service_->UsingDefaultTheme() &&
      !theme_service_->UsingSystemTheme()) {
    theme->colors_managed_by_policy = theme_service_->UsingPolicyTheme();
  } else {
    theme->colors_managed_by_policy = false;
  }
  theme->has_third_party_theme = theme_service_->UsingExtensionTheme();

  // If BrowserColorScheme is not set to follow the system, use
  // BrowserColorScheme for deciding dark mode boolean. Otherwise, use the
  // system value.
  ThemeService::BrowserColorScheme colorScheme =
      theme_service_->GetBrowserColorScheme();
  if (colorScheme != ThemeService::BrowserColorScheme::kSystem) {
    theme->is_dark_mode =
        colorScheme == ThemeService::BrowserColorScheme::kDark;
  } else {
    theme->is_dark_mode = native_theme->ShouldUseDarkColors();
  }
  client_->SetTheme(std::move(theme));
}

void ThemeColorPickerHandler::MaybeIncrementSeedColorChangeCount() {
  if (!seed_color_changed_) {
    const auto count =
        profile_->GetPrefs()->GetInteger(prefs::kSeedColorChangeCount);
    if (count < INT_MAX) {
      profile_->GetPrefs()->SetInteger(prefs::kSeedColorChangeCount, count + 1);
    }

    seed_color_changed_ = true;
  }
}

void ThemeColorPickerHandler::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  UpdateTheme();
}

void ThemeColorPickerHandler::OnThemeChanged() {
  UpdateTheme();
}

void ThemeColorPickerHandler::OnCustomBackgroundImageUpdated() {
  OnThemeChanged();
}

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/glic_button.h"

#include "base/functional/bind.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/user_education/browser_user_education_interface.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/common/buildflags.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/view_class_properties.h"

#if BUILDFLAG(ENABLE_GLIC)
#include "chrome/browser/glic/glic_keyed_service.h"
#include "chrome/browser/glic/glic_keyed_service_factory.h"
#endif  // BUILDFLAG(ENABLE_GLIC)

namespace glic {

GlicButton::GlicButton(TabStripController* tab_strip_controller)
    : TabStripControlButton(
          tab_strip_controller,
          PressedCallback(base::BindRepeating(&GlicButton::LaunchUI,
                                              base::Unretained(this))),
          kGlicButtonIcon) {
  tab_strip_controller_ = tab_strip_controller;
  SetProperty(views::kElementIdentifierKey, kGlicButtonElementId);

  // TODO(iwells): Replace the values here, values are required to compile.
  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_TAB_SEARCH));
  GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_TAB_SEARCH));

  SetForegroundFrameActiveColorId(kColorNewTabButtonForegroundFrameActive);
  SetForegroundFrameInactiveColorId(kColorNewTabButtonForegroundFrameInactive);
  SetBackgroundFrameActiveColorId(kColorNewTabButtonCRBackgroundFrameActive);
  SetBackgroundFrameInactiveColorId(
      kColorNewTabButtonCRBackgroundFrameInactive);

  UpdateColors();
}

GlicButton::~GlicButton() = default;

void GlicButton::LaunchUI() {
  // Indicate that the glic button was pressed so that we can either close the
  // IPH promo (if present) or note that it has already been used to prevent
  // unnecessarily displaying the promo.
  tab_strip_controller_->GetBrowserWindowInterface()
      ->GetUserEducationInterface()
      ->NotifyFeaturePromoFeatureUsed(
          feature_engagement::kIPHGlicPromoFeature,
          FeaturePromoFeatureUsedAction::kClosePromoIfPresent);

#if BUILDFLAG(ENABLE_GLIC)
  glic::GlicKeyedServiceFactory::GetGlicKeyedService(
      tab_strip_controller_->GetProfile())
      ->LaunchUI(this);
#endif  // BUILDFLAG(ENABLE_GLIC)
}

BEGIN_METADATA(GlicButton)
END_METADATA

}  // namespace glic

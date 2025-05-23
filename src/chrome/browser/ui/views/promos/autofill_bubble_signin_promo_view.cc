// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/promos/autofill_bubble_signin_promo_view.h"

#include <memory>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/chrome_signin_pref_names.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/signin_ui_util.h"
#include "chrome/browser/ui/autofill/autofill_bubble_signin_promo_controller.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "chrome/browser/ui/views/promos/bubble_signin_promo_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "components/signin/public/base/signin_metrics.h"
#include "components/signin/public/base/signin_prefs.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/fill_layout.h"

class AutofillBubbleSignInPromoView::DiceSigninPromoDelegate
    : public BubbleSignInPromoDelegate {
 public:
  explicit DiceSigninPromoDelegate(
      autofill::AutofillBubbleSignInPromoController* controller);
  DiceSigninPromoDelegate(const DiceSigninPromoDelegate&) = delete;
  DiceSigninPromoDelegate& operator=(const DiceSigninPromoDelegate&) = delete;
  ~DiceSigninPromoDelegate() override;

  // BubbleSignInPromoDelegate:
  void OnSignIn(const AccountInfo& account) override;

 private:
  raw_ptr<autofill::AutofillBubbleSignInPromoController> controller_;
};

AutofillBubbleSignInPromoView::DiceSigninPromoDelegate::DiceSigninPromoDelegate(
    autofill::AutofillBubbleSignInPromoController* controller)
    : controller_(controller) {
  CHECK(controller_);
}

AutofillBubbleSignInPromoView::DiceSigninPromoDelegate::
    ~DiceSigninPromoDelegate() = default;

void AutofillBubbleSignInPromoView::DiceSigninPromoDelegate::OnSignIn(
    const AccountInfo& account) {
  controller_->OnSignInToChromeClicked(account);
}

AutofillBubbleSignInPromoView::AutofillBubbleSignInPromoView(
    content::WebContents* web_contents,
    signin_metrics::AccessPoint access_point,
    syncer::LocalDataItemModel::DataId data_id)
    : controller_(*web_contents, access_point, std::move(data_id)),
      access_point_(access_point) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  CHECK(AccountConsistencyModeManager::IsDiceEnabledForProfile(profile));
  dice_sign_in_promo_delegate_ =
      std::make_unique<AutofillBubbleSignInPromoView::DiceSigninPromoDelegate>(
          &controller_);

  signin::RecordSignInPromoShown(access_point, profile);

  AddChildView(new BubbleSignInPromoView(
      profile, dice_sign_in_promo_delegate_.get(), access_point));
}

void AutofillBubbleSignInPromoView::AddedToWidget() {
  scoped_widget_observation_.Observe(GetWidget());
}

void AutofillBubbleSignInPromoView::OnWidgetDestroying(views::Widget* widget) {
  scoped_widget_observation_.Reset();
  std::string dismiss_action;

  switch (widget->closed_reason()) {
    case views::Widget::ClosedReason::kCloseButtonClicked:
      dismiss_action = "CloseButton";
      break;
    case views::Widget::ClosedReason::kEscKeyPressed:
      dismiss_action = "EscapeKey";
      break;
    case views::Widget::ClosedReason::kUnspecified:
    case views::Widget::ClosedReason::kLostFocus:
    case views::Widget::ClosedReason::kCancelButtonClicked:
    case views::Widget::ClosedReason::kAcceptButtonClicked:
      // Don't record anything if the bubble was not actively dismissed by the
      // user.
      return;
  }

  Profile* profile = Profile::FromBrowserContext(
      controller_.GetWebContents()->GetBrowserContext());
  AccountInfo account = signin_ui_util::GetSingleAccountForPromos(
      IdentityManagerFactory::GetForProfile(profile));

  // Count the number of times the promo was dismissed in order to not show it
  // anymore after 2 dismissals.
  if (account.gaia.empty()) {
    int dismiss_count = profile->GetPrefs()->GetInteger(
        prefs::kAutofillSignInPromoDismissCountPerProfile);
    profile->GetPrefs()->SetInteger(
        prefs::kAutofillSignInPromoDismissCountPerProfile, dismiss_count + 1);
  } else {
    SigninPrefs(*profile->GetPrefs())
        .IncrementAutofillSigninPromoDismissCount(account.gaia);
  }

  base::UmaHistogramEnumeration(
      base::StrCat({"Signin.SignInPromo.Dismissed", dismiss_action}),
      access_point_, signin_metrics::AccessPoint::ACCESS_POINT_MAX);
}

AutofillBubbleSignInPromoView::~AutofillBubbleSignInPromoView() = default;

BEGIN_METADATA(AutofillBubbleSignInPromoView)
END_METADATA

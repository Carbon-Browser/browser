// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/touch_to_fill_delegate_impl.h"

#include "components/autofill/core/browser/browser_autofill_manager.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_util.h"

namespace autofill {

TouchToFillDelegateImpl::TouchToFillDelegateImpl(
    BrowserAutofillManager* manager)
    : manager_(manager) {
  DCHECK(manager);
}

TouchToFillDelegateImpl::~TouchToFillDelegateImpl() = default;

bool TouchToFillDelegateImpl::TryToShowTouchToFill(int query_id,
                                                   const FormData& form,
                                                   const FormFieldData& field) {
  // Trigger only for a credit card field/form.
  // TODO(crbug.com/1247698): Clarify field/form requirements.
  if (manager_->GetPopupType(form, field) != PopupType::kCreditCards)
    return false;
  // Trigger only on supported platforms.
  if (!manager_->client()->IsTouchToFillCreditCardSupported())
    return false;
  // Trigger only if not shown before.
  if (ttf_credit_card_state_ != TouchToFillState::kShouldShow)
    return false;
  // Trigger only on focusable empty field.
  if (!field.is_focusable || !SanitizedFieldIsEmpty(field.value))
    return false;
  // Trigger only if there is at least 1 complete valid credit card on file.
  // Complete = contains number, expiration date and name on card.
  // Valid = unexpired with valid number format.
  PersonalDataManager* pdm = manager_->client()->GetPersonalDataManager();
  DCHECK(pdm);
  std::vector<CreditCard*> cards_to_suggest = pdm->GetCreditCardsToSuggest(
      manager_->client()->AreServerCardsSupported());
  auto NotACompleteValidCard = [](const CreditCard* card) {
    return card->IsExpired(AutofillClock::Now()) || !card->HasNameOnCard() ||
           (card->record_type() != autofill::CreditCard::MASKED_SERVER_CARD &&
            !card->HasValidCardNumber());
  };
  base::EraseIf(cards_to_suggest, NotACompleteValidCard);
  if (cards_to_suggest.empty())
    return false;
  // Trigger only if the UI is available.
  if (!manager_->driver()->CanShowAutofillUi())
    return false;
  // Finally try showing the surface.
  if (!manager_->client()->ShowTouchToFillCreditCard(GetWeakPtr()))
    return false;

  ttf_credit_card_state_ = TouchToFillState::kIsShowing;
  manager_->client()->HideAutofillPopup(
      PopupHidingReason::kOverlappingWithTouchToFillSurface);
  return true;
}

bool TouchToFillDelegateImpl::IsShowingTouchToFill() {
  return ttf_credit_card_state_ == TouchToFillState::kIsShowing;
}

// TODO(crbug.com/1247698): Review |HideAutofillPopup| invocations and maybe
// also trigger |HideTouchToFillCreditCard|.
void TouchToFillDelegateImpl::HideTouchToFill() {
  if (IsShowingTouchToFill()) {
    manager_->client()->HideTouchToFillCreditCard();
    ttf_credit_card_state_ = TouchToFillState::kWasShown;
  }
}

void TouchToFillDelegateImpl::Reset() {
  HideTouchToFill();
  ttf_credit_card_state_ = TouchToFillState::kShouldShow;
}

base::WeakPtr<TouchToFillDelegateImpl> TouchToFillDelegateImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace autofill

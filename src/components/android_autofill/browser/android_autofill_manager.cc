// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/android_autofill/browser/android_autofill_manager.h"

#include "base/memory/ptr_util.h"
#include "components/android_autofill/browser/autofill_provider.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace autofill {

using base::TimeTicks;

void AndroidDriverInitHook(
    AutofillClient* client,
    AutofillManager::EnableDownloadManager enable_download_manager,
    ContentAutofillDriver* driver) {
  driver->set_autofill_manager(base::WrapUnique(
      new AndroidAutofillManager(driver, client, enable_download_manager)));
  driver->GetAutofillAgent()->SetUserGestureRequired(false);
  driver->GetAutofillAgent()->SetSecureContextRequired(true);
  driver->GetAutofillAgent()->SetFocusRequiresScroll(false);
  driver->GetAutofillAgent()->SetQueryPasswordSuggestion(true);
}

AndroidAutofillManager::AndroidAutofillManager(
    AutofillDriver* driver,
    AutofillClient* client,
    EnableDownloadManager enable_download_manager)
    : AutofillManager(driver,
                      client,
                      version_info::Channel::UNKNOWN,
                      enable_download_manager) {}

AndroidAutofillManager::~AndroidAutofillManager() = default;

base::WeakPtr<AutofillManager> AndroidAutofillManager::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

AutofillOfferManager* AndroidAutofillManager::GetOfferManager() {
  return nullptr;
}

CreditCardAccessManager* AndroidAutofillManager::GetCreditCardAccessManager() {
  return nullptr;
}

bool AndroidAutofillManager::ShouldClearPreviewedForm() {
  return false;
}

void AndroidAutofillManager::FillCreditCardFormImpl(
    const FormData& form,
    const FormFieldData& field,
    const CreditCard& credit_card,
    const std::u16string& cvc,
    int query_id) {
  NOTREACHED();
}

void AndroidAutofillManager::FillProfileFormImpl(
    const FormData& form,
    const FormFieldData& field,
    const autofill::AutofillProfile& profile) {
  NOTREACHED();
}

void AndroidAutofillManager::SetProfileFillViaAutofillAssistantIntent(
    const autofill_assistant::AutofillAssistantIntent intent) {
  NOTREACHED();
}

void AndroidAutofillManager::SetCreditCardFillViaAutofillAssistantIntent(
    const autofill_assistant::AutofillAssistantIntent intent) {
  NOTREACHED();
}

void AndroidAutofillManager::OnFormSubmittedImpl(
    const FormData& form,
    bool known_success,
    mojom::SubmissionSource source) {
  if (auto* provider = GetAutofillProvider())
    provider->OnFormSubmitted(this, form, known_success, source);
}

void AndroidAutofillManager::OnTextFieldDidChangeImpl(
    const FormData& form,
    const FormFieldData& field,
    const gfx::RectF& bounding_box,
    const TimeTicks timestamp) {
  if (auto* provider = GetAutofillProvider())
    provider->OnTextFieldDidChange(this, form, field, bounding_box, timestamp);
}

void AndroidAutofillManager::OnTextFieldDidScrollImpl(
    const FormData& form,
    const FormFieldData& field,
    const gfx::RectF& bounding_box) {
  if (auto* provider = GetAutofillProvider())
    provider->OnTextFieldDidScroll(this, form, field, bounding_box);
}

void AndroidAutofillManager::OnAskForValuesToFillImpl(
    const FormData& form,
    const FormFieldData& field,
    const gfx::RectF& bounding_box,
    int query_id,
    bool autoselect_first_suggestion,
    TouchToFillEligible touch_to_fill_eligible) {
  if (auto* provider = GetAutofillProvider()) {
    provider->OnAskForValuesToFill(this, form, field, bounding_box, query_id,
                                   autoselect_first_suggestion,
                                   touch_to_fill_eligible);
  }
}

void AndroidAutofillManager::OnFocusOnFormFieldImpl(
    const FormData& form,
    const FormFieldData& field,
    const gfx::RectF& bounding_box) {
  if (auto* provider = GetAutofillProvider())
    provider->OnFocusOnFormField(this, form, field, bounding_box);
}

void AndroidAutofillManager::OnSelectControlDidChangeImpl(
    const FormData& form,
    const FormFieldData& field,
    const gfx::RectF& bounding_box) {
  if (auto* provider = GetAutofillProvider())
    provider->OnSelectControlDidChange(this, form, field, bounding_box);
}

bool AndroidAutofillManager::ShouldParseForms(
    const std::vector<FormData>& forms) {
  if (auto* provider = GetAutofillProvider())
    provider->OnFormsSeen(this, forms);
  // Need to parse the |forms| to FormStructure, so heuristic_type can be
  // retrieved later.
  return true;
}

void AndroidAutofillManager::OnFocusNoLongerOnFormImpl(
    bool had_interacted_form) {
  if (auto* provider = GetAutofillProvider())
    provider->OnFocusNoLongerOnForm(this, had_interacted_form);
}

void AndroidAutofillManager::OnDidFillAutofillFormDataImpl(
    const FormData& form,
    const base::TimeTicks timestamp) {
  if (auto* provider = GetAutofillProvider())
    provider->OnDidFillAutofillFormData(this, form, timestamp);
}

void AndroidAutofillManager::OnHidePopupImpl() {
  if (auto* provider = GetAutofillProvider())
    provider->OnHidePopup(this);
}

void AndroidAutofillManager::PropagateAutofillPredictions(
    const std::vector<FormStructure*>& forms) {
  has_server_prediction_ = true;
  if (auto* provider = GetAutofillProvider())
    provider->OnServerPredictionsAvailable(this);
}

void AndroidAutofillManager::OnServerRequestError(
    FormSignature form_signature,
    AutofillDownloadManager::RequestType request_type,
    int http_error) {
  if (auto* provider = GetAutofillProvider())
    provider->OnServerQueryRequestError(this, form_signature);
}

void AndroidAutofillManager::Reset() {
  AutofillManager::Reset();
  has_server_prediction_ = false;
  if (auto* provider = GetAutofillProvider())
    provider->Reset(this);
}

AutofillProvider* AndroidAutofillManager::GetAutofillProvider() {
  if (autofill_provider_for_testing_)
    return autofill_provider_for_testing_;
  if (auto* rfh =
          static_cast<ContentAutofillDriver*>(driver())->render_frame_host()) {
    if (rfh->IsActive()) {
      if (auto* web_contents = content::WebContents::FromRenderFrameHost(rfh)) {
        return AutofillProvider::FromWebContents(web_contents);
      }
    }
  }
  return nullptr;
}

void AndroidAutofillManager::FillOrPreviewForm(
    int query_id,
    mojom::RendererFormDataAction action,
    const FormData& form) {
  driver()->FillOrPreviewForm(query_id, action, form, form.main_frame_origin,
                              {});
}

}  // namespace autofill

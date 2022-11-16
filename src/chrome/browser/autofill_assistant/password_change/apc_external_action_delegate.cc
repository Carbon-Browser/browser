// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill_assistant/password_change/apc_external_action_delegate.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill_assistant/password_change/apc_scrim_manager.h"
#include "chrome/browser/ui/autofill_assistant/password_change/assistant_display_delegate.h"
#include "chrome/browser/ui/autofill_assistant/password_change/password_change_run_controller.h"
#include "chrome/browser/ui/autofill_assistant/password_change/password_change_run_display.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill_assistant/browser/public/external_action.pb.h"
#include "components/autofill_assistant/browser/public/external_action_delegate.h"
#include "components/autofill_assistant/browser/public/password_change/proto/actions.pb.h"
#include "components/autofill_assistant/browser/public/rectf.h"
#include "components/url_formatter/url_formatter.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using autofill_assistant::password_change::GenericPasswordChangeSpecification;

ApcExternalActionDelegate::ApcExternalActionDelegate(
    AssistantDisplayDelegate* display_delegate,
    ApcScrimManager* apc_scrim_manager)
    : display_delegate_(display_delegate),
      apc_scrim_manager_(apc_scrim_manager) {
  DCHECK(display_delegate_);
  DCHECK(apc_scrim_manager_);
}

ApcExternalActionDelegate::~ApcExternalActionDelegate() = default;

void ApcExternalActionDelegate::OnActionRequested(
    const autofill_assistant::external::Action& action,
    base::OnceCallback<void(DomUpdateCallback)> start_dom_checks_callback,
    base::OnceCallback<void(const autofill_assistant::external::Result& result)>
        end_action_callback) {
  end_action_callback_ = std::move(end_action_callback);
  start_dom_checks_callback_ = std::move(start_dom_checks_callback);

  if (!action.info().has_generic_password_change_specification()) {
    DLOG(ERROR) << "Action is not of type GenericPasswordChangeSpecification";
    EndAction(false);
    return;
  }
  GenericPasswordChangeSpecification generic_password_change_specification =
      action.info().generic_password_change_specification();
  switch (generic_password_change_specification.specification_case()) {
    case GenericPasswordChangeSpecification::SpecificationCase::kBasePrompt:
      HandleBasePrompt(generic_password_change_specification.base_prompt());
      break;
    case GenericPasswordChangeSpecification::SpecificationCase::
        kUseGeneratedPasswordPrompt:
      HandleGeneratedPasswordPrompt(generic_password_change_specification
                                        .use_generated_password_prompt());
      break;
    case GenericPasswordChangeSpecification::SpecificationCase::
        kUpdateSidePanel:
      HandleUpdateSidePanel(
          generic_password_change_specification.update_side_panel());
      break;
    case GenericPasswordChangeSpecification::SpecificationCase::
        SPECIFICATION_NOT_SET:
      DLOG(ERROR) << "unknown password change action";
      EndAction(false);
      break;
  }
}

void ApcExternalActionDelegate::SetupDisplay() {
  Show(PasswordChangeRunDisplay::Create(GetWeakPtr(), display_delegate_.get()));
}

void ApcExternalActionDelegate::OnInterruptStarted() {
  DCHECK(!model_before_interrupt_.has_value());
  model_before_interrupt_ = model_;

  // Reset the current model. The progress step remains the same, so we do not
  // touch it.
  SetTitle(std::u16string());
  SetDescription(std::u16string());
}

void ApcExternalActionDelegate::OnInterruptFinished() {
  DCHECK(model_before_interrupt_.has_value());

  // Restore the state from prior to the interrupt. We reset the model
  // by calling the setters instead of just restoring state to ensure that
  // the view is informed about the updates.
  PasswordChangeRunController::Model model = model_before_interrupt_.value();
  SetTopIcon(model.top_icon);
  SetTitle(model.title);
  SetDescription(model.description);

  model_before_interrupt_.reset();
}

void ApcExternalActionDelegate::OnTouchableAreaChanged(
    const autofill_assistant::RectF& visual_viewport,
    const std::vector<autofill_assistant::RectF>& touchable_areas,
    const std::vector<autofill_assistant::RectF>& restricted_areas) {
  if (!touchable_areas.empty()) {
    apc_scrim_manager_->Hide();
  } else {
    apc_scrim_manager_->Show();
  }
}

// PasswordChangeRunController
void ApcExternalActionDelegate::SetTopIcon(
    autofill_assistant::password_change::TopIcon top_icon) {
  DCHECK(password_change_run_display_);
  model_.top_icon = top_icon;
  password_change_run_display_->SetTopIcon(top_icon);
}

void ApcExternalActionDelegate::SetTitle(const std::u16string& title) {
  DCHECK(password_change_run_display_);
  model_.title = title;
  password_change_run_display_->SetTitle(title);
}

void ApcExternalActionDelegate::SetDescription(
    const std::u16string& description) {
  DCHECK(password_change_run_display_);
  model_.description = description;
  password_change_run_display_->SetDescription(description);
}

void ApcExternalActionDelegate::SetProgressBarStep(
    autofill_assistant::password_change::ProgressStep progress_step) {
  DCHECK(password_change_run_display_);
  model_.progress_step = progress_step;
  password_change_run_display_->SetProgressBarStep(progress_step);
}

void ApcExternalActionDelegate::ShowBasePrompt(
    const autofill_assistant::password_change::BasePromptSpecification&
        base_prompt) {
  // Showing the prompt will override the description, so set the model value
  // to empty to ensure that it reflects the state of the view.
  model_.description = std::u16string();

  std::vector<PasswordChangeRunDisplay::PromptChoice> choices;
  choices.reserve(base_prompt.choices_size());
  base_prompt_return_values_.clear();
  base_prompt_return_values_.reserve(base_prompt.choices_size());

  for (const auto& choice : base_prompt.choices()) {
    choices.push_back(PasswordChangeRunDisplay::PromptChoice{
        .text = base::UTF8ToUTF16(choice.text()),
        .highlighted = choice.highlighted()});
    base_prompt_return_values_.push_back(choice.tag());
  }

  SetTitle(base::UTF8ToUTF16(base_prompt.title()));
  password_change_run_display_->ShowBasePrompt(choices);
}

void ApcExternalActionDelegate::OnBasePromptChoiceSelected(
    size_t choice_index) {
  password_change_run_display_->ClearPrompt();

  // If no `output_key` is specified, only signal that the prompt action was
  // successfully executed.
  if (!base_prompt_should_send_payload_) {
    EndAction(true);
    return;
  }

  CHECK(choice_index < base_prompt_return_values_.size());
  autofill_assistant::password_change::BasePromptSpecification::Result
      base_prompt_result;
  base_prompt_result.set_selected_tag(base_prompt_return_values_[choice_index]);

  autofill_assistant::password_change::GenericPasswordChangeSpecificationResult
      action_result;
  *action_result.mutable_base_prompt_result() = base_prompt_result;
  EndAction(true, std::move(action_result));
}

void ApcExternalActionDelegate::ShowUseGeneratedPasswordPrompt(
    const autofill_assistant::password_change::
        UseGeneratedPasswordPromptSpecification& password_prompt,
    const std::u16string& generated_password) {
  // Showing the prompt will override both the title and the description. Since
  // they cannot be reconstructed from the model due to the additional field
  // for the password, we clear the model.
  model_.title = std::u16string();
  model_.description = std::u16string();
  password_change_run_display_->ShowUseGeneratedPasswordPrompt(
      base::UTF8ToUTF16(password_prompt.title()), generated_password,
      base::UTF8ToUTF16(password_prompt.description()),
      PasswordChangeRunDisplay::PromptChoice{
          .text = base::UTF8ToUTF16(
              password_prompt.manual_password_choice().text()),
          .highlighted =
              password_prompt.manual_password_choice().highlighted()},
      PasswordChangeRunDisplay::PromptChoice{
          .text = base::UTF8ToUTF16(
              password_prompt.generated_password_choice().text()),
          .highlighted =
              password_prompt.generated_password_choice().highlighted()});
}

void ApcExternalActionDelegate::OnGeneratedPasswordSelected(
    bool generated_password_accepted) {
  password_change_run_display_->ClearPrompt();
  SetTitle(std::u16string());

  autofill_assistant::password_change::UseGeneratedPasswordPromptSpecification::
      Result generated_password_prompt_result;
  generated_password_prompt_result.set_generated_password_accepted(
      generated_password_accepted);

  autofill_assistant::password_change::GenericPasswordChangeSpecificationResult
      action_result;
  *action_result.mutable_use_generated_password_prompt_result() =
      generated_password_prompt_result;

  EndAction(true, std::move(action_result));
}

void ApcExternalActionDelegate::ShowStartingScreen(const GURL& url) {
  SetTopIcon(
      autofill_assistant::password_change::TopIcon::TOP_ICON_UNSPECIFIED);
  SetProgressBarStep(
      autofill_assistant::password_change::ProgressStep::PROGRESS_STEP_START);

  const std::u16string formatted_url = url_formatter::FormatUrl(
      url,
      url_formatter::kFormatUrlOmitHTTP | url_formatter::kFormatUrlOmitHTTPS |
          url_formatter::kFormatUrlOmitTrivialSubdomains |
          url_formatter::kFormatUrlTrimAfterHost,
      base::UnescapeRule::SPACES, /*new_parsed=*/nullptr,
      /*prefix_end=*/nullptr, /*offset_for_adjustment=*/nullptr);
  SetTitle(l10n_util::GetStringFUTF16(
      IDS_AUTOFILL_ASSISTANT_PASSWORD_CHANGE_STARTING_SCREEN_TITLE,
      formatted_url));
  SetDescription(std::u16string());
}

void ApcExternalActionDelegate::Show(
    base::WeakPtr<PasswordChangeRunDisplay> password_change_run_display) {
  password_change_run_display_ = password_change_run_display;
  password_change_run_display_->Show();
}

void ApcExternalActionDelegate::EndAction(
    bool success,
    absl::optional<autofill_assistant::password_change::
                       GenericPasswordChangeSpecificationResult>
        action_result) {
  autofill_assistant::external::Result result;
  result.set_success(success);

  if (action_result.has_value()) {
    *result.mutable_result_info()
         ->mutable_generic_password_change_specification_result() =
        action_result.value();
  }
  std::move(end_action_callback_).Run(std::move(result));
}

void ApcExternalActionDelegate::HandleBasePrompt(
    const autofill_assistant::password_change::BasePromptSpecification&
        specification) {
  base_prompt_should_send_payload_ = specification.has_output_key();

  // TODO(crbug.com/1331202): If this causes flickering, separate prompt
  // argument extraction and showing the base prompt.
  ShowBasePrompt(specification);

  // `this` outlives the script controller, therefore we can pass an unretained
  // pointer.
  std::move(start_dom_checks_callback_)
      .Run(base::BindRepeating(
          &ApcExternalActionDelegate::OnBasePromptDomUpdateReceived,
          base::Unretained(this)));
}

void ApcExternalActionDelegate::HandleGeneratedPasswordPrompt(
    const autofill_assistant::password_change::
        UseGeneratedPasswordPromptSpecification& specification) {
  // TODO(crbug.com/1331202): Replace this hardcoded password with the real
  // generated one.
  ShowUseGeneratedPasswordPrompt(specification, u"verySecretPassword123");
}

void ApcExternalActionDelegate::HandleUpdateSidePanel(
    const autofill_assistant::password_change::UpdateSidePanelSpecification&
        specification) {
  if (specification.has_top_icon()) {
    SetTopIcon(specification.top_icon());
  }
  if (specification.has_progress_step()) {
    SetProgressBarStep(specification.progress_step());
  }
  if (specification.has_description()) {
    SetDescription(base::UTF8ToUTF16(specification.description()));
  }
  if (specification.has_title()) {
    SetTitle(base::UTF8ToUTF16(specification.title()));
  }
  EndAction(true);
}

void ApcExternalActionDelegate::OnBasePromptDomUpdateReceived(
    const autofill_assistant::external::ElementConditionsUpdate& update) {
  // To ensure predictable behavior, we always choose the condition with the
  // smallest index if there are multiple fulfilled conditions.
  size_t minimum_satisfied_index = base_prompt_return_values_.size();

  for (const auto& condition : update.results()) {
    if (condition.satisfied()) {
      // Ids must be within the range of the return values vector.
      if (condition.id() < 0 || static_cast<size_t>(condition.id()) >=
                                    base_prompt_return_values_.size()) {
        DLOG(ERROR) << "dom condition id is out of bounds";
        EndAction(false);
        return;
      }
      minimum_satisfied_index = std::min(minimum_satisfied_index,
                                         static_cast<size_t>(condition.id()));
    }
  }

  if (minimum_satisfied_index < base_prompt_return_values_.size()) {
    OnBasePromptChoiceSelected(minimum_satisfied_index);
  }
}

base::WeakPtr<PasswordChangeRunController>
ApcExternalActionDelegate::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

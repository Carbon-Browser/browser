// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill_assistant/password_change/apc_client_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/autofill_assistant/common_dependencies_chrome.h"
#include "chrome/browser/autofill_assistant/password_change/apc_external_action_delegate.h"
#include "chrome/browser/autofill_assistant/password_change/apc_onboarding_coordinator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill_assistant/password_change/apc_scrim_manager.h"
#include "chrome/browser/ui/autofill_assistant/password_change/assistant_display_delegate.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/channel_info.h"
#include "components/autofill_assistant/browser/public/autofill_assistant_factory.h"
#include "components/autofill_assistant/browser/public/headless_script_controller.h"
#include "components/autofill_assistant/browser/public/public_script_parameters.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace {
constexpr char kIntent[] = "PASSWORD_CHANGE";

constexpr int kInChromeCaller = 7;
constexpr int kSourcePasswordChangeLeakWarning = 10;
constexpr int kSourcePasswordChangeSettings = 11;
}  // namespace

ApcClientImpl::ApcClientImpl(content::WebContents* web_contents)
    : content::WebContentsUserData<ApcClientImpl>(*web_contents) {}

ApcClientImpl::~ApcClientImpl() = default;

void ApcClientImpl::Start(
    const GURL& url,
    const std::string& username,
    bool skip_login,
    ResultCallback callback,
    absl::optional<DebugRunInformation> debug_run_information) {
  // If the unified side panel is not enabled, trying to register an entry in it
  // later on will crash.
  if (!base::FeatureList::IsEnabled(features::kUnifiedSidePanel)) {
    std::move(callback).Run(false);
    return;
  }

  // Ensure that only one run is ongoing.
  if (is_running_) {
    std::move(callback).Run(false);
    return;
  }
  is_running_ = true;
  result_callback_ = std::move(callback);

  GetRuntimeManager()->SetUIState(autofill_assistant::UIState::kShown);

  url_ = url;
  username_ = username;
  skip_login_ = skip_login;
  debug_run_information_ = debug_run_information;

  // The coordinator takes care of checking whether a user has previously given
  // consent and, if not, prompts the user to give consent now.
  onboarding_coordinator_ = CreateOnboardingCoordinator();
  onboarding_coordinator_->PerformOnboarding(base::BindOnce(
      &ApcClientImpl::OnOnboardingComplete, base::Unretained(this)));
}

void ApcClientImpl::Stop(bool success) {
  GetRuntimeManager()->SetUIState(autofill_assistant::UIState::kNotShown);
  onboarding_coordinator_.reset();
  external_script_controller_.reset();
  scrim_manager_.reset();
  is_running_ = false;
  if (result_callback_)
    std::move(result_callback_).Run(success);
}

bool ApcClientImpl::IsRunning() const {
  return is_running_;
}

void ApcClientImpl::PromptForConsent() {
  if (is_running_)
    return;
  is_running_ = true;

  onboarding_coordinator_ = CreateOnboardingCoordinator();
  onboarding_coordinator_->PerformOnboarding(
      base::BindOnce(&ApcClientImpl::Stop, base::Unretained(this)));
}

void ApcClientImpl::RevokeConsent(const std::vector<int>& description_grd_ids) {
  if (is_running_)
    Stop(false);

  onboarding_coordinator_ = CreateOnboardingCoordinator();
  onboarding_coordinator_->RevokeConsent(description_grd_ids);
  onboarding_coordinator_.reset();
}

// `success` indicates whether onboarding was successful, i.e. whether consent
// has been given.
void ApcClientImpl::OnOnboardingComplete(bool success) {
  if (!success) {
    Stop(/*success=*/false);
    return;
  }

  side_panel_coordinator_ = CreateSidePanel();
  side_panel_coordinator_->AddObserver(this);

  base::flat_map<std::string, std::string> params_map = {
      {autofill_assistant::public_script_parameters::
           kPasswordChangeUsernameParameterName,
       username_},
      {autofill_assistant::public_script_parameters::kIntentParameterName,
       kIntent},
      {autofill_assistant::public_script_parameters::
           kStartImmediatelyParameterName,
       "true"},
      {autofill_assistant::public_script_parameters::
           kOriginalDeeplinkParameterName,
       url_.spec()},
      {autofill_assistant::public_script_parameters::
           kPasswordChangeSkipLoginParameterName,
       skip_login_ ? "true" : "false"},
      {autofill_assistant::public_script_parameters::kEnabledParameterName,
       "true"},
      {autofill_assistant::public_script_parameters::kCallerParameterName,
       base::NumberToString(kInChromeCaller)},
      {autofill_assistant::public_script_parameters::kSourceParameterName,
       skip_login_ ? base::NumberToString(kSourcePasswordChangeLeakWarning)
                   : base::NumberToString(kSourcePasswordChangeSettings)}};

  if (debug_run_information_.has_value()) {
    params_map[autofill_assistant::public_script_parameters::
                   kDebugBundleIdParameterName] =
        debug_run_information_.value().bundle_id;
    params_map[autofill_assistant::public_script_parameters::
                   kDebugSocketIdParameterName] =
        debug_run_information_.value().socket_id;
  }

  scrim_manager_ = CreateApcScrimManager();
  external_script_controller_ = CreateHeadlessScriptController();
  scrim_manager_->Show();
  external_script_controller_->StartScript(
      params_map,
      base::BindOnce(&ApcClientImpl::OnRunComplete, base::Unretained(this)));
}

void ApcClientImpl::OnRunComplete(
    autofill_assistant::HeadlessScriptController::ScriptResult result) {
  // TODO(crbug.com/1324089): Handle failed result.
  Stop(result.success);
}

void ApcClientImpl::OnHidden() {
  Stop(/*success=*/false);

  // The two resets below are not included in `Stop()`, since we may wish to
  // render content in the side panel even for a stopped flow.
  apc_external_action_delegate_.reset();
  side_panel_coordinator_.reset();
}

std::unique_ptr<ApcOnboardingCoordinator>
ApcClientImpl::CreateOnboardingCoordinator() {
  return ApcOnboardingCoordinator::Create(&GetWebContents());
}

std::unique_ptr<AssistantSidePanelCoordinator>
ApcClientImpl::CreateSidePanel() {
  return AssistantSidePanelCoordinator::Create(&GetWebContents());
}

std::unique_ptr<autofill_assistant::HeadlessScriptController>
ApcClientImpl::CreateHeadlessScriptController() {
  DCHECK(scrim_manager_);
  apc_external_action_delegate_ = std::make_unique<ApcExternalActionDelegate>(
      side_panel_coordinator_.get(), scrim_manager_.get());
  apc_external_action_delegate_->SetupDisplay();
  apc_external_action_delegate_->ShowStartingScreen(url_);

  std::unique_ptr<autofill_assistant::AutofillAssistant> autofill_assistant =
      autofill_assistant::AutofillAssistantFactory::CreateForBrowserContext(
          GetWebContents().GetBrowserContext(),
          std::make_unique<autofill_assistant::CommonDependenciesChrome>());
  return autofill_assistant->CreateHeadlessScriptController(
      &GetWebContents(), apc_external_action_delegate_.get());
}

autofill_assistant::RuntimeManager* ApcClientImpl::GetRuntimeManager() {
  return autofill_assistant::RuntimeManager::GetOrCreateForWebContents(
      &GetWebContents());
}

std::unique_ptr<ApcScrimManager> ApcClientImpl::CreateApcScrimManager() {
  return ApcScrimManager::Create(&GetWebContents());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ApcClientImpl);

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/facilitated_payments/core/browser/ewallet_manager.h"

#include <algorithm>

#include "base/check_deref.h"
#include "base/containers/span.h"
#include "base/functional/callback_helpers.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_model/ewallet.h"
#include "components/autofill/core/browser/payments/payments_autofill_client.h"
#include "components/autofill/core/browser/payments/payments_util.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_api_client.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_client.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_initiate_payment_request_details.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_initiate_payment_response_details.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_network_interface.h"
#include "components/facilitated_payments/core/features/features.h"
#include "components/facilitated_payments/core/metrics/facilitated_payments_metrics.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_utils.h"
#include "components/facilitated_payments/core/validation/payment_link_validator.h"
#include "components/optimization_guide/core/optimization_guide_decider.h"
#include "url/gurl.h"

namespace payments::facilitated {
namespace {

static constexpr FacilitatedPaymentsType kPaymentsType =
    FacilitatedPaymentsType::kEwallet;

}  // namespace

EwalletManager::EwalletManager(
    FacilitatedPaymentsClient* client,
    FacilitatedPaymentsApiClientCreator api_client_creator,
    optimization_guide::OptimizationGuideDecider* optimization_guide_decider)
    : client_(CHECK_DEREF(client)),
      api_client_creator_(std::move(api_client_creator)),
      optimization_guide_decider_(CHECK_DEREF(optimization_guide_decider)) {
  optimization_guide_decider_->RegisterOptimizationTypes(
      {optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST});
}

EwalletManager::~EwalletManager() {
  DismissPrompt();
}

void EwalletManager::TriggerEwalletPushPayment(const GURL& payment_link_url,
                                               const GURL& page_url) {
  payment_flow_triggered_timestamp_ = base::TimeTicks::Now();

  if (optimization_guide_decider_->CanApplyOptimization(
          page_url, optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST,
          /*optimization_metadata=*/nullptr) !=
      optimization_guide::OptimizationGuideDecision::kTrue) {
    // The merchant is not part of the allowlist, ignore the eWallet push
    // payment.
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kNotInAllowlist);
    return;
  }

  scheme_ = PaymentLinkValidator().GetScheme(payment_link_url);
  if (scheme_ == PaymentLinkValidator::Scheme::kInvalid) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kLinkIsInvalid);
    return;
  }

  // Ewallet payment flow can't be completed in the landscape mode as the
  // Payments server doesn't support it yet.
  if (client_->IsInLandscapeMode()) {
    LogEwalletFlowExitedReason(
        EwalletFlowExitedReason::kLandscapeScreenOrientation, scheme_);
    return;
  }

  autofill::PaymentsDataManager* payments_data_manager =
      client_->GetPaymentsDataManager();
  if (!payments_data_manager) {
    // Payments data manager can be null only in tests.
    return;
  }

  if (!payments_data_manager->IsFacilitatedPaymentsEwalletUserPrefEnabled()) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kUserOptedOut, scheme_);
    return;
  }

  base::span<const autofill::Ewallet> ewallet_accounts =
      payments_data_manager->GetEwalletAccounts();
  supported_ewallets_.reserve(ewallet_accounts.size());
  std::ranges::copy_if(
      ewallet_accounts, std::back_inserter(supported_ewallets_),
      [&payment_link_url](const autofill::Ewallet& ewallet) {
        return ewallet.SupportsPaymentLink(payment_link_url.spec());
      });

  if (supported_ewallets_.size() == 0) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kNoSupportedEwallet,
                               scheme_);
    return;
  }

  if (!GetApiClient()) {
    return;
  }

  initiate_payment_request_details_ =
      std::make_unique<FacilitatedPaymentsInitiatePaymentRequestDetails>();
  initiate_payment_request_details_->merchant_payment_page_hostname_ =
      page_url.host();
  initiate_payment_request_details_->payment_link_ = payment_link_url.spec();

  client_->SetUiEventListener(base::BindRepeating(
      &EwalletManager::OnUiEvent, weak_ptr_factory_.GetWeakPtr()));

  GetApiClient()->IsAvailable(
      base::BindOnce(&EwalletManager::OnApiAvailabilityReceived,
                     weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()));
}

void EwalletManager::Reset() {
  supported_ewallets_.clear();
  initiate_payment_request_details_.reset();
  ui_state_ = UiState::kHidden;
  weak_ptr_factory_.InvalidateWeakPtrs();
}

FacilitatedPaymentsApiClient* EwalletManager::GetApiClient() {
  if (!api_client_) {
    if (api_client_creator_) {
      api_client_ = std::move(api_client_creator_).Run();
    }
  }

  return api_client_.get();
}

void EwalletManager::OnApiAvailabilityReceived(base::TimeTicks start_time,
                                               bool is_api_available) {
  LogApiAvailabilityCheckResultAndLatency(kPaymentsType, is_api_available,
                                          (base::TimeTicks::Now() - start_time),
                                          scheme_);
  if (!is_api_available) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kApiClientNotAvailable,
                               scheme_);
    return;
  }

  initiate_payment_request_details_->billing_customer_number_ =
      autofill::payments::GetBillingCustomerId(
          *client_->GetPaymentsDataManager());

  ShowEwalletPaymentPrompt(
      supported_ewallets_,
      base::BindOnce(&EwalletManager::OnEwalletPaymentPromptResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EwalletManager::OnEwalletPaymentPromptResult(
    bool is_prompt_accepted,
    int64_t selected_instrument_id) {
  if (!is_prompt_accepted) {
    return;
  }
  ShowProgressScreen();

  initiate_payment_request_details_->instrument_id_ = selected_instrument_id;

  client_->LoadRiskData(base::BindOnce(&EwalletManager::OnRiskDataLoaded,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       base::TimeTicks::Now()));
}

void EwalletManager::OnRiskDataLoaded(base::TimeTicks start_time,
                                      const std::string& risk_data) {
  LogLoadRiskDataResultAndLatency(kPaymentsType,
                                  /*was_successful=*/!risk_data.empty(),
                                  base::TimeTicks::Now() - start_time, scheme_);
  if (risk_data.empty()) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kRiskDataEmpty,
                               scheme_);
    ShowErrorScreen();
    return;
  }

  initiate_payment_request_details_->risk_data_ = risk_data;

  GetApiClient()->GetClientToken(
      base::BindOnce(&EwalletManager::OnGetClientToken,
                     weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()));
}

void EwalletManager::OnGetClientToken(base::TimeTicks start_time,
                                      std::vector<uint8_t> client_token) {
  LogGetClientTokenResultAndLatency(kPaymentsType, !client_token.empty(),
                                    (base::TimeTicks::Now() - start_time),
                                    scheme_);
  if (client_token.empty()) {
    LogEwalletFlowExitedReason(
        EwalletFlowExitedReason::kClientTokenNotAvailable, scheme_);
    ShowErrorScreen();
    return;
  }
  initiate_payment_request_details_->client_token_ = std::move(client_token);

  SendInitiatePaymentRequest();
}

void EwalletManager::SendInitiatePaymentRequest() {
  FacilitatedPaymentsNetworkInterface* payments_network_interface =
      client_->GetFacilitatedPaymentsNetworkInterface();

  if (!payments_network_interface) {
    ShowErrorScreen();
    return;
  }

  LogInitiatePaymentAttempt(kPaymentsType, scheme_);
  payments_network_interface->InitiatePayment(
      std::move(initiate_payment_request_details_),
      base::BindOnce(&EwalletManager::OnInitiatePaymentResponseReceived,
                     weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()),
      client_->GetPaymentsDataManager()->app_locale());
}

void EwalletManager::OnInitiatePaymentResponseReceived(
    base::TimeTicks start_time,
    autofill::payments::PaymentsAutofillClient::PaymentsRpcResult result,
    std::unique_ptr<FacilitatedPaymentsInitiatePaymentResponseDetails>
        response_details) {
  bool is_successful =
      result ==
      autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::kSuccess;
  LogInitiatePaymentResultAndLatency(kPaymentsType, /*result=*/is_successful,
                                     base::TimeTicks::Now() - start_time,
                                     scheme_);
  if (!is_successful) {
    ShowErrorScreen();
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kInitiatePaymentFailed,
                               scheme_);
    return;
  }
  if (!response_details || response_details->action_token_.empty()) {
    LogEwalletFlowExitedReason(
        EwalletFlowExitedReason::kActionTokenNotAvailable, scheme_);
    ShowErrorScreen();
    return;
  }
  std::optional<CoreAccountInfo> account_info = client_->GetCoreAccountInfo();
  // If the user logged out after selecting the payment method, the
  // `account_info` would be empty, and the  the payment flow should be
  // abandoned.
  if (!account_info.has_value() || account_info.value().IsEmpty()) {
    LogEwalletFlowExitedReason(EwalletFlowExitedReason::kUserLoggedOut,
                               scheme_);
    ShowErrorScreen();
    return;
  }

  LogInitiatePurchaseActionAttempt(kPaymentsType, scheme_);
  GetApiClient()->InvokePurchaseAction(
      account_info.value(), response_details->action_token_,
      base::BindOnce(&EwalletManager::OnTransactionResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void EwalletManager::OnTransactionResult(PurchaseActionResult result) {
  switch (result) {
    case PurchaseActionResult::kCouldNotInvoke:
      ShowErrorScreen();
      break;
    case PurchaseActionResult::kResultOk:
      [[fallthrough]];  // Intentional fallthrough.
    case PurchaseActionResult::kResultCanceled:
      DismissPrompt();
      break;
  }
}

void EwalletManager::OnUiEvent(UiEvent ui_event_type) {
  switch (ui_event_type) {
    case UiEvent::kNewScreenShown: {
      CHECK_NE(ui_state_, UiState::kHidden);
      LogUiScreenShown(kPaymentsType, ui_state_, scheme_);
      if (ui_state_ == UiState::kFopSelector) {
        LogFopSelectorShownLatency(
            kPaymentsType,
            base::TimeTicks::Now() - payment_flow_triggered_timestamp_,
            scheme_);
      }
      break;
    }
    case UiEvent::kScreenClosedNotByUser: {
      if (ui_state_ == UiState::kFopSelector) {
        LogEwalletFlowExitedReason(
            EwalletFlowExitedReason::kFopSelectorClosedNotByUser, scheme_);
      }
      ui_state_ = UiState::kHidden;
      break;
    }
    case UiEvent::kScreenClosedByUser: {
      if (ui_state_ == UiState::kFopSelector) {
        LogEwalletFlowExitedReason(
            EwalletFlowExitedReason::kFopSelectorClosedByUser, scheme_);
      }
      ui_state_ = UiState::kHidden;
      break;
    }
  }
}

void EwalletManager::DismissPrompt() {
  ui_state_ = UiState::kHidden;
  client_->DismissPrompt();
}

void EwalletManager::ShowEwalletPaymentPrompt(
    base::span<const autofill::Ewallet> ewallet_suggestions,
    base::OnceCallback<void(bool, int64_t)> on_user_decision_callback) {
  ui_state_ = UiState::kFopSelector;
  client_->ShowEwalletPaymentPrompt(std::move(ewallet_suggestions),
                                    std::move(on_user_decision_callback));
}

void EwalletManager::ShowProgressScreen() {
  ui_state_ = UiState::kProgressScreen;
  client_->ShowProgressScreen();
}

void EwalletManager::ShowErrorScreen() {
  ui_state_ = UiState::kErrorScreen;
  client_->ShowErrorScreen();
}

}  // namespace payments::facilitated

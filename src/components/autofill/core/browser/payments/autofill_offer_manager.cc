// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/autofill_offer_manager.h"

#include "base/check_deref.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/ranges/ranges.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/autofill/core/browser/data_model/autofill_offer_data.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/foundations/autofill_client.h"
#include "components/autofill/core/browser/suggestions/suggestion_type.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {
AutofillOfferManager::AutofillOfferManager(
    PaymentsDataManager* payments_data_manager)
    : payments_data_manager_(CHECK_DEREF(payments_data_manager)) {
  payments_data_manager_observation.Observe(payments_data_manager);
  UpdateEligibleMerchantDomains();
}

AutofillOfferManager::~AutofillOfferManager() = default;

void AutofillOfferManager::OnPaymentsDataChanged() {
  UpdateEligibleMerchantDomains();
}

void AutofillOfferManager::OnDidNavigateFrame(AutofillClient& client) {
  notification_handler_.UpdateOfferNotificationVisibility(client);
}

AutofillOfferManager::CardLinkedOffersMap
AutofillOfferManager::GetCardLinkedOffersMap(
    const GURL& last_committed_primary_main_frame_url) const {
  GURL last_committed_primary_main_frame_origin =
      last_committed_primary_main_frame_url.DeprecatedGetOriginAsURL();

  if (!base::Contains(eligible_merchant_domains_,
                      last_committed_primary_main_frame_origin)) {
    return {};
  }

  const std::vector<const CreditCard*> cards =
      payments_data_manager_->GetCreditCards();
  AutofillOfferManager::CardLinkedOffersMap card_linked_offers_map;

  for (const AutofillOfferData* offer :
       payments_data_manager_->GetAutofillOffers()) {
    // Ensure the offer is valid.
    if (!offer->IsActiveAndEligibleForOrigin(
            last_committed_primary_main_frame_origin)) {
      continue;
    }

    // Ensure the offer is a card-linked offer.
    if (!offer->IsCardLinkedOffer()) {
      continue;
    }

    for (const CreditCard* card : cards) {
      // If card has an offer, add the card's guid id to the map. There is
      // currently a one-to-one mapping between cards and offer data.
      if (base::Contains(offer->GetEligibleInstrumentIds(),
                         card->instrument_id())) {
        card_linked_offers_map[card->guid()] = offer;
      }
    }
  }

  return card_linked_offers_map;
}

bool AutofillOfferManager::IsUrlEligible(
    const GURL& last_committed_primary_main_frame_url) {
  return base::Contains(
      eligible_merchant_domains_,
      last_committed_primary_main_frame_url.DeprecatedGetOriginAsURL());
}

const AutofillOfferData* AutofillOfferManager::GetOfferForUrl(
    const GURL& last_committed_primary_main_frame_url) const {
  for (const AutofillOfferData* offer :
       payments_data_manager_->GetAutofillOffers()) {
    if (offer->IsActiveAndEligibleForOrigin(
            last_committed_primary_main_frame_url.DeprecatedGetOriginAsURL())) {
      return offer;
    }
  }

  return nullptr;
}

void AutofillOfferManager::UpdateEligibleMerchantDomains() {
  eligible_merchant_domains_.clear();
  for (const AutofillOfferData* offer :
       payments_data_manager_->GetAutofillOffers()) {
    eligible_merchant_domains_.insert(offer->GetMerchantOrigins().begin(),
                                      offer->GetMerchantOrigins().end());
  }
}

}  // namespace autofill

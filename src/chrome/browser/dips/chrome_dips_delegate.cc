// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/dips/chrome_dips_delegate.h"

#include <memory>

#include "base/check.h"
#include "base/types/pass_key.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/dips/dips_browser_signin_detector.h"
#include "chrome/browser/dips/stateful_bounce_counter.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/dips_delegate.h"

static_assert(DIPSService::kDefaultRemoveMask ==
                  (chrome_browsing_data_remover::FILTERABLE_DATA_TYPES &
                   ((content::BrowsingDataRemover::DATA_TYPE_CONTENT_END << 1) -
                    1)),
              "kDefaultRemoveMask must contain all the entries of "
              "FILTERABLE_DATA_TYPES that are known in //content");

ChromeDipsDelegate::ChromeDipsDelegate(
    base::PassKey<ChromeContentBrowserClient>) {}

void ChromeDipsDelegate::OnDipsServiceCreated(
    content::BrowserContext* browser_context,
    DIPSService* dips_service) {
  // Create DIPSBrowserSigninDetector.
  CHECK(DIPSBrowserSigninDetector::Get(browser_context));
  dips::StatefulBounceCounter::CreateFor(dips_service);
}

uint64_t ChromeDipsDelegate::GetRemoveMask() {
  return chrome_browsing_data_remover::FILTERABLE_DATA_TYPES;
}

bool ChromeDipsDelegate::ShouldDeleteInteractionRecords(uint64_t remove_mask) {
  return remove_mask & chrome_browsing_data_remover::DATA_TYPE_HISTORY;
}

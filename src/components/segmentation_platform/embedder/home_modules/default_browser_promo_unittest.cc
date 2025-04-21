// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/segmentation_platform/embedder/home_modules/default_browser_promo.h"

#include "components/prefs/testing_pref_service.h"
#include "components/segmentation_platform/embedder/home_modules/card_selection_signals.h"
#include "components/segmentation_platform/embedder/home_modules/constants.h"
#include "components/segmentation_platform/embedder/home_modules/home_modules_card_registry.h"
#include "components/segmentation_platform/embedder/home_modules/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace segmentation_platform::home_modules {

// Tests DefaultBrowserPromo's functionality.
class DefaultBrowserPromoTest : public testing::Test {
 public:
  DefaultBrowserPromoTest() = default;
  ~DefaultBrowserPromoTest() override = default;

  void SetUp() override {
    HomeModulesCardRegistry::RegisterProfilePrefs(pref_service_.registry());
  }

  void TearDown() override { Test::TearDown(); }

 protected:
  TestingPrefServiceSimple pref_service_;
};

// Verifies that the `GetInputs(…)` method returns the expected inputs.
TEST_F(DefaultBrowserPromoTest, GetInputsReturnsExpectedInputs) {
  auto card = std::make_unique<DefaultBrowserPromo>(&pref_service_);
  std::map<SignalKey, FeatureQuery> inputs = card->GetInputs();
  EXPECT_EQ(inputs.size(), 2u);
  // Verify that the inputs map contains the expected keys.
  EXPECT_NE(
      inputs.find(
          segmentation_platform::kShouldShowNonRoleManagerDefaultBrowserPromo),
      inputs.end());
  EXPECT_NE(
      inputs.find(
          segmentation_platform::kHasDefaultBrowserPromoShownInOtherSurface),
      inputs.end());
}

// Validates that ComputeCardResult() returns kTop when default browser promo
// card is enabled.
TEST_F(DefaultBrowserPromoTest, TestComputeCardResultWithCardEnabled) {
  pref_service_.SetUserPref(kDefaultBrowserPromoInteractedPref,
                            std::make_unique<base::Value>(false));
  auto card = std::make_unique<DefaultBrowserPromo>(&pref_service_);
  AllCardSignals all_signals = CreateAllCardSignals(
      card.get(), {/* kHasDefaultBrowserPromoShownInOtherSurface */ 0,
                   /* kShouldShowNonRoleManagerDefaultBrowserPromo */ 1});
  CardSelectionSignals card_signal(&all_signals, kDefaultBrowserPromo);
  CardSelectionInfo::ShowResult result = card->ComputeCardResult(card_signal);
  EXPECT_EQ(EphemeralHomeModuleRank::kTop, result.position);
}

// Validates that when the default browser promo card is disabled because the
// non-role manager default browser promo should not be displayed, the
// ComputeCardResult() function returns kNotShown.
TEST_F(DefaultBrowserPromoTest,
       TestComputeCardResultWithCardDisabledForNotShowNonRoleManagerPromo) {
  pref_service_.SetUserPref(kDefaultBrowserPromoInteractedPref,
                            std::make_unique<base::Value>(false));
  auto card = std::make_unique<DefaultBrowserPromo>(&pref_service_);
  AllCardSignals all_signals = CreateAllCardSignals(
      card.get(), {/* kHasDefaultBrowserPromoShownInOtherSurface */ 0,
                   /* kShouldShowNonRoleManagerDefaultBrowserPromo */ 0});
  CardSelectionSignals card_signal(&all_signals, kDefaultBrowserPromo);
  CardSelectionInfo::ShowResult result = card->ComputeCardResult(card_signal);
  EXPECT_EQ(EphemeralHomeModuleRank::kNotShown, result.position);
}

// Validates that the ComputeCardResult() function returns kNotShown when the
// default browser promo card is disabled because the user already saw the promo
// in other surfaces, such as through settings, messages, or alternative NTPs.
TEST_F(DefaultBrowserPromoTest,
       TestComputeCardResultWithCardDisabledForShownInOtherSurface) {
  pref_service_.SetUserPref(kDefaultBrowserPromoInteractedPref,
                            std::make_unique<base::Value>(false));
  auto card = std::make_unique<DefaultBrowserPromo>(&pref_service_);
  AllCardSignals all_signals = CreateAllCardSignals(
      card.get(), {/* kHasDefaultBrowserPromoShownInOtherSurface */ 1,
                   /* kShouldShowNonRoleManagerDefaultBrowserPromo */ 1});
  CardSelectionSignals card_signal(&all_signals, kDefaultBrowserPromo);
  CardSelectionInfo::ShowResult result = card->ComputeCardResult(card_signal);
  EXPECT_EQ(EphemeralHomeModuleRank::kNotShown, result.position);
}

// Validates that the ComputeCardResult() function returns kNotShown when the
// default browser promo card is disabled because the user already interacted
// with the card.
TEST_F(DefaultBrowserPromoTest,
       TestComputeCardResultWithCardDisabledForUserInteraction) {
  pref_service_.SetUserPref(kDefaultBrowserPromoInteractedPref,
                            std::make_unique<base::Value>(true));
  auto card = std::make_unique<DefaultBrowserPromo>(&pref_service_);
  AllCardSignals all_signals = CreateAllCardSignals(
      card.get(), {/* kHasDefaultBrowserPromoShownInOtherSurface */ 0,
                   /* kShouldShowNonRoleManagerDefaultBrowserPromo */ 1});
  CardSelectionSignals card_signal(&all_signals, kDefaultBrowserPromo);
  CardSelectionInfo::ShowResult result = card->ComputeCardResult(card_signal);
  EXPECT_EQ(EphemeralHomeModuleRank::kNotShown, result.position);
}

}  // namespace segmentation_platform::home_modules

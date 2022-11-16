// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/lens/lens_side_panel_coordinator.h"

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/test/metrics/user_action_tester.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/test_with_browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "components/lens/lens_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kNewLensQueryAction[] = "LensUnifiedSidePanel.LensQuery_New";
constexpr char kLensQueryAction[] = "LensUnifiedSidePanel.LensQuery";
constexpr char kLensEntryHiddenAction[] =
    "LensUnifiedSidePanel.LensEntryHidden";
constexpr char kLensEntryShownAction[] = "LensUnifiedSidePanel.LensEntryShown";
constexpr char kLensQueryFollowupAction[] =
    "LensUnifiedSidePanel.LensQuery_Followup";
constexpr char kLensQuerySidePanelClosedAction[] =
    "LensUnifiedSidePanel.LensQuery_SidePanelClosed";
constexpr char kLensQuerySidePanelOpenNonLensAction[] =
    "LensUnifiedSidePanel.LensQuery_SidePanelOpenNonLens";
constexpr char kLensQuerySidePanelOpenLensAction[] =
    "LensUnifiedSidePanel.LensQuery_SidePanelOpenLens";

class LensSidePanelCoordinatorTest : public TestWithBrowserView {
 public:
  void SetUp() override {
    base::test::ScopedFeatureList features;
    features.InitWithFeaturesAndParameters(
        {{lens::features::kLensStandalone,
          {{lens::features::kEnableSidePanelForLens.name, "true"},
           {lens::features::kEnableLensSidePanelFooter.name, "true"}}},
         {features::kUnifiedSidePanel, {{}}}},
        {});
    TestWithBrowserView::SetUp();

    GetSidePanelCoordinator()->SetNoDelaysForTesting();
    auto* browser = browser_view()->browser();
    auto* global_registry =
        GetSidePanelCoordinator()->GetGlobalSidePanelRegistry();
    SidePanelUtil::PopulateGlobalEntries(browser, global_registry);

    EXPECT_EQ(global_registry->entries().size(), 2u);

    // Create the lens coordinator in Browser.
    lens_side_panel_coordinator_ =
        LensSidePanelCoordinator::GetOrCreateForBrowser(browser);
    // Create an active web contents.
    AddTab(browser, GURL("about:blank"));
  }

  SidePanelCoordinator* GetSidePanelCoordinator() {
    return browser_view()->side_panel_coordinator();
  }

  SidePanel* GetRightAlignedSidePanel() {
    return browser_view()->right_aligned_side_panel();
  }

 protected:
  raw_ptr<LensSidePanelCoordinator> lens_side_panel_coordinator_;
};

TEST_F(LensSidePanelCoordinatorTest,
       OpenWithUrlShowsUnifiedSidePanelWithLensSelected) {
  base::UserActionTester user_action_tester;
  EXPECT_EQ(0, user_action_tester.GetActionCount(kNewLensQueryAction));

  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://foo.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));

  EXPECT_TRUE(GetRightAlignedSidePanel()->GetVisible());
  EXPECT_EQ(
      GetSidePanelCoordinator()->GetCurrentSidePanelEntryForTesting()->id(),
      SidePanelEntry::Id::kLens);
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensQueryAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kNewLensQueryAction));
  EXPECT_EQ(1,
            user_action_tester.GetActionCount(kLensQuerySidePanelClosedAction));
}

TEST_F(LensSidePanelCoordinatorTest, OpenWithUrlWhenSidePanelOpenShowsLens) {
  base::UserActionTester user_action_tester;
  GetSidePanelCoordinator()->Show(SidePanelEntry::Id::kBookmarks);
  EXPECT_EQ(0, user_action_tester.GetActionCount(kNewLensQueryAction));

  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://foo.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));

  EXPECT_TRUE(GetRightAlignedSidePanel()->GetVisible());
  EXPECT_EQ(
      GetSidePanelCoordinator()->GetCurrentSidePanelEntryForTesting()->id(),
      SidePanelEntry::Id::kLens);
  EXPECT_EQ(1, user_action_tester.GetActionCount(kNewLensQueryAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(
                   kLensQuerySidePanelOpenNonLensAction));
}

TEST_F(LensSidePanelCoordinatorTest,
       CallingRegisterTwiceOpensNewUrlAndLogsAction) {
  base::UserActionTester user_action_tester;

  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://foo.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));
  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://bar.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));

  EXPECT_TRUE(GetRightAlignedSidePanel()->GetVisible());
  EXPECT_EQ(
      GetSidePanelCoordinator()->GetCurrentSidePanelEntryForTesting()->id(),
      SidePanelEntry::Id::kLens);
  EXPECT_EQ(2, user_action_tester.GetActionCount(kLensQueryAction));
  EXPECT_EQ(1,
            user_action_tester.GetActionCount(kLensQuerySidePanelClosedAction));
  EXPECT_EQ(
      1, user_action_tester.GetActionCount(kLensQuerySidePanelOpenLensAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensQueryFollowupAction));
}

TEST_F(LensSidePanelCoordinatorTest, SwitchToDifferentItemTriggersHideEvent) {
  base::UserActionTester user_action_tester;
  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://foo.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));

  GetSidePanelCoordinator()->Show(SidePanelEntry::Id::kBookmarks);

  EXPECT_TRUE(GetRightAlignedSidePanel()->GetVisible());
  EXPECT_EQ(
      GetSidePanelCoordinator()->GetCurrentSidePanelEntryForTesting()->id(),
      SidePanelEntry::Id::kBookmarks);
  EXPECT_EQ(1,
            user_action_tester.GetActionCount(kLensQuerySidePanelClosedAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensEntryHiddenAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensEntryShownAction));
}

TEST_F(LensSidePanelCoordinatorTest, SwitchBackToLensTriggersShowEvent) {
  base::UserActionTester user_action_tester;
  lens_side_panel_coordinator_->RegisterEntryAndShow(
      content::OpenURLParams(GURL("http://foo.com"), content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));

  GetSidePanelCoordinator()->Show(SidePanelEntry::Id::kBookmarks);
  GetSidePanelCoordinator()->Show(SidePanelEntry::Id::kLens);

  EXPECT_TRUE(GetRightAlignedSidePanel()->GetVisible());
  EXPECT_EQ(
      GetSidePanelCoordinator()->GetCurrentSidePanelEntryForTesting()->id(),
      SidePanelEntry::Id::kLens);
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensQueryAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kNewLensQueryAction));
  EXPECT_EQ(1,
            user_action_tester.GetActionCount(kLensQuerySidePanelClosedAction));
  EXPECT_EQ(1, user_action_tester.GetActionCount(kLensEntryHiddenAction));
  EXPECT_EQ(2, user_action_tester.GetActionCount(kLensEntryShownAction));
}

}  // namespace

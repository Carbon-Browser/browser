// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_search/unified_side_search_controller.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/side_search/side_search_utils.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_search/side_search_browsertest.h"
#include "content/public/common/result_codes.h"
#include "content/public/test/browser_test.h"
#include "ui/views/interaction/element_tracker_views.h"

// Fixture for testing side panel v2 only. Only instantiate tests for DSE
// configuration.
class SideSearchV2Test : public SideSearchBrowserTest {
 public:
  void SetUp() override {
    scoped_feature_list_.InitWithFeatures(
        {features::kSideSearch, features::kSideSearchDSESupport,
         features::kUnifiedSidePanel},
        {});
    SideSearchBrowserTest::SetUp();
  }

  SidePanel* GetSidePanelFor(Browser* browser) override {
    return BrowserViewFor(browser)->right_aligned_side_panel();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       SidePanelButtonShowsCorrectlySingleTab) {
  // If no previous matched search page has been navigated to the button should
  // not be visible.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());

  // The side panel button should never be visible on a matched search page.
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());

  // The side panel button should be visible if on a non-matched page and the
  // current tab has previously encountered a matched search page.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       SidePanelButtonShowsCorrectlyMultipleTabs) {
  // The side panel button should never be visible on non-matching pages.
  AppendTab(browser(), GetNonMatchingUrl());
  ActivateTabAt(browser(), 1);
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());

  // Navigate to a matched search page and then to a non-matched search page.
  // This should show the side panel button in the toolbar.
  AppendTab(browser(), GetMatchingSearchUrl());
  ActivateTabAt(browser(), 2);
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());

  // Switch back to the matched search page, the side panel button should no
  // longer be visible.
  ActivateTabAt(browser(), 1);
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());

  // When switching back to the tab on the non-matched page with a previously
  // visited matched search page, the button should be visible.
  ActivateTabAt(browser(), 2);
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test, SidePanelTogglesCorrectlySingleTab) {
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // The side panel button should be visible if on a non-matched page and the
  // current tab has previously encountered a matched search page.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // Toggle the side panel.
  NotifyButtonClick(browser());
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());

  // Toggling the close button should close the side panel.
  NotifyCloseButtonClick(browser());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test, CloseButtonClosesSidePanel) {
  // The close button should be visible in the toggled state.
  NavigateToMatchingSearchPageAndOpenSidePanel(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());
  NotifyCloseButtonClick(browser());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test, SideSearchNotAvailableInOTR) {
  Browser* browser2 = CreateIncognitoBrowser();
  EXPECT_TRUE(browser2->profile()->IsOffTheRecord());
  NavigateActiveTab(browser2, GetMatchingSearchUrl());
  NavigateActiveTab(browser2, GetNonMatchingUrl());

  EXPECT_EQ(nullptr, GetSidePanelButtonFor(browser2));
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       SidePanelButtonIsNotShownWhenSRPIsUnavailable) {
  // Set the side panel SRP be unavailable.
  SetIsSidePanelSRPAvailableAt(browser(), 0, false);

  // If no previous matched search page has been navigated to the button should
  // not be visible.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // The side panel button should never be visible on the matched search page.
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // The side panel button should not be visible if the side panel SRP is not
  // available.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1340387): Test is flaky on Mac.
#define MAYBE_SidePanelStatePreservedWhenMovingTabsAcrossBrowserWindows \
  DISABLED_SidePanelStatePreservedWhenMovingTabsAcrossBrowserWindows
#else
#define MAYBE_SidePanelStatePreservedWhenMovingTabsAcrossBrowserWindows \
  SidePanelStatePreservedWhenMovingTabsAcrossBrowserWindows
#endif
IN_PROC_BROWSER_TEST_F(
    SideSearchV2Test,
    MAYBE_SidePanelStatePreservedWhenMovingTabsAcrossBrowserWindows) {
  NavigateToMatchingSearchPageAndOpenSidePanel(browser());

  Browser* browser2 = CreateBrowser(browser()->profile());
  NavigateToMatchingAndNonMatchingSearchPage(browser2);

  std::unique_ptr<content::WebContents> web_contents =
      browser2->tab_strip_model()->DetachWebContentsAtForInsertion(0);
  browser()->tab_strip_model()->InsertWebContentsAt(1, std::move(web_contents),
                                                    TabStripModel::ADD_ACTIVE);

  ASSERT_EQ(2, browser()->tab_strip_model()->GetTabCount());
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  ActivateTabAt(browser(), 0);
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       SidePanelTogglesCorrectlyMultipleTabs) {
  // Navigate to a matching search URL followed by a non-matching URL in two
  // independent browser tabs such that both have the side panel ready. The
  // side panel should respect the state-per-tab flag.

  // Tab 1.
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // Tab 2.
  AppendTab(browser(), GetMatchingSearchUrl());
  ActivateTabAt(browser(), 1);
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // Show the side panel on Tab 2 and switch to Tab 1. The side panel should
  // not be visible for Tab 1.
  NotifyButtonClick(browser());
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());

  ActivateTabAt(browser(), 0);
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  // Show the side panel on Tab 1 and switch to Tab 2. The side panel should be
  // still be visible for Tab 2, respecting its per-tab state.
  NotifyButtonClick(browser());
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());

  ActivateTabAt(browser(), 1);
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());

  // Close the side panel on Tab 2 and switch to Tab 1. The side panel should be
  // still be visible for Tab 1, respecting its per-tab state.
  NotifyCloseButtonClick(browser());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());

  ActivateTabAt(browser(), 0);
  TestSidePanelOpenEntrypointState(browser());
  EXPECT_TRUE(GetSidePanelFor(browser())->GetVisible());

  NotifyCloseButtonClick(browser());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_FALSE(GetSidePanelFor(browser())->GetVisible());
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       SidePanelTogglesClosedCorrectlyDuringNavigation) {
  // Navigate to a matching SRP and then a non-matched page. The side panel will
  // be available and open.
  NavigateToMatchingSearchPageAndOpenSidePanel(browser());
  auto* side_panel = GetSidePanelFor(browser());

  // Navigating to a matching SRP URL should automatically hide the side panel
  // as it should not be available.
  EXPECT_TRUE(side_panel->GetVisible());
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(side_panel->GetVisible());

  // When navigating again to a non-matching page the side panel will become
  // available again but should not automatically reopen.
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  EXPECT_FALSE(side_panel->GetVisible());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1347306): This test is flakey on macOS.
#define MAYBE_SidePanelCrashesCloseSidePanel \
  DISABLED_SidePanelCrashesCloseSidePanel
#else
#define MAYBE_SidePanelCrashesCloseSidePanel SidePanelCrashesCloseSidePanel
#endif
IN_PROC_BROWSER_TEST_F(SideSearchV2Test, MAYBE_SidePanelCrashesCloseSidePanel) {
  auto* browser_view = BrowserViewFor(browser());
  auto* coordinator = browser_view->side_panel_coordinator();
  coordinator->SetNoDelaysForTesting();

  // Open two tabs with the side panel open.
  NavigateToMatchingSearchPageAndOpenSidePanel(browser());
  AppendTab(browser(), GetNonMatchingUrl());
  ActivateTabAt(browser(), 1);
  NavigateToMatchingSearchPageAndOpenSidePanel(browser());

  auto* side_panel = GetSidePanelFor(browser());

  // Side panel should be open with the side contents present.
  EXPECT_TRUE(side_panel->GetVisible());
  EXPECT_NE(nullptr, GetSidePanelContentsFor(browser(), 1));

  // Simulate a crash in the hosted side panel contents.
  auto* rph_second_tab = GetSidePanelContentsFor(browser(), 1)
                             ->GetPrimaryMainFrame()
                             ->GetProcess();
  content::RenderProcessHostWatcher crash_observer_second_tab(
      rph_second_tab,
      content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  EXPECT_TRUE(rph_second_tab->Shutdown(content::RESULT_CODE_KILLED));
  crash_observer_second_tab.Wait();

  // Side panel should be closed and the crashed WebContents cleared.
  EXPECT_FALSE(side_panel->GetVisible());
  EXPECT_EQ(nullptr, GetSidePanelContentsFor(browser(), 1));
  EXPECT_NE(nullptr, GetSidePanelContentsFor(browser(), 0));

  // Reopen side panel.
  coordinator->Show();
  EXPECT_TRUE(side_panel->GetVisible());

  // Simulate a crash in the side panel contents of the first tab which is not
  // currently active.
  EXPECT_NE(nullptr, GetSidePanelContentsFor(browser(), 0));
  auto* rph_first_tab = GetSidePanelContentsFor(browser(), 0)
                            ->GetPrimaryMainFrame()
                            ->GetProcess();
  content::RenderProcessHostWatcher crash_observer_first_tab(
      rph_first_tab, content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  EXPECT_TRUE(rph_first_tab->Shutdown(content::RESULT_CODE_KILLED));
  crash_observer_first_tab.Wait();

  // Switch to the first tab, the side panel should still be closed.
  ActivateTabAt(browser(), 0);
  EXPECT_FALSE(side_panel->GetVisible());
  EXPECT_EQ(nullptr, GetSidePanelContentsFor(browser(), 0));

  // Reopening the side panel should restore the side panel and its contents.
  NotifyButtonClick(browser());
  EXPECT_TRUE(side_panel->GetVisible());
  EXPECT_NE(nullptr, GetSidePanelContentsFor(browser(), 0));
}

IN_PROC_BROWSER_TEST_F(SideSearchV2Test, SwitchSidePanelInSingleTab) {
  auto* browser_view = BrowserViewFor(browser());
  auto* coordinator = browser_view->side_panel_coordinator();
  coordinator->SetNoDelaysForTesting();

  // Tab 0 with side search available and open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to reading list side panel.
  coordinator->Show(SidePanelEntry::Id::kReadingList);
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(SidePanelEntry::Id::kReadingList,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch back to side search side panel.
  coordinator->Show(SidePanelEntry::Id::kSideSearch);
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1341272): Test is flaky on Mac.
#define MAYBE_SwitchTabsWithGlobalSidePanel \
  DISABLED_SwitchTabsWithGlobalSidePanel
#else
#define MAYBE_SwitchTabsWithGlobalSidePanel SwitchTabsWithGlobalSidePanel
#endif
IN_PROC_BROWSER_TEST_F(SideSearchV2Test, MAYBE_SwitchTabsWithGlobalSidePanel) {
  auto* browser_view = BrowserViewFor(browser());
  auto* coordinator = browser_view->side_panel_coordinator();
  coordinator->SetNoDelaysForTesting();

  // Tab 0 without side search available and open with reading list.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  coordinator->Show(SidePanelEntry::Id::kReadingList);
  EXPECT_EQ(SidePanelEntry::Id::kReadingList,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Tab 1 with side search available and open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Tab 2 with side search available and open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Tab 3 with side search available but not open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(SidePanelEntry::Id::kReadingList,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 0, side panel is open with reading list.
  ActivateTabAt(browser(), 0);
  EXPECT_EQ(SidePanelEntry::Id::kReadingList,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 1, side panel is open with side search.
  ActivateTabAt(browser(), 1);
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 2, side panel is open with side search.
  ActivateTabAt(browser(), 2);
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 3, side panel is open with reading list.
  ActivateTabAt(browser(), 3);
  EXPECT_EQ(SidePanelEntry::Id::kReadingList,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1340387): Test is flaky on Mac.
#define MAYBE_SwitchTabsWithoutGlobalSidePanel \
  DISABLED_SwitchTabsWithoutGlobalSidePanel
#else
#define MAYBE_SwitchTabsWithoutGlobalSidePanel SwitchTabsWithoutGlobalSidePanel
#endif
IN_PROC_BROWSER_TEST_F(SideSearchV2Test,
                       MAYBE_SwitchTabsWithoutGlobalSidePanel) {
  auto* browser_view = BrowserViewFor(browser());
  auto* coordinator = browser_view->side_panel_coordinator();

  // Tab 0 without side search available.
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_FALSE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(nullptr, coordinator->GetCurrentSidePanelEntryForTesting());

  // Tab 1 with side search available and open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Tab 2 with side search available and open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Tab 3 with side search available but not open.
  AppendTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  EXPECT_EQ(nullptr, coordinator->GetCurrentSidePanelEntryForTesting());

  // Switch to tab 0, side panel is closed.
  ActivateTabAt(browser(), 0);
  EXPECT_EQ(nullptr, coordinator->GetCurrentSidePanelEntryForTesting());

  // Switch to tab 1, side panel is open with side search.
  ActivateTabAt(browser(), 1);
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 2, side panel is open with side search.
  ActivateTabAt(browser(), 2);
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            coordinator->GetCurrentSidePanelEntryForTesting()->id());

  // Switch to tab 3, side panel is closed.
  ActivateTabAt(browser(), 3);
  EXPECT_EQ(nullptr, coordinator->GetCurrentSidePanelEntryForTesting());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1340387): Test is flaky on Mac.
#define MAYBE_CloseSidePanelShouldClearCache \
  DISABLED_CloseSidePanelShouldClearCache
#else
#define MAYBE_CloseSidePanelShouldClearCache CloseSidePanelShouldClearCache
#endif
IN_PROC_BROWSER_TEST_F(SideSearchV2Test, MAYBE_CloseSidePanelShouldClearCache) {
  auto* browser_view = BrowserViewFor(browser());
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            browser_view->side_panel_coordinator()
                ->GetCurrentSidePanelEntryForTesting()
                ->id());

  // When side panel is open,  side panel web contents is present.
  auto* tab_contents_helper = SideSearchTabContentsHelper::FromWebContents(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_NE(nullptr, tab_contents_helper->side_panel_contents_for_testing());

  browser_view->side_panel_coordinator()->Close();

  // When side panel is closed, side panel web contents is destroyed.
  EXPECT_EQ(nullptr, tab_contents_helper->side_panel_contents_for_testing());
}

#if BUILDFLAG(IS_MAC)
// TODO(crbug.com/1340387): Test is flaky on Mac.
#define MAYBE_NewForegroundTabShouldNotDestroySidePanelContents \
  DISABLED_NewForegroundTabShouldNotDestroySidePanelContents
#else
#define MAYBE_NewForegroundTabShouldNotDestroySidePanelContents \
  NewForegroundTabShouldNotDestroySidePanelContents
#endif
// Test added for crbug.com/1349687 .
IN_PROC_BROWSER_TEST_F(
    SideSearchV2Test,
    MAYBE_NewForegroundTabShouldNotDestroySidePanelContents) {
  auto* browser_view = BrowserViewFor(browser());
  NavigateActiveTab(browser(), GetMatchingSearchUrl());
  NavigateActiveTab(browser(), GetNonMatchingUrl());
  EXPECT_TRUE(GetSidePanelButtonFor(browser())->GetVisible());
  NotifyButtonClick(browser());
  EXPECT_EQ(SidePanelEntry::Id::kSideSearch,
            browser_view->side_panel_coordinator()
                ->GetCurrentSidePanelEntryForTesting()
                ->id());

  // When side panel is open,  side panel web contents is present.
  auto* tab_contents_helper = SideSearchTabContentsHelper::FromWebContents(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_NE(nullptr, tab_contents_helper->side_panel_contents_for_testing());

  // Open URL with a new foreground tab.
  tab_contents_helper->side_panel_contents_for_testing()->OpenURL(
      content::OpenURLParams(embedded_test_server()->GetURL("/foo"),
                             content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_TYPED, false));

  // When swtiched to the new tab, side panel web contents should not be
  // destroyed. Otherwise a UAF will occur.
  EXPECT_NE(nullptr, tab_contents_helper->side_panel_contents_for_testing());
}

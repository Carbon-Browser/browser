// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/extensions/extensions_toolbar_controls.h"

#include "base/test/metrics/user_action_tester.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/extensions/site_permissions_helper.h"
#include "chrome/browser/ui/views/extensions/extensions_request_access_button.h"
#include "chrome/browser/ui/views/extensions/extensions_toolbar_button.h"
#include "chrome/browser/ui/views/extensions/extensions_toolbar_container.h"
#include "chrome/browser/ui/views/extensions/extensions_toolbar_unittest.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension_features.h"
#include "extensions/test/permissions_manager_waiter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view_utils.h"
#include "url/origin.h"

class ExtensionsToolbarControlsUnitTest : public ExtensionsToolbarUnitTest {
 public:
  ExtensionsToolbarControlsUnitTest();
  ~ExtensionsToolbarControlsUnitTest() override = default;
  ExtensionsToolbarControlsUnitTest(const ExtensionsToolbarControlsUnitTest&) =
      delete;
  const ExtensionsToolbarControlsUnitTest& operator=(
      const ExtensionsToolbarControlsUnitTest&) = delete;

  ExtensionsRequestAccessButton* request_access_button();
  ExtensionsToolbarButton* site_access_button();

  // Returns whether the request access button is visible or not.
  bool IsRequestAccessButtonVisible();

  // Returns whether the site access button is visible or not.
  bool IsSiteAccessButtonVisible();

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

ExtensionsToolbarControlsUnitTest::ExtensionsToolbarControlsUnitTest() {
  scoped_feature_list_.InitAndEnableFeature(
      extensions_features::kExtensionsMenuAccessControl);
}

ExtensionsRequestAccessButton*
ExtensionsToolbarControlsUnitTest::request_access_button() {
  return extensions_container()
      ->GetExtensionsToolbarControls()
      ->request_access_button_for_testing();
}

ExtensionsToolbarButton*
ExtensionsToolbarControlsUnitTest::site_access_button() {
  return extensions_container()
      ->GetExtensionsToolbarControls()
      ->site_access_button_for_testing();
}

bool ExtensionsToolbarControlsUnitTest::IsRequestAccessButtonVisible() {
  return request_access_button()->GetVisible();
}

bool ExtensionsToolbarControlsUnitTest::IsSiteAccessButtonVisible() {
  return site_access_button()->GetVisible();
}

TEST_F(ExtensionsToolbarControlsUnitTest,
       SiteAccessButtonVisibility_NavigationBetweenPages) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // Add an extension that only requests access to a specific url.
  InstallExtensionWithHostPermissions("specific_url", {url_a.spec()});
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Navigate to an url the extension should have access to.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_TRUE(IsSiteAccessButtonVisible());

  // Navigate to an url the extension should not have access to.
  web_contents_tester->NavigateAndCommit(url_b);
  EXPECT_FALSE(IsSiteAccessButtonVisible());
}

TEST_F(ExtensionsToolbarControlsUnitTest,
       SiteAccessButtonVisibility_ContextMenuChangesHostPermissions) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // Add an extension with all urls host permissions. Since we haven't navigated
  // to an url yet, the extension should not have access.
  auto extension =
      InstallExtensionWithHostPermissions("all_urls", {"<all_urls>"});
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Navigate to an url the extension should have access to as part of
  // <all_urls>.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_TRUE(IsSiteAccessButtonVisible());

  // Change the extension to run only on the current site using the context
  // menu. The extension should still have access to the current site.
  extensions::ExtensionContextMenuModel context_menu(
      extension.get(), browser(), extensions::ExtensionContextMenuModel::PINNED,
      nullptr, true,
      extensions::ExtensionContextMenuModel::ContextMenuSource::kToolbarAction);
  context_menu.ExecuteCommand(
      extensions::ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_SITE, 0);
  EXPECT_TRUE(IsSiteAccessButtonVisible());

  // Navigate to a different url. The extension should not have access.
  web_contents_tester->NavigateAndCommit(url_b);
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Go back to the original url. The extension should have access.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_TRUE(IsSiteAccessButtonVisible());
}

TEST_F(ExtensionsToolbarControlsUnitTest,
       SiteAccessButtonVisibility_MultipleExtensions) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // There are no extensions installed yet, so no extension has access to the
  // current site.
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Add an extension that doesn't request host permissions. Extension should
  // not have access to the current site.
  InstallExtension("no_permissions");
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Add an extension that only requests access to url_a. Extension should not
  // have access to the current site.
  InstallExtensionWithHostPermissions("specific_url", {url_a.spec()});
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Add an extension with all urls host permissions. Extension should not have
  // access because there isn't a real url yet.
  auto extension_all_urls =
      InstallExtensionWithHostPermissions("all_urls", {"<all_urls>"});
  EXPECT_FALSE(IsSiteAccessButtonVisible());

  // Navigate to the url that "specific_url" extension has access to. Both
  // "all_urls" and "specific_urls" should have accessn to the current site.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_TRUE(IsSiteAccessButtonVisible());

  // Navigate to a different url. Only "all_urls" should have access.
  web_contents_tester->NavigateAndCommit(url_b);
  EXPECT_TRUE(IsSiteAccessButtonVisible());

  // TODO(crbug.com/1304959): Remove the only extension that requests access to
  // the current site to verify no extension has access to the current
  // site. Uninstall extension in unit tests is flaky.
}

// TODO(crbug.com/1321562) Disabled for flakiness.
TEST_F(ExtensionsToolbarControlsUnitTest,
       DISABLED_RequestAccessButtonVisibility_NavigationBetweenPages) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // Add an extension that only requests access to a specific url, and withhold
  // site access.
  auto extension_a =
      InstallExtensionWithHostPermissions("Extension A", {url_a.spec()});
  WithholdHostPermissions(extension_a.get());
  EXPECT_FALSE(IsRequestAccessButtonVisible());

  // Navigate to an url the extension requests access to.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_TRUE(IsRequestAccessButtonVisible());
  EXPECT_EQ(
      request_access_button()->GetText(),
      l10n_util::GetStringFUTF16Int(IDS_EXTENSIONS_REQUEST_ACCESS_BUTTON, 1));

  // Navigate to an url the extension does not request access to.
  web_contents_tester->NavigateAndCommit(url_b);
  EXPECT_FALSE(IsRequestAccessButtonVisible());
}

// TODO(crbug.com/1321562) Disabled for flakiness.
TEST_F(
    ExtensionsToolbarControlsUnitTest,
    DISABLED_RequestAccessButtonVisibility_ContextMenuChangesHostPermissions) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // Add an extension with all urls host permissions. Since we haven't navigated
  // to an url yet, the extension should not request access.
  auto extension =
      InstallExtensionWithHostPermissions("Extension AllUrls", {"<all_urls>"});
  EXPECT_FALSE(IsRequestAccessButtonVisible());

  // Navigate to an url the extension should have access to as part of
  // <all_urls>, since permissions are granted by default.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_FALSE(IsRequestAccessButtonVisible());

  extensions::ExtensionContextMenuModel context_menu(
      extension.get(), browser(), extensions::ExtensionContextMenuModel::PINNED,
      nullptr, true,
      extensions::ExtensionContextMenuModel::ContextMenuSource::kToolbarAction);

  // Change the extension to run only on click using the context
  // menu. The extension should request access to the current site.
  {
    extensions::PermissionsManagerWaiter waiter(
        extensions::PermissionsManager::Get(profile()));
    context_menu.ExecuteCommand(
        extensions::ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_CLICK, 0);
    waiter.WaitForExtensionPermissionsUpdate();
    EXPECT_TRUE(IsRequestAccessButtonVisible());
    EXPECT_EQ(
        request_access_button()->GetText(),
        l10n_util::GetStringFUTF16Int(IDS_EXTENSIONS_REQUEST_ACCESS_BUTTON, 1));
  }

  // Change the extension to run only on site using the context
  // menu. The extension should not request access to the current site.
  {
    extensions::PermissionsManagerWaiter waiter(
        extensions::PermissionsManager::Get(profile()));
    context_menu.ExecuteCommand(
        extensions::ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_SITE, 0);
    waiter.WaitForExtensionPermissionsUpdate();
    EXPECT_FALSE(IsRequestAccessButtonVisible());
  }
}

// TODO(crbug.com/1321562) Disabled for flakiness.
TEST_F(ExtensionsToolbarControlsUnitTest,
       DISABLED_RequestAccessButtonVisibility_MultipleExtensions) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url_a("http://www.a.com");
  const GURL url_b("http://www.b.com");

  // Navigate to a.com and since there are no extensions installed yet, no
  // extension is requesting access to the current site.
  web_contents_tester->NavigateAndCommit(url_a);
  EXPECT_FALSE(IsRequestAccessButtonVisible());

  // Add an extension that doesn't request host permissions.
  InstallExtension("no_permissions");
  EXPECT_FALSE(IsRequestAccessButtonVisible());

  // Add an extension that only requests access to a.com, and
  // withhold host permissions.
  auto extension_a =
      InstallExtensionWithHostPermissions("Extension A", {url_a.spec()});
  WithholdHostPermissions(extension_a.get());
  EXPECT_TRUE(IsRequestAccessButtonVisible());
  EXPECT_EQ(
      request_access_button()->GetText(),
      l10n_util::GetStringFUTF16Int(IDS_EXTENSIONS_REQUEST_ACCESS_BUTTON, 1));

  // Add an extension with all urls host permissions, and withhold host
  // permissions.
  auto extension_all_urls =
      InstallExtensionWithHostPermissions("Extension AllUrls", {"<all_urls>"});
  WithholdHostPermissions(extension_all_urls.get());
  EXPECT_TRUE(IsRequestAccessButtonVisible());
  EXPECT_EQ(
      request_access_button()->GetText(),
      l10n_util::GetStringFUTF16Int(IDS_EXTENSIONS_REQUEST_ACCESS_BUTTON, 2));

  // Navigate to a different url. Only "all_urls" should request access.
  web_contents_tester->NavigateAndCommit(url_b);
  EXPECT_TRUE(IsRequestAccessButtonVisible());
  EXPECT_EQ(
      request_access_button()->GetText(),
      l10n_util::GetStringFUTF16Int(IDS_EXTENSIONS_REQUEST_ACCESS_BUTTON, 1));

  // TODO(crbug.com/1304959): Remove the only extension that requests access to
  // the current site to verify no extension should have access to the current
  // site. Uninstall extension in unit tests is flaky.
}

// TODO(crbug.com/3671898): Add a test that checks the correct dialog is open
// when clicking on request access button.

// Tests that extensions with activeTab and requested url with withheld access
// are taken into account for the request access button visibility, but not the
// ones with just activeTab.
// TODO(crbug.com/1339370): Withholding host permissions is flaky when the test
// is run multiple times.
TEST_F(ExtensionsToolbarControlsUnitTest,
       DISABLED_RequestAccessButtonVisibility_ActiveTabExtensions) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL requested_url("http://www.requested-url.com");

  InstallExtensionWithPermissions("Extension A", {"activeTab"});
  constexpr char kExtensionName[] = "Extension B";
  auto extension = InstallExtensionWithHostPermissions(
      kExtensionName, {requested_url.spec(), "activeTab"});
  WithholdHostPermissions(extension.get());

  web_contents_tester->NavigateAndCommit(requested_url);
  EXPECT_TRUE(IsRequestAccessButtonVisible());
  EXPECT_THAT(request_access_button()->GetExtensionsNamesForTesting(),
              testing::ElementsAre(kExtensionName));

  web_contents_tester->NavigateAndCommit(
      GURL("http://www.non-requested-url.com"));
  EXPECT_FALSE(IsRequestAccessButtonVisible());
}

// Test that request access button is visible based on the user site setting
// selected.
TEST_F(ExtensionsToolbarControlsUnitTest,
       RequestAccessButtonVisibility_UserSiteSetting) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();
  const GURL url("http://www.url.com");
  auto url_origin = url::Origin::Create(url);

  // Install an extension and withhold permissions so request access button can
  // be visible.
  auto extension =
      InstallExtensionWithHostPermissions("Extension", {"<all_urls>"});
  WithholdHostPermissions(extension.get());

  web_contents_tester->NavigateAndCommit(url);
  WaitForAnimation();

  // A site has "customize by extensions" site setting by default,
  ASSERT_EQ(
      GetUserSiteSetting(url),
      extensions::PermissionsManager::UserSiteSetting::kCustomizeByExtension);
  EXPECT_TRUE(IsRequestAccessButtonVisible());

  auto* manager = extensions::PermissionsManager::Get(profile());
  {
    // Request access button is not visible in permitted sites.
    extensions::PermissionsManagerWaiter manager_waiter(
        extensions::PermissionsManager::Get(profile()));
    manager->AddUserPermittedSite(url_origin);
    manager_waiter.WaitForUserPermissionsSettingsChange();
    WaitForAnimation();
    EXPECT_FALSE(IsRequestAccessButtonVisible());
  }

  {
    // Request access button is not visible in restricted sites.
    extensions::PermissionsManagerWaiter manager_waiter(
        extensions::PermissionsManager::Get(profile()));
    manager->AddUserRestrictedSite(url_origin);
    manager_waiter.WaitForUserPermissionsSettingsChange();
    WaitForAnimation();
    EXPECT_FALSE(IsRequestAccessButtonVisible());
  }

  {
    // Request acesss button is visible if site is not permitted or restricted,
    // and at least one extension is requesting access.
    extensions::PermissionsManagerWaiter manager_waiter(
        extensions::PermissionsManager::Get(profile()));
    manager->RemoveUserRestrictedSite(url_origin);
    manager_waiter.WaitForUserPermissionsSettingsChange();
    WaitForAnimation();
    EXPECT_TRUE(IsRequestAccessButtonVisible());
  }
}

// TODO(crbug.com/1339370): Withholding host permissions is flaky when the test
// is run multiple times.
TEST_F(ExtensionsToolbarControlsUnitTest,
       DISABLED_RequestAccessButton_OnPressedExecuteAction) {
  content::WebContentsTester* web_contents_tester =
      AddWebContentsAndGetTester();

  auto extension =
      InstallExtensionWithHostPermissions("Extension", {"<all_urls>"});
  WithholdHostPermissions(extension.get());

  const GURL url("http://www.example.com");
  web_contents_tester->NavigateAndCommit(url);
  WaitForAnimation();
  LayoutContainerIfNecessary();

  constexpr char kActivatedUserAction[] =
      "Extensions.Toolbar.ExtensionsActivatedFromRequestAccessButton";
  base::UserActionTester user_action_tester;
  extensions::SitePermissionsHelper permissions(browser()->profile());

  // Request access button is visible because extension A is requesting
  // access.
  ASSERT_TRUE(request_access_button()->GetVisible());
  EXPECT_EQ(user_action_tester.GetActionCount(kActivatedUserAction), 0);
  EXPECT_EQ(permissions.GetSiteAccess(*extension, url),
            extensions::SitePermissionsHelper::SiteAccess::kOnClick);

  ClickButton(request_access_button());

  WaitForAnimation();
  LayoutContainerIfNecessary();

  // Verify request access button is hidden since extension executed its
  // action. Extension's site access should have not changed, since clicking the
  // button grants one time access.
  ASSERT_FALSE(request_access_button()->GetVisible());
  EXPECT_EQ(user_action_tester.GetActionCount(kActivatedUserAction), 1);
  EXPECT_EQ(permissions.GetSiteAccess(*extension, url),
            extensions::SitePermissionsHelper::SiteAccess::kOnClick);
}

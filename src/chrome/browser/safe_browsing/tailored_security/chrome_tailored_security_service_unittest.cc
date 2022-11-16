// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/tailored_security/chrome_tailored_security_service.h"

#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/signin/identity_test_environment_profile_adaptor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/prefs/testing_pref_store.h"
#include "components/safe_browsing/core/common/features.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/sync_preferences/pref_model_associator_client.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class Profile;

namespace safe_browsing {

#if !BUILDFLAG(IS_ANDROID)
// Names for Tailored Security status to make the test cases clearer.
const bool kTailoredSecurityEnabled = true;
const bool kTailoredSecurityDisabled = false;
#endif

namespace {
// Test implementation of ChromeTailoredSecurityService.
class TestChromeTailoredSecurityService : public ChromeTailoredSecurityService {
 public:
  explicit TestChromeTailoredSecurityService(Profile* profile)
      : ChromeTailoredSecurityService(profile) {}
  ~TestChromeTailoredSecurityService() override = default;

  // Returns the most recent value of `show_enable_dialog` that was provided to
  // `DisplayDesktopDialog`.
  //
  // This method should be used in conjunction with `times_dialog_displayed` to
  // ensure that the dialog has been displayed the number of times you expected.
  bool previous_show_enable_dialog_value() const {
    return previous_show_enable_dialog_value_;
  }

  // Returns the number of times that `DisplayDesktopDialog` has been called.
  int times_dialog_displayed() const {
    return times_display_desktop_dialog_called_;
  }

  // ChromeTailoredSecurityService:
  // This method is overridden so we can detect the number of times that the
  // dialog has been requested to be shown and what the last value was.
  void DisplayDesktopDialog(Browser* browser,
                            content::WebContents* web_contents,
                            bool show_enable_dialog) override {
    previous_show_enable_dialog_value_ = show_enable_dialog;
    times_display_desktop_dialog_called_++;
  }

  // overridden to make the method public for testing.
  void MaybeNotifySyncUser(bool is_enabled,
                           base::Time previous_update) override {
    ChromeTailoredSecurityService::MaybeNotifySyncUser(is_enabled,
                                                       previous_update);
  }

 private:
  bool previous_show_enable_dialog_value_ = false;
  int times_display_desktop_dialog_called_ = 0;
};
}  // namespace

class ChromeTailoredSecurityServiceTest : public testing::Test {
 public:
  ChromeTailoredSecurityServiceTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~ChromeTailoredSecurityServiceTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile(
        "primary_account", IdentityTestEnvironmentProfileAdaptor::
                               GetIdentityTestEnvironmentFactories());
    identity_test_env_adaptor_ =
        std::make_unique<IdentityTestEnvironmentProfileAdaptor>(profile_);
    GetIdentityTestEnv()->MakePrimaryAccountAvailable(
        "test@foo.com", signin::ConsentLevel::kSync);
    prefs_ = profile_->GetTestingPrefService();

    browser_window_ = std::make_unique<TestBrowserWindow>();
    Browser::CreateParams params(profile(), true);
    params.type = Browser::TYPE_NORMAL;
    params.window = browser_window_.get();
    browser_ = std::unique_ptr<Browser>(Browser::Create(params));

    chrome_tailored_security_service_ =
        std::make_unique<TestChromeTailoredSecurityService>(profile_);
  }

  void TearDown() override {
    // Remove any tabs in the tab strip otherwise the test will crash.
    if (browser_) {
      while (!browser_->tab_strip_model()->empty()) {
        browser_->tab_strip_model()->DetachAndDeleteWebContentsAt(0);
      }
    }

    browser_.reset();
    browser_window_.reset();
    chrome_tailored_security_service_->Shutdown();
    chrome_tailored_security_service_.reset();

    if (profile_) {
      content::StoragePartitionConfig default_storage_partition_config =
          content::StoragePartitionConfig::CreateDefault(profile());
      auto* partition = profile_->GetStoragePartition(
          default_storage_partition_config, /*can_create=*/false);
      if (partition) {
        partition->WaitForDeletionTasksForTesting();
      }
      DestroyProfile();
    }
  }

  TestChromeTailoredSecurityService* tailored_security_service() {
    return chrome_tailored_security_service_.get();
  }

  Browser* browser() { return browser_.get(); }

  // Add a tab with a test `content::WebContents` to the browser.
  content::WebContents* AddTab(const GURL& url) {
    std::unique_ptr<content::WebContents> web_contents(
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
    content::WebContents* raw_contents = web_contents.get();

    browser()->tab_strip_model()->AppendWebContents(std::move(web_contents),
                                                    true);
    EXPECT_EQ(browser()->tab_strip_model()->GetActiveWebContents(),
              raw_contents);

    content::NavigationSimulator::NavigateAndCommitFromBrowser(raw_contents,
                                                               url);
    EXPECT_EQ(url, raw_contents->GetLastCommittedURL());

    return raw_contents;
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() {
    return profile_->GetTestingPrefService();
  }

  TestingProfile* profile() { return profile_; }

  signin::IdentityTestEnvironment* GetIdentityTestEnv() {
    DCHECK(identity_test_env_adaptor_);
    return identity_test_env_adaptor_->identity_test_env();
  }

  void DestroyProfile() {
    identity_test_env_adaptor_.reset();
    profile_ = nullptr;
    profile_manager_.DeleteTestingProfile("primary_account");
  }

  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  // Must be declared before anything that may make use of the
  // directory so as to ensure files are closed before cleanup.
  base::ScopedTempDir temp_dir_;
  content::BrowserTaskEnvironment task_environment_;
  // This is required to create browser tabs in the tests.
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  raw_ptr<sync_preferences::TestingPrefServiceSyncable> prefs_;
  signin::IdentityTestEnvironment identity_test_environment_;
  std::unique_ptr<IdentityTestEnvironmentProfileAdaptor>
      identity_test_env_adaptor_;
  TestingProfileManager profile_manager_;
  TestingProfile* profile_;
  std::unique_ptr<TestBrowserWindow> browser_window_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<TestChromeTailoredSecurityService>
      chrome_tailored_security_service_;
};

#if !BUILDFLAG(IS_ANDROID)
// Some of the test names are shorted using "Ts" for Tailored Security, "Ep"
// for Enhanced Protection and "Sb" for Safe Browsing.

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeDisabledAndTailoredSecurityEnabledDoesNotShowDialog) {
  scoped_feature_list_.InitAndDisableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    int initial_times_displayed =
        tailored_security_service()->times_dialog_displayed();
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());
    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed);
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeDisabledAndTsEnabledDoesNotEnableEp) {
  scoped_feature_list_.InitAndDisableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());
    EXPECT_FALSE(IsEnhancedProtectionEnabled(*prefs()));
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndTailoredSecurityEnabledShowsEnableDialog) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    int initial_times_displayed =
        tailored_security_service()->times_dialog_displayed();

    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());

    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed + 1);
    EXPECT_TRUE(
        tailored_security_service()->previous_show_enable_dialog_value());
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndTsEnabledEnablesEp) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    EXPECT_FALSE(IsEnhancedProtectionEnabled(*prefs()));
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());
    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpAlreadyEnabledDoesNotShowDialog) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::ENHANCED_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    int initial_times_displayed =
        tailored_security_service()->times_dialog_displayed();
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());
    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed);
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpAlreadyEnabledLeavesEpEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::ENHANCED_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());
    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpWasEnabledByTsAndTsNowDisabledShowsDialog) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    int initial_times_displayed =
        tailored_security_service()->times_dialog_displayed();
    // Enable ESB - this will display the dialog once.
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());

    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed + 1);
    // Then detect that TailoredSecurity was disabled.
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());
    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed + 2);
    EXPECT_FALSE(
        tailored_security_service()->previous_show_enable_dialog_value());
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpEnabledByTsAndTsNowDisabledDisablesEp) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    // Enable ESB
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityEnabled,
                                                     base::Time::Now());

    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
    // Then detect that TailoredSecurity was disabled.
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());

    EXPECT_FALSE(IsEnhancedProtectionEnabled(*prefs()));
    EXPECT_TRUE(IsSafeBrowsingEnabled(*prefs()));
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndSpEnabledAndTsNowDisabledDoesNotShowDialog) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);
    int initial_times_displayed =
        tailored_security_service()->times_dialog_displayed();
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());
    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              initial_times_displayed);
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndSpEnabledAndTsNowDisabledDoesNotChangeSb) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::STANDARD_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);

    EXPECT_FALSE(IsEnhancedProtectionEnabled(*prefs()));
    EXPECT_TRUE(IsSafeBrowsingEnabled(*prefs()));

    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());
    EXPECT_FALSE(IsEnhancedProtectionEnabled(*prefs()));
    EXPECT_TRUE(IsSafeBrowsingEnabled(*prefs()));
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpEnabledByUserTsDisabledDoesNotShowDialog) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::ENHANCED_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);

    int times_dialog_displayed_before =
        tailored_security_service()->times_dialog_displayed();
    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());
    EXPECT_EQ(tailored_security_service()->times_dialog_displayed(),
              times_dialog_displayed_before);
  }
}

TEST_F(ChromeTailoredSecurityServiceTest,
       WhenDesktopNoticeEnabledAndEpEnabledByUserTsDisabledDoesNotChangeSb) {
  scoped_feature_list_.InitAndEnableFeature(kTailoredSecurityDesktopNotice);
  {
    SetSafeBrowsingState(prefs(), SafeBrowsingState::ENHANCED_PROTECTION);
    const GURL google_url("https://www.google.com");
    AddTab(google_url);

    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
    EXPECT_FALSE(prefs()->GetBoolean(
        prefs::kEnhancedProtectionEnabledViaTailoredSecurity));

    tailored_security_service()->MaybeNotifySyncUser(kTailoredSecurityDisabled,
                                                     base::Time::Now());
    EXPECT_TRUE(IsEnhancedProtectionEnabled(*prefs()));
  }
}

#endif

}  // namespace safe_browsing

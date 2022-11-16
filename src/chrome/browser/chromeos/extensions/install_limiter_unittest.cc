// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/install_limiter.h"

#include "ash/components/tpm/stub_install_attributes.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/browser/ash/login/demo_mode/demo_mode_test_helper.h"
#include "chrome/browser/ash/login/demo_mode/demo_session.h"
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/verifier_formats.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::InstallLimiter;
using testing::Field;
using testing::Invoke;
using testing::Mock;

namespace {

constexpr char kRandomExtensionId[] = "abacabadabacabaeabacabadabacabaf";

constexpr int kLargeExtensionSize = 2000000;
constexpr int kSmallExtensionSize = 200000;

constexpr char kLargeExtensionCrx[] = "large.crx";
constexpr char kSmallExtensionCrx[] = "small.crx";

constexpr base::TimeDelta kLessThanExpectedWaitTime = base::Seconds(4);
constexpr base::TimeDelta kTimeDeltaUntilExpectedWaitTime = base::Seconds(1);

}  // namespace

class InstallLimiterShouldDeferInstallTest
    : public testing::TestWithParam<ash::DemoSession::DemoModeConfig> {
 public:
  InstallLimiterShouldDeferInstallTest()
      : scoped_user_manager_(std::make_unique<ash::FakeChromeUserManager>()) {}

  InstallLimiterShouldDeferInstallTest(
      const InstallLimiterShouldDeferInstallTest&) = delete;
  InstallLimiterShouldDeferInstallTest& operator=(
      const InstallLimiterShouldDeferInstallTest&) = delete;

  ~InstallLimiterShouldDeferInstallTest() override = default;

 private:
  content::BrowserTaskEnvironment task_environment_;
  ash::ScopedStubInstallAttributes test_install_attributes_;
  user_manager::ScopedUserManager scoped_user_manager_;
};

TEST_P(InstallLimiterShouldDeferInstallTest, ShouldDeferInstall) {
  const std::vector<std::string> screensaver_ids = {
      extension_misc::kScreensaverAppId, extension_misc::kScreensaverAtlasAppId,
      extension_misc::kScreensaverKraneZdksAppId};

  ash::DemoModeTestHelper demo_mode_test_helper;
  if (GetParam() != ash::DemoSession::DemoModeConfig::kNone)
    demo_mode_test_helper.InitializeSession(GetParam());

  // In demo mode, all apps larger than 1MB except for the screensaver
  // should be deferred.
  for (const std::string& id : screensaver_ids) {
    bool expected_defer_install =
        GetParam() == ash::DemoSession::DemoModeConfig::kNone ||
        id != ash::DemoSession::GetScreensaverAppId();
    EXPECT_EQ(expected_defer_install,
              InstallLimiter::ShouldDeferInstall(kLargeExtensionSize, id));
  }
  EXPECT_TRUE(InstallLimiter::ShouldDeferInstall(kLargeExtensionSize,
                                                 kRandomExtensionId));
  EXPECT_FALSE(InstallLimiter::ShouldDeferInstall(kSmallExtensionSize,
                                                  kRandomExtensionId));
}

INSTANTIATE_TEST_SUITE_P(
    DemoModeConfig,
    InstallLimiterShouldDeferInstallTest,
    ::testing::Values(ash::DemoSession::DemoModeConfig::kNone,
                      ash::DemoSession::DemoModeConfig::kOnline));

namespace extensions {

// A mock around CrxInstaller to track extension installations.
class MockCrxInstaller : public CrxInstaller {
 public:
  explicit MockCrxInstaller(ExtensionService* frontend)
      : CrxInstaller(frontend->AsWeakPtr(), nullptr, nullptr) {}

  MOCK_METHOD(void, InstallCrxFile, (const CRXFileInfo& source_file));

 private:
  ~MockCrxInstaller() override = default;
};

}  // namespace extensions

class InstallLimiterTest : public extensions::ExtensionServiceTestBase {
 public:
  InstallLimiterTest()
      : extensions::ExtensionServiceTestBase(
            std::make_unique<content::BrowserTaskEnvironment>(
                base::test::TaskEnvironment::MainThreadType::IO,
                content::BrowserTaskEnvironment::TimeSource::MOCK_TIME)) {}

  InstallLimiterTest(const InstallLimiterTest&) = delete;
  InstallLimiterTest& operator=(const InstallLimiterTest&) = delete;

  ~InstallLimiterTest() override = default;

  void NotifyCrxInstallerDone() {
    content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_CRX_INSTALLER_DONE,
        content::Source<extensions::MockCrxInstaller>(mock_installer_.get()),
        content::Details<const extensions::Extension>(NULL));
  }

 protected:
  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();

    extensions::ExtensionServiceTestBase::ExtensionServiceInitParams params =
        CreateDefaultInitParams();
    params.enable_install_limiter = true;
    InitializeExtensionService(params);

    install_limiter_ = InstallLimiter::Get(profile());

    mock_installer_ =
        base::MakeRefCounted<extensions::MockCrxInstaller>(service());
  }

  extensions::CRXFileInfo CreateTestExtensionCrx(const base::FilePath& path,
                                                 int extension_size) {
    const std::string data(extension_size, 0);
    EXPECT_TRUE(base::WriteFile(path, data));
    extensions::CRXFileInfo crx_info(path, extensions::GetTestVerifierFormat());
    crx_info.extension_id = kRandomExtensionId;
    return crx_info;
  }

  InstallLimiter* install_limiter_;
  scoped_refptr<extensions::MockCrxInstaller> mock_installer_;
};

// Test that small extensions are installed immediately.
TEST_F(InstallLimiterTest, DontDeferSmallExtensionInstallation) {
  const base::FilePath path =
      extensions_install_dir().AppendASCII(kSmallExtensionCrx);
  extensions::CRXFileInfo crx_info_small =
      CreateTestExtensionCrx(path, kSmallExtensionSize);

  EXPECT_CALL(*mock_installer_,
              InstallCrxFile(Field(&extensions::CRXFileInfo::path, path)));
  install_limiter_->Add(mock_installer_, crx_info_small);
  task_environment()->RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_installer_.get());
}

// Test that large extension installations are deferred.
TEST_F(InstallLimiterTest, DeferLargeExtensionInstallation) {
  const base::FilePath path =
      extensions_install_dir().AppendASCII(kLargeExtensionCrx);
  extensions::CRXFileInfo crx_info_large =
      CreateTestExtensionCrx(path, kLargeExtensionSize);

  // Check that the large extension will not be installed immediately.
  EXPECT_CALL(*mock_installer_,
              InstallCrxFile(Field(&extensions::CRXFileInfo::path, path)))
      .Times(0);
  install_limiter_->Add(mock_installer_, crx_info_large);
  task_environment()->FastForwardBy(kLessThanExpectedWaitTime);
  task_environment()->RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_installer_.get());

  // The installation starts only after the wait time is elapsed.
  EXPECT_CALL(*mock_installer_,
              InstallCrxFile(Field(&extensions::CRXFileInfo::path, path)));
  task_environment()->FastForwardBy(kTimeDeltaUntilExpectedWaitTime);
  task_environment()->RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_installer_.get());
}

// Test that deferred installations are run before the wait time expires if the
// OnAllExternalProvidersReady() signal was called.
TEST_F(InstallLimiterTest, RunDeferredInstallsWhenAllExternalProvidersReady) {
  const base::FilePath path =
      extensions_install_dir().AppendASCII(kLargeExtensionCrx);
  extensions::CRXFileInfo crx_info_large =
      CreateTestExtensionCrx(path, kLargeExtensionSize);

  // Check that the large extension will not be installed immediately.
  EXPECT_CALL(*mock_installer_,
              InstallCrxFile(Field(&extensions::CRXFileInfo::path, path)))
      .Times(0);
  install_limiter_->Add(mock_installer_, crx_info_large);
  task_environment()->FastForwardBy(kLessThanExpectedWaitTime);
  task_environment()->RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_installer_.get());

  // The installation starts before the wait time is elapsed if
  // OnAllExternalProvidersReady() is called.
  EXPECT_CALL(*mock_installer_,
              InstallCrxFile(Field(&extensions::CRXFileInfo::path, path)));
  install_limiter_->OnAllExternalProvidersReady();
  task_environment()->RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_installer_.get());
}

// Test that small extensions are installed before large extensions.
TEST_F(InstallLimiterTest, InstallSmallBeforeLargeExtensions) {
  // Create a large test extension crx file.
  const base::FilePath crx_path_large =
      extensions_install_dir().AppendASCII(kLargeExtensionCrx);
  extensions::CRXFileInfo crx_info_large =
      CreateTestExtensionCrx(crx_path_large, kLargeExtensionSize);

  // Create a small test extension crx file.
  const base::FilePath crx_path_small =
      extensions_install_dir().AppendASCII(kSmallExtensionCrx);
  extensions::CRXFileInfo crx_info_small =
      CreateTestExtensionCrx(crx_path_small, kSmallExtensionSize);

  base::RunLoop run_loop;

  // When adding a large extension and then a small extension, the small
  // extension will be installed first. The mock function call will trigger a
  // CRX_INSTALLER_DONE notification which will notify the install limiter to
  // continue with any deferred installations. This will then start the
  // installation of the large extension.
  {
    testing::InSequence s;

    EXPECT_CALL(
        *mock_installer_,
        InstallCrxFile(Field(&extensions::CRXFileInfo::path, crx_path_small)))
        .WillOnce(Invoke(this, &InstallLimiterTest::NotifyCrxInstallerDone));
    EXPECT_CALL(
        *mock_installer_,
        InstallCrxFile(Field(&extensions::CRXFileInfo::path, crx_path_large)))
        .WillOnce(Invoke(&run_loop, &base::RunLoop::Quit));
  }

  install_limiter_->Add(mock_installer_, crx_info_large);
  // Ensure that AddWithSize() is called for the large extension before also
  // adding the small extension.
  task_environment()->RunUntilIdle();
  install_limiter_->Add(mock_installer_, crx_info_small);

  run_loop.Run();

  Mock::VerifyAndClearExpectations(mock_installer_.get());
}

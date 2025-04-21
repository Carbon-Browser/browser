// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/login/demo_mode/demo_login_controller.h"

#include "ash/constants/ash_features.h"
#include "ash/constants/ash_pref_names.h"
#include "ash/login/test_login_screen.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ash/login/existing_user_controller.h"
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/ash/settings/scoped_cros_settings_test_helper.h"
#include "chrome/browser/ash/settings/scoped_testing_cros_settings.h"
#include "chrome/browser/ui/ash/login/mock_login_display_host.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chromeos/ash/components/dbus/session_manager/fake_session_manager_client.h"
#include "chromeos/ash/components/demo_mode/utils/demo_session_utils.h"
#include "chromeos/ash/components/install_attributes/stub_install_attributes.h"
#include "chromeos/ash/components/settings/cros_settings_names.h"
#include "chromeos/ash/components/system/fake_statistics_provider.h"
#include "components/account_id/account_id.h"
#include "components/policy/core/common/device_local_account_type.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "google_apis/gaia/gaia_id.h"
#include "google_apis/google_api_keys.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace {
constexpr char kValidGaiaCreds[] =
    R"({
      "username":"example@gmail.com",
      "obfuscatedGaiaId":"%s",
      "authorizationCode":"abc"
    })";

constexpr char kInValidGaiaCreds[] =
    R"({
      "username":"example@gmail.com",
      "gaia_id":"123"
    })";

constexpr char kSetupDemoAccountUrl[] =
    "https://demomode-pa.googleapis.com/v1/accounts";

constexpr char kCleanUpDemoAccountUrl[] =
    "https://demomode-pa.googleapis.com/v1/accounts:remove";

constexpr char kApiKeyParam[] = "key";

constexpr char kPublicAccountUserId[] = "public_session_user@localhost";

constexpr char kTestGaiaId[] = "123";
constexpr char kTestEmail[] = "example@gmail.com";

}  // namespace

class DemoLoginControllerTest : public testing::Test {
 protected:
  ash::MockLoginDisplayHost& login_display_host() {
    return mock_login_display_host_;
  }

  DemoLoginController* demo_login_controller() {
    return demo_login_controller_.get();
  }

  LoginScreenClientImpl* login_screen_client() {
    return login_screen_client_.get();
  }

  void SetUp() override {
    features_.InitAndEnableFeature(features::kDemoModeSignIn);

    settings_helper_.InstallAttributes()->SetDemoMode();
    fake_user_manager_->AddPublicAccountUser(auto_login_account_id_);
    settings_helper_.ReplaceDeviceSettingsProviderWithStub();

    base::Value::Dict account;
    account.Set(kAccountsPrefDeviceLocalAccountsKeyId, kPublicAccountUserId);
    account.Set(
        kAccountsPrefDeviceLocalAccountsKeyType,
        static_cast<int>(policy::DeviceLocalAccountType::kPublicSession));
    base::Value::List accounts;
    accounts.Append(std::move(account));
    settings_helper_.Set(kAccountsPrefDeviceLocalAccounts,
                         base::Value(std::move(accounts)));

    auth_events_recorder_ = ash::AuthEventsRecorder::CreateForTesting();

    login_screen_client_ = std::make_unique<LoginScreenClientImpl>();
    demo_login_controller_ =
        std::make_unique<DemoLoginController>(login_screen_client_.get());

    TestingBrowserProcess::GetGlobal()->SetSharedURLLoaderFactory(
        test_url_loader_factory_.GetSafeWeakWrapper());
    system::StatisticsProvider::SetTestProvider(&statistics_provider_);
  }

  void TearDown() override {
    demo_login_controller_.reset();
    login_screen_client_.reset();
  }

  GURL GetSetupUrl() {
    return net::AppendQueryParameter(GURL(kSetupDemoAccountUrl), kApiKeyParam,
                                     google_apis::GetAPIKey());
  }

  GURL GetCleanUpUrl() {
    return net::AppendQueryParameter(GURL(kCleanUpDemoAccountUrl), kApiKeyParam,
                                     google_apis::GetAPIKey());
  }

  // Mock a setup response return provided `gaia_id`. Verify that setup request
  // gets triggered and login is success.
  void MockSuccessSetupResponseAndVerifyLogin(const GaiaId& gaia_id) {
    // Mock a setup request will be success.
    test_url_loader_factory_.AddResponse(
        GetSetupUrl().spec(),
        base::StringPrintf(kValidGaiaCreds, gaia_id.ToString()));
    // Expect login if after clean up success.
    base::RunLoop loop;
    EXPECT_CALL(login_display_host(), CompleteLogin)
        .Times(1)
        .WillOnce(testing::Invoke([&](const UserContext& user_context) {
          const auto device_id = user_context.GetDeviceId();
          EXPECT_FALSE(device_id.empty());
          EXPECT_EQ(g_browser_process->local_state()->GetString(
                        prefs::kDemoModeSessionIdentifier),
                    device_id);
          EXPECT_EQ(GaiaId(g_browser_process->local_state()->GetString(
                        prefs::kDemoAccountGaiaId)),
                    gaia_id);

          loop.Quit();
        }));
    loop.Run();
  }

  void ExpectGetExistingController() {
    EXPECT_CALL(login_display_host(), GetExistingUserController())
        .WillRepeatedly(testing::Return(&existing_user_controller_));
  }

  ScopedCrosSettingsTestHelper* settings_helper() { return &settings_helper_; }
  ExistingUserController* existing_user_controller() {
    return &existing_user_controller_;
  }

  void AppendTestUserToUserList() {
    EXPECT_EQ(1U, fake_user_manager_->GetUsers().size());
    fake_user_manager_->AddUser(AccountId::FromNonCanonicalEmail(
        kTestEmail, GaiaId(kTestGaiaId), AccountType::GOOGLE));
    // Expect 2 users: test user with `kTestGaiaId` and public account user.
    EXPECT_EQ(2U, fake_user_manager_->GetUsers().size());
  }

  void ExpectOnlyDeviceLocalAccountInUserList() {
    const auto user_list = fake_user_manager_->GetUsers();
    EXPECT_EQ(1U, user_list.size());
    EXPECT_TRUE(user_list[0]->IsDeviceLocalAccount());
  }

  // FakeChromeUserManager* user_manager() { return fake_user_manager_.Get(); }

  network::TestURLLoaderFactory test_url_loader_factory_;

 private:
  base::test::ScopedFeatureList features_;
  content::BrowserTaskEnvironment task_environment_;

  testing::NiceMock<ash::MockLoginDisplayHost> mock_login_display_host_;
  ScopedTestingLocalState local_state_{TestingBrowserProcess::GetGlobal()};
  system::FakeStatisticsProvider statistics_provider_;

  // Required for `user_manager::UserList`:
  std::unique_ptr<ash::AuthEventsRecorder> auth_events_recorder_;

  // Dependencies for `ExistingUserController`:
  FakeSessionManagerClient fake_session_manager_client_;
  ScopedCrosSettingsTestHelper settings_helper_;
  user_manager::TypedScopedUserManager<ash::FakeChromeUserManager>
      fake_user_manager_{std::make_unique<FakeChromeUserManager>()};
  session_manager::SessionManager session_manager_;
  const AccountId auto_login_account_id_ =
      AccountId::FromUserEmail(policy::GenerateDeviceLocalAccountUserId(
          kPublicAccountUserId,
          policy::DeviceLocalAccountType::kPublicSession));
  ExistingUserController existing_user_controller_;

  // Dependencies for `LoginScreenClientImpl`:
  TestLoginScreen test_login_screen_;
  std::unique_ptr<LoginScreenClientImpl> login_screen_client_;

  std::unique_ptr<DemoLoginController> demo_login_controller_;
};

TEST_F(DemoLoginControllerTest, OnSetupDemoAccountSuccessFirstTime) {
  const GaiaId gaia_id(kTestGaiaId);
  test_url_loader_factory_.AddResponse(
      GetSetupUrl().spec(),
      base::StringPrintf(kValidGaiaCreds, gaia_id.ToString()));
  EXPECT_TRUE(g_browser_process->local_state()
                  ->GetString(prefs::kDemoModeSessionIdentifier)
                  .empty());
  base::RunLoop loop;
  EXPECT_CALL(login_display_host(), CompleteLogin)
      .Times(1)
      .WillOnce(testing::Invoke([&](const UserContext& user_context) {
        const auto device_id = user_context.GetDeviceId();
        EXPECT_FALSE(device_id.empty());
        EXPECT_EQ(g_browser_process->local_state()->GetString(
                      prefs::kDemoModeSessionIdentifier),
                  device_id);
        EXPECT_EQ(GaiaId(g_browser_process->local_state()->GetString(
                      prefs::kDemoAccountGaiaId)),
                  gaia_id);
        loop.Quit();
      }));
  login_screen_client()->OnLoginScreenShown();
  // For first time setup demo account, no clean up get triggered.
  ASSERT_FALSE(test_url_loader_factory_.IsPending(GetCleanUpUrl().spec()));
  loop.Run();
}

TEST_F(DemoLoginControllerTest, InValidGaia) {
  test_url_loader_factory_.AddResponse(GetSetupUrl().spec(), kInValidGaiaCreds);
  ExpectGetExistingController();
  base::RunLoop loop;
  EXPECT_CALL(login_display_host(), CompleteLogin).Times(0);
  demo_login_controller()->SetSetupFailedCallbackForTest(
      base::BindLambdaForTesting(
          [&](const DemoLoginController::ResultCode result_code) {
            EXPECT_EQ(result_code,
                      DemoLoginController::ResultCode::kInvalidCreds);
            loop.Quit();
          }));
  login_screen_client()->OnLoginScreenShown();
  loop.Run();
}

TEST_F(DemoLoginControllerTest, ServerCleanUpSuccess) {
  AppendTestUserToUserList();
  auto* local_state = g_browser_process->local_state();
  local_state->SetString(prefs::kDemoAccountGaiaId, kTestGaiaId);
  const std::string last_session_id = "device_id";
  local_state->SetString(prefs::kDemoModeSessionIdentifier, last_session_id);
  base::MockCallback<DemoLoginController::FailedRequestCallback>
      cleanup_failed_callback;
  // `cleanup_failed_callback` is not called means no failure for clean up.
  EXPECT_CALL(cleanup_failed_callback, Run(testing::_)).Times(0);
  demo_login_controller()->SetCleanUpFailedCallbackForTest(
      cleanup_failed_callback.Get());

  login_screen_client()->OnLoginScreenShown();

  // Verify the request was sent.
  ASSERT_TRUE(test_url_loader_factory_.IsPending(GetCleanUpUrl().spec()));
  test_url_loader_factory_.AddResponse(GetCleanUpUrl().spec(), "{}");

  MockSuccessSetupResponseAndVerifyLogin(GaiaId("234"));
  const auto new_session_id =
      local_state->GetString(prefs::kDemoModeSessionIdentifier);
  EXPECT_NE(new_session_id, last_session_id);

  ExpectOnlyDeviceLocalAccountInUserList();
}

TEST_F(DemoLoginControllerTest, ServerCleanUpFailed) {
  AppendTestUserToUserList();
  auto* local_state = g_browser_process->local_state();
  local_state->SetString(prefs::kDemoAccountGaiaId, "123");
  const std::string last_session_id = "device_id";
  local_state->SetString(prefs::kDemoModeSessionIdentifier, last_session_id);
  test_url_loader_factory_.AddResponse(GetCleanUpUrl().spec(), "{}",
                                       net::HTTP_UNAUTHORIZED);
  base::RunLoop loop;
  demo_login_controller()->SetCleanUpFailedCallbackForTest(
      base::BindLambdaForTesting(
          [&](const DemoLoginController::ResultCode result_code) {
            EXPECT_EQ(result_code,
                      DemoLoginController::ResultCode::kRequestFailed);
            loop.Quit();
          }));

  // Verify login screen shown will trigger clean up and `loop` will quick on
  // fail callback gets invoked.
  login_screen_client()->OnLoginScreenShown();
  loop.Run();

  // Verify login:
  MockSuccessSetupResponseAndVerifyLogin(GaiaId("234"));

  const auto new_session_id =
      local_state->GetString(prefs::kDemoModeSessionIdentifier);
  EXPECT_NE(new_session_id, last_session_id);

  // Expect test account is removed from local even server clean up failed.
  ExpectOnlyDeviceLocalAccountInUserList();
}

TEST_F(DemoLoginControllerTest, FallbackToMGS) {
  // Mock setup failed by returning invalid credential.
  test_url_loader_factory_.AddResponse(GetSetupUrl().spec(), kInValidGaiaCreds);
  ExpectGetExistingController();

  // Configure auto login settings. This is done by policy in prod env.
  settings_helper()->SetString(kAccountsPrefDeviceLocalAccountAutoLoginId,
                               kPublicAccountUserId);
  settings_helper()->SetInteger(kAccountsPrefDeviceLocalAccountAutoLoginDelay,
                                0);

  base::RunLoop loop;
  EXPECT_CALL(login_display_host(), CompleteLogin).Times(0);
  demo_login_controller()->SetSetupFailedCallbackForTest(
      base::BindLambdaForTesting(
          [&](const DemoLoginController::ResultCode result_code) {
            loop.Quit();
          }));
  login_display_host().StartSignInScreen();
  login_screen_client()->OnLoginScreenShown();
  loop.Run();

  // Expect auto login managed guest session starts.
  EXPECT_TRUE(existing_user_controller()->IsAutoLoginTimerRunningForTesting());
}

// TODO(crbug.com/372771485): Add more request fail test cases.

}  // namespace ash

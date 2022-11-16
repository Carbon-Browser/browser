// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/strings/string_util.h"
#include "chrome/browser/ash/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/extensions/telemetry/api/api_guard_delegate.h"
#include "chrome/browser/chromeos/extensions/telemetry/api/base_telemetry_extension_browser_test.h"
#include "chrome/browser/chromeos/extensions/telemetry/api/fake_api_guard_delegate.h"
#include "chrome/browser/chromeos/extensions/telemetry/api/fake_hardware_info_delegate.h"
#include "chrome/common/chromeos/extensions/chromeos_system_extension_info.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/user_manager/scoped_user_manager.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/net_errors.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chromeos {

namespace {

std::string GetServiceWorkerForError(const std::string& error) {
  std::string service_worker = R"(
    const tests = [
      // Telemetry APIs.
      async function getBatteryInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getBatteryInfo(),
            'Error: Unauthorized access to ' +
            'chrome.os.telemetry.getBatteryInfo.' + ' %s'
        );
        chrome.test.succeed();
      },
      async function getCpuInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getCpuInfo(),
            'Error: Unauthorized access to chrome.os.telemetry.getCpuInfo.' +
            ' %s'
        );
        chrome.test.succeed();
      },
      async function getMemoryInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getMemoryInfo(),
            'Error: Unauthorized access to chrome.os.telemetry.getMemoryInfo.' +
            ' %s'
        );
        chrome.test.succeed();
      },
      async function getOemData() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getOemData(),
            'Error: Unauthorized access to chrome.os.telemetry.getOemData. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function getOsVersionInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getOsVersionInfo(),
            'Error: Unauthorized access to ' +
            'chrome.os.telemetry.getOsVersionInfo. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function getStatefulPartitionInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getStatefulPartitionInfo(),
            'Error: Unauthorized access to ' +
            'chrome.os.telemetry.getStatefulPartitionInfo.' +
            ' %s'
        );
        chrome.test.succeed();
      },
      async function getVpdInfo() {
        await chrome.test.assertPromiseRejects(
            chrome.os.telemetry.getVpdInfo(),
            'Error: Unauthorized access to chrome.os.telemetry.getVpdInfo. ' +
            '%s'
        );
        chrome.test.succeed();
      },

      // Diagnostics APIs.
      async function getAvailableRoutines() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.getAvailableRoutines(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.getAvailableRoutines. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function getRoutineUpdate() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.getRoutineUpdate(
              {
                id: 12345,
                command: "status"
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.getRoutineUpdate. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runAcPowerRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runAcPowerRoutine(
              {
                expected_status: "connected",
                expected_power_type: "ac_power"
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runAcPowerRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runBatteryCapacityRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runBatteryCapacityRoutine(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runBatteryCapacityRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runBatteryChargeRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runBatteryChargeRoutine(
              {
                length_seconds: 1000,
                minimum_charge_percent_required: 1
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runBatteryChargeRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runBatteryDischargeRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runBatteryDischargeRoutine(
              {
                length_seconds: 10,
                maximum_discharge_percent_allowed: 15
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runBatteryDischargeRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runBatteryHealthRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runBatteryHealthRoutine(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runBatteryHealthRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runCpuCacheRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runCpuCacheRoutine(
              {
                length_seconds: 120
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runCpuCacheRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runCpuFloatingPointAccuracyRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runCpuFloatingPointAccuracyRoutine(
              {
                length_seconds: 120
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runCpuFloatingPointAccuracyRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runCpuPrimeSearchRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runCpuPrimeSearchRoutine(
              {
                length_seconds: 120
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runCpuPrimeSearchRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runCpuStressRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runCpuStressRoutine(
              {
                length_seconds: 120
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runCpuStressRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runDiskReadRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runDiskReadRoutine(
              {
                type: "random",
                length_seconds: 60,
                file_size_mb: 200
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runDiskReadRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runLanConnectivityRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runLanConnectivityRoutine(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runLanConnectivityRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runMemoryRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runMemoryRoutine(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runMemoryRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runNvmeWearLevelRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runNvmeWearLevelRoutine(
              {
                wear_level_threshold: 80
              }
            ),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runNvmeWearLevelRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
      async function runSmartctlCheckRoutine() {
        await chrome.test.assertPromiseRejects(
            chrome.os.diagnostics.runSmartctlCheckRoutine(),
            'Error: Unauthorized access to ' +
            'chrome.os.diagnostics.runSmartctlCheckRoutine. ' +
            '%s'
        );
        chrome.test.succeed();
      },
    ];

    chrome.test.runTests([
      async function allAPIsTested() {
        getTestNames = function(arr) {
          return arr.map(item => item.name);
        }
        getMethods = function(obj) {
          return Object.getOwnPropertyNames(obj).filter(
            item => typeof obj[item] === 'function');
        }
        apiNames = [
          ...getMethods(chrome.os.telemetry),
          ...getMethods(chrome.os.diagnostics)
        ];
        chrome.test.assertEq(getTestNames(tests), apiNames);
        chrome.test.succeed();
      },
      ...tests
    ]);
  )";

  base::ReplaceSubstringsAfterOffset(&service_worker, /*start_offset=*/0, "%s",
                                     error);
  return service_worker;
}

}  // namespace

using TelemetryExtensionApiGuardBrowserTest = BaseTelemetryExtensionBrowserTest;

IN_PROC_BROWSER_TEST_F(TelemetryExtensionApiGuardBrowserTest,
                       CanAccessApiReturnsError) {
  constexpr char kErrorMessage[] = "Test error message";
  // Make sure ApiGuardDelegate::CanAccessApi() returns specified error message.
  api_guard_delegate_factory_ =
      std::make_unique<FakeApiGuardDelegate::Factory>(kErrorMessage);
  ApiGuardDelegate::Factory::SetForTesting(api_guard_delegate_factory_.get());

  CreateExtensionAndRunServiceWorker(GetServiceWorkerForError(kErrorMessage));
}

// Class that use real ApiGuardDelegate instance to verify its behavior.
class TelemetryExtensionApiGuardRealDelegateBrowserTest
    : public BaseTelemetryExtensionBrowserTest {
 public:
  TelemetryExtensionApiGuardRealDelegateBrowserTest()
      : fake_hardware_info_delegate_factory_("HP"),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    // Make sure device manufacturer is allowlisted.
    HardwareInfoDelegate::Factory::SetForTesting(
        &fake_hardware_info_delegate_factory_);
  }
  ~TelemetryExtensionApiGuardRealDelegateBrowserTest() override = default;

  TelemetryExtensionApiGuardRealDelegateBrowserTest(
      const TelemetryExtensionApiGuardRealDelegateBrowserTest&) = delete;
  TelemetryExtensionApiGuardRealDelegateBrowserTest& operator=(
      const TelemetryExtensionApiGuardRealDelegateBrowserTest&) = delete;

  // BaseTelemetryExtensionBrowserTest:
  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());

    BaseTelemetryExtensionBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    BaseTelemetryExtensionBrowserTest::SetUpCommandLine(command_line);

    command_line->AppendSwitchASCII(
        chromeos::switches::kTelemetryExtensionPwaOriginOverrideForTesting,
        pwa_page_url());
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    // Skip BaseTelemetryExtensionBrowserTest::SetUpOnMainThread() as it sets up
    // a FakeApiGuardDelegate instance.
    extensions::ExtensionBrowserTest::SetUpOnMainThread();

    mock_cert_verifier()->set_default_result(net::OK);

    // Must be initialized before dealing with UserManager.
    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        std::make_unique<ash::FakeChromeUserManager>());

    https_server_.StartAcceptingConnections();

    // This is needed when navigating to a network URL (e.g.
    // ui_test_utils::NavigateToURL). Rules can only be added before
    // BrowserTestBase::InitializeNetworkProcess() is called because host
    // changes will be disabled afterwards.
    host_resolver()->AddRule("*", "127.0.0.1");
  }
  void TearDownOnMainThread() override {
    // Explicitly removing the user is required; otherwise ProfileHelper keeps
    // a dangling pointer to the User.
    // TODO(b/208629291): Consider removing all users from ProfileHelper in the
    // destructor of ash::FakeChromeUserManager.
    GetFakeUserManager()->RemoveUserFromList(
        GetFakeUserManager()->GetActiveUser()->GetAccountId());
    user_manager_enabler_.reset();

    ASSERT_TRUE(https_server_.ShutdownAndWaitUntilComplete());

    BaseTelemetryExtensionBrowserTest::TearDownOnMainThread();
  }

  void SetUpInProcessBrowserTestFixture() override {
    BaseTelemetryExtensionBrowserTest::SetUpInProcessBrowserTestFixture();

    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();

    BaseTelemetryExtensionBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::ContentMockCertVerifier::CertVerifier* mock_cert_verifier() {
    return mock_cert_verifier_.mock_cert_verifier();
  }

 protected:
  ash::FakeChromeUserManager* GetFakeUserManager() const {
    return static_cast<ash::FakeChromeUserManager*>(
        user_manager::UserManager::Get());
  }

  GURL GetPwaGURL() const { return https_server_.GetURL("/ssl/google.html"); }

  // BaseTelemetryExtensionBrowserTest:
  std::string pwa_page_url() const override { return GetPwaGURL().spec(); }
  std::string matches_origin() const override { return GetPwaGURL().spec(); }

  FakeHardwareInfoDelegate::Factory fake_hardware_info_delegate_factory_;
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// Smoke test to verify that real ApiGuardDelegate works in prod.
IN_PROC_BROWSER_TEST_F(TelemetryExtensionApiGuardRealDelegateBrowserTest,
                       CanAccessRunBatteryCapacityRoutine) {
  // Add a new user and make it owner.
  auto* const user_manager = GetFakeUserManager();
  const AccountId account_id = AccountId::FromUserEmail("user@example.com");
  user_manager->AddUser(account_id);
  user_manager->LoginUser(account_id);
  user_manager->SwitchActiveUser(account_id);
  user_manager->SetOwnerId(account_id);

  // Make sure PWA UI is open and secure.
  auto* pwa_page_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(pwa_page_url()));
  ASSERT_TRUE(pwa_page_rfh);

  CreateExtensionAndRunServiceWorker(R"(
    chrome.test.runTests([
      async function runBatteryCapacityRoutine() {
        const response =
          await chrome.os.diagnostics.runBatteryCapacityRoutine();
        chrome.test.assertEq({id: 0, status: "ready"}, response);
        chrome.test.succeed();
      }
    ]);
  )");
}

}  // namespace chromeos

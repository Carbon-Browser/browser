// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/guest_os/guest_os_session_tracker.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "chrome/browser/ash/guest_os/dbus_test_helper.h"
#include "chrome/browser/ash/guest_os/public/types.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/ash/components/dbus/cicerone/cicerone_service.pb.h"
#include "chromeos/ash/components/dbus/cicerone/fake_cicerone_client.h"
#include "chromeos/ash/components/dbus/concierge/concierge_service.pb.h"
#include "chromeos/ash/components/dbus/concierge/fake_concierge_client.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace guest_os {

class GuestOsSessionTrackerTest : public testing::Test,
                                  protected guest_os::FakeVmServicesHelper {
 public:
  GuestOsSessionTrackerTest() {
    vm_started_signal_.set_owner_id(OwnerId());
    vm_started_signal_.set_name("vm_name");

    vm_shutdown_signal_.set_name("vm_name");
    vm_shutdown_signal_.set_owner_id(OwnerId());

    container_started_signal_.set_vm_name("vm_name");
    container_started_signal_.set_owner_id(OwnerId());
    container_started_signal_.set_container_name("penguin");

    container_shutdown_signal_.set_container_name("penguin");
    container_shutdown_signal_.set_vm_name("vm_name");
    container_shutdown_signal_.set_owner_id(OwnerId());
  }
  std::string OwnerId() {
    return ash::ProfileHelper::GetUserIdHashFromProfile(&profile_);
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  base::RunLoop run_loop_;
  GuestOsSessionTracker tracker_{OwnerId()};

  vm_tools::concierge::VmStartedSignal vm_started_signal_;
  vm_tools::concierge::VmStoppedSignal vm_shutdown_signal_;
  vm_tools::cicerone::ContainerStartedSignal container_started_signal_;
  vm_tools::cicerone::ContainerShutdownSignal container_shutdown_signal_;
};

TEST_F(GuestOsSessionTrackerTest, ContainerAddedOnStartup) {
  vm_tools::concierge::VmStartedSignal signal;
  signal.set_owner_id(OwnerId());
  signal.set_name("vm_name");
  signal.mutable_vm_info()->set_cid(32);
  FakeConciergeClient()->NotifyVmStarted(signal);
  vm_tools::cicerone::ContainerStartedSignal cicerone_signal;
  cicerone_signal.set_container_name("penguin");
  cicerone_signal.set_owner_id(OwnerId());
  cicerone_signal.set_vm_name("vm_name");
  cicerone_signal.set_container_username("username");
  cicerone_signal.set_container_homedir("/home");
  cicerone_signal.set_ipv4_address("1.2.3.4");
  FakeCiceroneClient()->NotifyContainerStarted(cicerone_signal);

  auto info = tracker_.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));

  EXPECT_EQ(info->guest_id.vm_name, "vm_name");
  EXPECT_EQ(info->guest_id.container_name, "penguin");
  EXPECT_EQ(info->username, cicerone_signal.container_username());
  EXPECT_EQ(info->homedir, base::FilePath(cicerone_signal.container_homedir()));
  EXPECT_EQ(info->cid, signal.vm_info().cid());
  EXPECT_EQ(info->ipv4_address, cicerone_signal.ipv4_address());
}

TEST_F(GuestOsSessionTrackerTest, ContainerRemovedOnContainerShutdown) {
  FakeConciergeClient()->NotifyVmStarted(vm_started_signal_);
  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);

  auto info = tracker_.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));
  ASSERT_NE(info, absl::nullopt);

  FakeCiceroneClient()->NotifyContainerShutdownSignal(
      container_shutdown_signal_);

  info = tracker_.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));
  EXPECT_EQ(info, absl::nullopt);
}

TEST_F(GuestOsSessionTrackerTest, ContainerRemovedOnVmShutdown) {
  FakeConciergeClient()->NotifyVmStarted(vm_started_signal_);
  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);

  auto info = tracker_.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));
  ASSERT_NE(info, absl::nullopt);

  FakeConciergeClient()->NotifyVmStopped(vm_shutdown_signal_);

  info = tracker_.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));
  EXPECT_EQ(info, absl::nullopt);
}

TEST_F(GuestOsSessionTrackerTest, AlreadyRunningVMsTracked) {
  vm_tools::concierge::ListVmsResponse response;
  vm_tools::concierge::ExtendedVmInfo vm_info;
  vm_info.set_owner_id(OwnerId());
  vm_info.set_name("vm_name");
  *response.add_vms() = vm_info;
  response.set_success(true);
  FakeConciergeClient()->set_list_vms_response(response);

  GuestOsSessionTracker tracker{OwnerId()};
  run_loop_.RunUntilIdle();

  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);

  auto info = tracker.GetInfo(GuestId(VmType::UNKNOWN, "vm_name", "penguin"));
  ASSERT_NE(info, absl::nullopt);
}

TEST_F(GuestOsSessionTrackerTest, AlreadyRunningContainersTracked) {
  // TODO(b/217469540): We need to expose additional information from Cicerone
  // to be able to adopt up already-running containers.
}

TEST_F(GuestOsSessionTrackerTest, RunOnceContainerStartedAlreadyRunning) {
  FakeConciergeClient()->NotifyVmStarted(vm_started_signal_);
  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);
  GuestId id{VmType::UNKNOWN, "vm_name", "penguin"};
  bool called = false;
  auto _ = tracker_.RunOnceContainerStarted(
      id,
      base::BindLambdaForTesting([&called](GuestInfo info) { called = true; }));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(called);
}

TEST_F(GuestOsSessionTrackerTest, RunOnceContainerStartedDelayedStart) {
  FakeConciergeClient()->NotifyVmStarted(vm_started_signal_);
  GuestId id{VmType::UNKNOWN, "vm_name", "penguin"};
  bool called = false;
  auto _ = tracker_.RunOnceContainerStarted(
      id,
      base::BindLambdaForTesting([&called](GuestInfo info) { called = true; }));
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(called);
  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(called);
}

TEST_F(GuestOsSessionTrackerTest, RunOnceContainerStartedCancel) {
  FakeConciergeClient()->NotifyVmStarted(vm_started_signal_);
  GuestId id{VmType::UNKNOWN, "vm_name", "penguin"};
  bool called = false;
  static_cast<void>(tracker_.RunOnceContainerStarted(
      id, base::BindLambdaForTesting(
              [&called](GuestInfo info) { called = true; })));
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(called);
  FakeCiceroneClient()->NotifyContainerStarted(container_started_signal_);
  task_environment_.RunUntilIdle();

  // We dropped the subscription, so it should've been cancelled straight away.
  EXPECT_FALSE(called);
}

}  // namespace guest_os

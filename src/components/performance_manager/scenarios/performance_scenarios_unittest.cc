// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/performance_manager/public/scenarios/performance_scenarios.h"

#include <atomic>
#include <utility>

#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/weak_ptr.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/scoped_feature_list.h"
#include "components/performance_manager/public/features.h"
#include "components/performance_manager/public/graph/graph.h"
#include "components/performance_manager/public/graph/process_node.h"
#include "components/performance_manager/public/performance_manager.h"
#include "components/performance_manager/test_support/performance_manager_test_harness.h"
#include "components/performance_manager/test_support/run_in_graph.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/navigation_simulator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/performance/performance_scenario_observer.h"
#include "third_party/blink/public/common/performance/performance_scenarios.h"
#include "url/gurl.h"

namespace performance_manager {

namespace {

using blink::performance_scenarios::GetLoadingScenario;
using blink::performance_scenarios::PerformanceScenarioObserverList;
using blink::performance_scenarios::ScenarioScope;
using ::testing::_;

// Since the browser process also maps in a read-only view of the global
// scenario state for querying outside performance_manager, the blink observer
// is also notified.
class MockPerformanceScenarioObserver
    : public blink::performance_scenarios::PerformanceScenarioObserver {
 public:
 public:
  MOCK_METHOD(void,
              OnLoadingScenarioChanged,
              (ScenarioScope scope,
               LoadingScenario old_scenario,
               LoadingScenario new_scenario),
              (override));
  MOCK_METHOD(void,
              OnInputScenarioChanged,
              (ScenarioScope scope,
               InputScenario old_scenario,
               InputScenario new_scenario),
              (override));
};

using StrictMockPerformanceScenarioObserver =
    ::testing::StrictMock<MockPerformanceScenarioObserver>;

class PerformanceScenariosTest : public PerformanceManagerTestHarness,
                                 public ::testing::WithParamInterface<bool> {
 public:
  PerformanceScenariosTest() {
    // Run with and without PM on main thread, since this can affect whether
    // setting a scenario needs to post a task.
    scoped_feature_list_.InitWithFeatureState(features::kRunOnMainThreadSync,
                                              GetParam());
  }

  void SetUp() override {
    PerformanceManagerTestHarness::SetUp();

    // Load a page with FrameNodes and ProcessNodes.
    SetContents(CreateTestWebContents());
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        web_contents(), GURL("https://www.example.com"));
  }

  // Returns the shared memory region handle for the main frame's process, or an
  // invalid handle if there is none.
  base::ReadOnlySharedMemoryRegion main_process_region() {
    if (!process()) {
      return base::ReadOnlySharedMemoryRegion();
    }
    base::WeakPtr<ProcessNode> process_node =
        PerformanceManager::GetProcessNodeForRenderProcessHost(process());
    base::ReadOnlySharedMemoryRegion process_region;
    RunInGraph([&] {
      ASSERT_TRUE(process_node);
      // GetPerformanceScenarioRegionForProcess() creates writable shared memory
      // for that process' state if it doesn't already exist.
      process_region =
          GetSharedScenarioRegionForProcessNode(process_node.get());
    });
    return process_region;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(All, PerformanceScenariosTest, ::testing::Bool());

TEST_P(PerformanceScenariosTest, SetWithoutSharedMemory) {
  // Can't set up the blink scenario observer without mapped memory.
  EXPECT_FALSE(
      PerformanceScenarioObserverList::GetForScope(ScenarioScope::kGlobal));

  // When the global shared scenario memory isn't set up, setting a scenario
  // should silently do nothing. (Process scenario memory is scoped to the
  // ProcessNode so will always be mapped as needed.)
  SetGlobalLoadingScenario(LoadingScenario::kFocusedPageLoading);
  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kGlobal)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kNoPageLoading);
}

TEST_P(PerformanceScenariosTest, SetWithSharedMemory) {
  base::WeakPtr<ProcessNode> process_node =
      PerformanceManager::GetProcessNodeForRenderProcessHost(process());
  StrictMockPerformanceScenarioObserver mock_observer;
  EXPECT_CALL(mock_observer,
              OnLoadingScenarioChanged(ScenarioScope::kGlobal,
                                       LoadingScenario::kNoPageLoading,
                                       LoadingScenario::kFocusedPageLoading))
      .WillOnce(base::test::RunOnceClosure(task_environment()->QuitClosure()));

  // Create writable shared memory for the global state. This maps a read-only
  // view of the memory in as well so changes immediately become visible to the
  // current (browser) process.
  ScopedGlobalScenarioMemory global_shared_memory;
  auto observer_list =
      PerformanceScenarioObserverList::GetForScope(ScenarioScope::kGlobal);
  ASSERT_TRUE(observer_list);
  observer_list->AddObserver(&mock_observer);

  SetGlobalLoadingScenario(LoadingScenario::kFocusedPageLoading);
  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kGlobal)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kFocusedPageLoading);

  // PerformanceScenarioObserverList is an ObserverListThreadSafe that posts
  // a message to notify. Need to wait for the message.
  task_environment()->RunUntilQuit();
  ::testing::Mock::VerifyAndClearExpectations(&mock_observer);

  // Create writable shared memory for a render process state. Since this is
  // called in the browser process and the state is for a different process, it
  // doesn't map in a read-only view.
  base::ReadOnlySharedMemoryRegion process_region = main_process_region();
  EXPECT_TRUE(process_region.IsValid());
  SetLoadingScenarioForProcess(LoadingScenario::kVisiblePageLoading, process());

  // SetLoadingScenarioForProcess posts to the PM thread. Wait until the message
  // is received before reading.
  RunInGraph([] {
    EXPECT_EQ(GetLoadingScenario(ScenarioScope::kCurrentProcess)
                  ->load(std::memory_order_relaxed),
              LoadingScenario::kNoPageLoading);
  });

  // Map in the read-only view of `process_region`. Normally this would be done
  // in the renderer process as the "current process" state. The state should
  // now become visible.
  blink::performance_scenarios::ScopedReadOnlyScenarioMemory
      process_shared_memory(ScenarioScope::kCurrentProcess,
                            std::move(process_region));
  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kCurrentProcess)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kVisiblePageLoading);

  observer_list->RemoveObserver(&mock_observer);
}

// TODO(crbug.com/382551028): Test is flaky on Android.
#if BUILDFLAG(IS_ANDROID)
#define MAYBE_SetFromPMSequence DISABLED_SetFromPMSequence
#else
#define MAYBE_SetFromPMSequence SetFromPMSequence
#endif
TEST_P(PerformanceScenariosTest, MAYBE_SetFromPMSequence) {
  base::WeakPtr<ProcessNode> process_node =
      PerformanceManager::GetProcessNodeForRenderProcessHost(process());

  StrictMockPerformanceScenarioObserver mock_observer;
  EXPECT_CALL(mock_observer,
              OnLoadingScenarioChanged(ScenarioScope::kGlobal,
                                       LoadingScenario::kNoPageLoading,
                                       LoadingScenario::kFocusedPageLoading))
      .WillOnce(base::test::RunOnceClosure(task_environment()->QuitClosure()));

  // Create writable shared memory for the global state. This maps a read-only
  // view of the memory in as well.
  ScopedGlobalScenarioMemory global_shared_memory;
  auto observer_list =
      PerformanceScenarioObserverList::GetForScope(ScenarioScope::kGlobal);
  ASSERT_TRUE(observer_list);
  observer_list->AddObserver(&mock_observer);

  // Create writable shared memory for a render process state. Since this is
  // called in the browser process and the state is for a different process, it
  // doesn't map in a read-only view.
  base::ReadOnlySharedMemoryRegion process_region = main_process_region();
  EXPECT_TRUE(process_region.IsValid());

  // Set the loading scenario from the PM sequence.
  RunInGraph([&] {
    ASSERT_TRUE(process_node);
    SetLoadingScenarioForProcessNode(LoadingScenario::kVisiblePageLoading,
                                     process_node.get());
    SetGlobalLoadingScenario(LoadingScenario::kFocusedPageLoading);
  });

  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kCurrentProcess)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kNoPageLoading);
  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kGlobal)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kFocusedPageLoading);

  // PerformanceScenarioObserverList is an ObserverListThreadSafe that posts
  // a message to notify. Need to wait for the message.
  task_environment()->RunUntilQuit();
  ::testing::Mock::VerifyAndClearExpectations(&mock_observer);

  // Map in the read-only view of `process_region`. Normally this would be done
  // in the renderer process as the "current process" state. The state should
  // now become visible.
  blink::performance_scenarios::ScopedReadOnlyScenarioMemory
      process_shared_memory(ScenarioScope::kCurrentProcess,
                            std::move(process_region));
  EXPECT_EQ(GetLoadingScenario(ScenarioScope::kCurrentProcess)
                ->load(std::memory_order_relaxed),
            LoadingScenario::kVisiblePageLoading);

  observer_list->RemoveObserver(&mock_observer);
}

}  // namespace

}  // namespace performance_manager

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_BUDGETER_H_
#define CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_BUDGETER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/browser/private_aggregation/private_aggregation_budget_key.h"
#include "content/common/content_export.h"

template <class T>
class scoped_refptr;

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {

class PrivateAggregationBudgetStorage;

// UI thread class that provides an interface for querying and updating the
// budget used by each key, i.e. the sum of contributions, by interacting with
// the storage layer. This class is responsible for owning the storage class.
class CONTENT_EXPORT PrivateAggregationBudgeter {
 public:
  // Public for testing
  enum class StorageStatus {
    // The database is in the process of being initialized.
    kInitializing,
    // The database initialization did not succeed.
    kInitializationFailed,
    // The database successfully initialized and can be used.
    kOpen,
  };

  // Maximum budget allowed to be claimed per-origin per-day per-API.
  static constexpr int kMaxBudgetPerScope = 65536;

  // To avoid unbounded memory growth, limit the number of pending consume
  // budget calls during initialization.
  static constexpr int kMaxPendingCalls = 1000;

  // The total length of time that per-origin per-API budgets are enforced
  // against. Note that there are 24 `PrivateAggregationBudgetKey::TimeWindow`s
  // per `kBudgetScopeDuration`.
  static constexpr base::TimeDelta kBudgetScopeDuration = base::Days(1);
  static_assert(kBudgetScopeDuration %
                    PrivateAggregationBudgetKey::TimeWindow::kDuration ==
                base::TimeDelta());

  // `db_task_runner` should not be nullptr.
  PrivateAggregationBudgeter(
      scoped_refptr<base::SequencedTaskRunner> db_task_runner,
      bool exclusively_run_in_memory,
      const base::FilePath& path_to_db_dir);

  PrivateAggregationBudgeter(const PrivateAggregationBudgeter& other) = delete;
  PrivateAggregationBudgeter& operator=(
      const PrivateAggregationBudgeter& other) = delete;
  virtual ~PrivateAggregationBudgeter();

  // Attempts to consume `budget` for `budget_key`. The callback
  // `on_done` is then run with `true` if the attempt was successful and
  // `false` otherwise.
  //
  // The attempt is rejected if it would cause an origin's daily per-API budget
  // to exceed `kMaxBudgetPerScope` (for the 24-hour period ending at the *end*
  // of `budget_key.time_window`, see `kBudgetScopeDuration` and
  // `PrivateAggregationBudgetKey` for more detail). The attempt is also
  // rejected if the requested `budget` is non-positive, if `budget_key.origin`
  // is not potentially trustworthy or if the database is closed. If the
  // database is initializing, this query is queued until the initialization is
  // complete. Otherwise, the budget use is recorded and the attempt is
  // successful. May clean up stale budget storage. Note that this call assumes
  // that budget time windows are non-decreasing. In very rare cases, a network
  // time update could allow budget to be used slightly early. Virtual for
  // testing.
  virtual void ConsumeBudget(int budget,
                             const PrivateAggregationBudgetKey& budget_key,
                             base::OnceCallback<void(bool)> on_done);

  // TODO(crbug.com/1328439): Clear stale data periodically and on startup.

 protected:
  // Should only be used for testing/mocking to avoid creating the underlying
  // storage.
  PrivateAggregationBudgeter();

  // Virtual for testing.
  virtual void OnStorageDoneInitializing(
      std::unique_ptr<PrivateAggregationBudgetStorage> storage);

  StorageStatus storage_status_ = StorageStatus::kInitializing;

 private:
  void ConsumeBudgetImpl(int additional_budget,
                         const PrivateAggregationBudgetKey& budget_key,
                         base::OnceCallback<void(bool)> on_done);

  void ProcessAllPendingCalls();

  // While the storage initializes, queues calls to ConsumeBudget() in the
  // order the calls are received. Should be empty after storage is initialized.
  // The size is limited to `kMaxPendingCalls`.
  std::vector<base::OnceClosure> pending_consume_budget_calls_;

  // `nullptr` until initialization is complete or if initialization failed.
  // Otherwise, owned by this class until destruction. Iff present,
  // `storage_status_` should be `kOpen`.
  std::unique_ptr<PrivateAggregationBudgetStorage> storage_;

  // Holds a closure that will shut down the initializing storage until
  // initialization is complete. After then, it is null.
  base::OnceClosure shutdown_initializing_storage_;

  base::WeakPtrFactory<PrivateAggregationBudgeter> weak_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_BUDGETER_H_

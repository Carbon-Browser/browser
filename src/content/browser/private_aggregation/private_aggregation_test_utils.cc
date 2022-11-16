// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/private_aggregation/private_aggregation_test_utils.h"

#include <tuple>

#include "content/browser/private_aggregation/private_aggregation_budget_key.h"

namespace content {

bool operator==(const PrivateAggregationBudgetKey::TimeWindow& a,
                const PrivateAggregationBudgetKey::TimeWindow& b) {
  return a.start_time() == b.start_time();
}

bool operator==(const PrivateAggregationBudgetKey& a,
                const PrivateAggregationBudgetKey& b) {
  const auto tie = [](const PrivateAggregationBudgetKey& budget_key) {
    return std::make_tuple(budget_key.origin(), budget_key.time_window(),
                           budget_key.api());
  };
  return tie(a) == tie(b);
}

}  // namespace content

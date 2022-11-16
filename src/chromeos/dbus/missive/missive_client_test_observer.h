// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_MISSIVE_MISSIVE_CLIENT_TEST_OBSERVER_H_
#define CHROMEOS_DBUS_MISSIVE_MISSIVE_CLIENT_TEST_OBSERVER_H_

#include <tuple>

#include "base/test/repeating_test_future.h"
#include "chromeos/dbus/missive/missive_client.h"
#include "components/reporting/proto/synced/record.pb.h"
#include "components/reporting/proto/synced/record_constants.pb.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace chromeos {

// Test helper class that observe |FakeMissiveClient| events.
class MissiveClientTestObserver
    : public MissiveClient::TestInterface::Observer {
 public:
  // If |destination| is specified, the observer will capture only enqueued
  // records with the specified |destination|, otherwise, all records will be
  // captured.
  explicit MissiveClientTestObserver(
      absl::optional<::reporting::Destination> destination = absl::nullopt);

  MissiveClientTestObserver(const MissiveClientTestObserver&) = delete;
  MissiveClientTestObserver operator=(const MissiveClientTestObserver&) =
      delete;

  ~MissiveClientTestObserver() override;

  void OnRecordEnqueued(::reporting::Priority priority,
                        const ::reporting::Record& record) override;

  // Wait for next |::reporting::Record| to be enqueued and return it along with
  // the corresponding |::reporting::Priority|.
  std::tuple<::reporting::Priority, ::reporting::Record>
  GetNextEnqueuedRecord();

  // Return true if there is no new enqueued records that was not consumed by
  // |GetNextEnqueuedRecord()|.
  bool HasNewEnqueuedRecords();

 private:
  base::test::RepeatingTestFuture<::reporting::Priority, ::reporting::Record>
      enqueued_records_;

  const absl::optional<::reporting::Destination> destination_;
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_MISSIVE_MISSIVE_CLIENT_TEST_OBSERVER_H_

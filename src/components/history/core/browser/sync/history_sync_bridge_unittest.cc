// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/sync/history_sync_bridge.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "components/history/core/browser/sync/history_sync_metadata_database.h"
#include "components/history/core/browser/sync/test_history_backend_for_sync.h"
#include "components/sync/base/page_transition_conversion.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/protocol/entity_metadata.pb.h"
#include "components/sync/protocol/history_specifics.pb.h"
#include "components/sync/test/model/forwarding_model_type_change_processor.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace history {

namespace {

using testing::_;
using testing::Return;

sync_pb::HistorySpecifics CreateSpecifics(
    base::Time visit_time,
    const std::string& originator_cache_guid,
    const GURL& url) {
  sync_pb::HistorySpecifics specifics;
  specifics.set_visit_time_windows_epoch_micros(
      visit_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  specifics.set_originator_cache_guid(originator_cache_guid);
  auto* url_entry = specifics.add_redirect_entries();
  url_entry->set_url(url.spec());
  return specifics;
}

syncer::EntityData SpecificsToEntityData(
    const sync_pb::HistorySpecifics& specifics) {
  syncer::EntityData data;
  *data.specifics.mutable_history() = specifics;
  return data;
}

class FakeModelTypeChangeProcessor : public syncer::ModelTypeChangeProcessor {
 public:
  FakeModelTypeChangeProcessor() = default;
  ~FakeModelTypeChangeProcessor() override = default;

  void SetIsTrackingMetadata(bool is_tracking_metadata) {
    is_tracking_metadata_ = is_tracking_metadata;
  }

  void MarkEntitySynced(const std::string& storage_key) {
    DCHECK(unsynced_entities_.count(storage_key));
    unsynced_entities_.erase(storage_key);
  }

  void AddRemoteEntity(const std::string& storage_key,
                       syncer::EntityData entity_data) {
    // Remote entities are tracked, but *not* unsynced.
    tracked_entities_[storage_key] = entity_data.client_tag_hash;
    entities_[storage_key] = std::move(entity_data);
  }

  const std::map<std::string, syncer::EntityData>& GetEntities() const {
    return entities_;
  }

  void Put(const std::string& storage_key,
           std::unique_ptr<syncer::EntityData> entity_data,
           syncer::MetadataChangeList* metadata_change_list) override {
    // Update the persisted metadata.
    sync_pb::EntityMetadata metadata;
    metadata.set_sequence_number(1);
    metadata_change_list->UpdateMetadata(storage_key, metadata);
    // Store the entity, and mark it as tracked and unsynced.
    tracked_entities_[storage_key] = entity_data->client_tag_hash;
    unsynced_entities_.insert(storage_key);
    entities_[storage_key] = std::move(*entity_data);
  }

  void Delete(const std::string& storage_key,
              syncer::MetadataChangeList* metadata_change_list) override {
    NOTREACHED();
  }

  void UpdateStorageKey(
      const syncer::EntityData& entity_data,
      const std::string& storage_key,
      syncer::MetadataChangeList* metadata_change_list) override {
    NOTREACHED();
  }

  void UntrackEntityForStorageKey(const std::string& storage_key) override {
    tracked_entities_.erase(storage_key);
    // If the entity isn't tracked anymore, it also can't be unsynced.
    unsynced_entities_.erase(storage_key);
  }

  void UntrackEntityForClientTagHash(
      const syncer::ClientTagHash& client_tag_hash) override {
    for (const auto& [storage_key, cth] : tracked_entities_) {
      if (cth == client_tag_hash) {
        UntrackEntityForStorageKey(storage_key);
        // Note: This modified `tracked_entities_`, so it's not safe to continue
        // the loop.
        break;
      }
    }
  }

  std::vector<std::string> GetAllTrackedStorageKeys() const override {
    std::vector<std::string> storage_keys;
    for (const auto& [storage_key, cth] : tracked_entities_) {
      storage_keys.push_back(storage_key);
    }
    return storage_keys;
  }

  bool IsEntityUnsynced(const std::string& storage_key) override {
    return unsynced_entities_.count(storage_key) > 0;
  }

  base::Time GetEntityCreationTime(
      const std::string& storage_key) const override {
    NOTREACHED();
    return base::Time();
  }

  base::Time GetEntityModificationTime(
      const std::string& storage_key) const override {
    NOTREACHED();
    return base::Time();
  }

  void OnModelStarting(syncer::ModelTypeSyncBridge* bridge) override {}

  void ModelReadyToSync(std::unique_ptr<syncer::MetadataBatch> batch) override {
  }

  bool IsTrackingMetadata() const override { return is_tracking_metadata_; }

  std::string TrackedAccountId() const override {
    if (!IsTrackingMetadata()) {
      return "";
    }
    return "account_id";
  }

  std::string TrackedCacheGuid() const override {
    if (!IsTrackingMetadata()) {
      return "";
    }
    return "local_cache_guid";
  }

  void ReportError(const syncer::ModelError& error) override {
    ADD_FAILURE() << "ReportError: " << error.ToString();
  }

  absl::optional<syncer::ModelError> GetError() const override {
    return absl::nullopt;
  }

  base::WeakPtr<syncer::ModelTypeControllerDelegate> GetControllerDelegate()
      override {
    NOTREACHED();
    return nullptr;
  }

  const sync_pb::EntitySpecifics& GetPossiblyTrimmedRemoteSpecifics(
      const std::string& storage_key) const override {
    NOTREACHED();
    return sync_pb::EntitySpecifics::default_instance();
  }

  std::unique_ptr<ModelTypeChangeProcessor> CreateForwardingProcessor() {
    return base::WrapUnique<ModelTypeChangeProcessor>(
        new syncer::ForwardingModelTypeChangeProcessor(this));
  }

 private:
  bool is_tracking_metadata_ = false;

  // Map from storage key to EntityData for all entities passed to Put().
  std::map<std::string, syncer::EntityData> entities_;

  // Map from storage key to ClientTagHash for all entities currently tracked by
  // the processor.
  std::map<std::string, syncer::ClientTagHash> tracked_entities_;

  // Set of storage keys of all unsynced entities (i.e. with local changes that
  // are pending commit).
  std::set<std::string> unsynced_entities_;
};

class HistorySyncBridgeTest : public testing::Test {
 public:
  HistorySyncBridgeTest() : metadata_db_(&db_, &meta_table_) {}
  ~HistorySyncBridgeTest() override = default;

 protected:
  void SetUp() override {
    EXPECT_TRUE(db_.OpenInMemory());
    metadata_db_.Init();
    meta_table_.Init(&db_, /*version=*/1, /*compatible_version=*/1);

    // Creating the bridge triggers loading of the metadata, which is
    // synchronous.
    bridge_ = std::make_unique<HistorySyncBridge>(
        &backend_, &metadata_db_, fake_processor_.CreateForwardingProcessor());
  }

  void TearDown() override {
    bridge_.reset();
    db_.Close();
  }

  TestHistoryBackendForSync* backend() { return &backend_; }
  FakeModelTypeChangeProcessor* processor() { return &fake_processor_; }
  HistorySyncBridge* bridge() { return bridge_.get(); }

  std::pair<URLRow, VisitRow> AddVisitToBackendAndAdvanceClock(
      const GURL& url,
      ui::PageTransition transition) {
    // After grabbing the visit time, advance the mock time so that the next
    // visit will get a unique time.
    base::Time visit_time = base::Time::Now();
    task_environment_.FastForwardBy(base::Seconds(1));

    URLRow url_row;
    const URLRow* existing_url_row = backend()->FindURLRow(url);
    if (existing_url_row) {
      url_row = *existing_url_row;
    } else {
      url_row.set_url(url);
      url_row.set_title(u"Title");
      url_row.set_id(backend()->AddURL(url_row));
    }

    VisitRow visit_row;
    visit_row.url_id = url_row.id();
    visit_row.visit_time = visit_time;
    visit_row.transition =
        ui::PageTransitionFromInt(transition | ui::PAGE_TRANSITION_CHAIN_START |
                                  ui::PAGE_TRANSITION_CHAIN_END);
    visit_row.visit_id = backend()->AddVisit(visit_row);

    return {url_row, visit_row};
  }

  syncer::EntityChangeList CreateAddEntityChangeList(
      const std::vector<sync_pb::HistorySpecifics>& specifics_vector) {
    syncer::EntityChangeList entity_change_list;
    for (const sync_pb::HistorySpecifics& specifics : specifics_vector) {
      syncer::EntityData data = SpecificsToEntityData(specifics);
      data.client_tag_hash = syncer::ClientTagHash::FromUnhashed(
          syncer::HISTORY, bridge_->GetClientTag(data));
      std::string storage_key = bridge_->GetStorageKey(data);
      entity_change_list.push_back(
          syncer::EntityChange::CreateAdd(storage_key, std::move(data)));
    }
    return entity_change_list;
  }

  void MergeSyncData(
      const std::vector<sync_pb::HistorySpecifics>& specifics_vector) {
    // Just before the merge, the processor starts tracking metadata.
    processor()->SetIsTrackingMetadata(true);

    // Populate a MetadataChangeList with an update for each entity.
    std::unique_ptr<syncer::MetadataChangeList> metadata_changes =
        bridge()->CreateMetadataChangeList();
    for (const sync_pb::HistorySpecifics& specifics : specifics_vector) {
      syncer::EntityData data = SpecificsToEntityData(specifics);
      data.client_tag_hash = syncer::ClientTagHash::FromUnhashed(
          syncer::HISTORY, bridge_->GetClientTag(data));
      std::string storage_key = bridge_->GetStorageKey(data);
      // Note: Don't bother actually populating the EntityMetadata - the bridge
      // doesn't inspect it anyway.
      metadata_changes->UpdateMetadata(storage_key, sync_pb::EntityMetadata());
      processor()->AddRemoteEntity(storage_key, std::move(data));
    }

    absl::optional<syncer::ModelError> error =
        bridge()->MergeSyncData(std::move(metadata_changes),
                                CreateAddEntityChangeList(specifics_vector));
    if (error) {
      ADD_FAILURE() << "MergeSyncData failed: " << error->ToString();
    }
  }

  void ApplySyncChanges(
      const std::vector<sync_pb::HistorySpecifics>& specifics_vector,
      const std::vector<std::string> extra_updated_metadata_storage_keys = {}) {
    // Populate a MetadataChangeList with the given updates/clears.
    std::unique_ptr<syncer::MetadataChangeList> metadata_changes =
        bridge()->CreateMetadataChangeList();
    // By default, add a metadata update for each new/changed entity.
    for (const sync_pb::HistorySpecifics& specifics : specifics_vector) {
      syncer::EntityData data = SpecificsToEntityData(specifics);
      data.client_tag_hash = syncer::ClientTagHash::FromUnhashed(
          syncer::HISTORY, bridge_->GetClientTag(data));
      std::string storage_key = bridge_->GetStorageKey(data);
      // Note: Don't bother actually populating the EntityMetadata - the bridge
      // doesn't inspect it anyway.
      metadata_changes->UpdateMetadata(storage_key, sync_pb::EntityMetadata());
      processor()->AddRemoteEntity(storage_key, std::move(data));
    }
    // Add additional metadata updates, if specified.
    for (const std::string& storage_key : extra_updated_metadata_storage_keys) {
      // Note: Don't bother actually populating the EntityMetadata - the bridge
      // doesn't inspect it anyway.
      metadata_changes->UpdateMetadata(storage_key, sync_pb::EntityMetadata());
    }

    absl::optional<syncer::ModelError> error =
        bridge()->ApplySyncChanges(std::move(metadata_changes),
                                   CreateAddEntityChangeList(specifics_vector));
    if (error) {
      ADD_FAILURE() << "ApplySyncChanges failed: " << error->ToString();
    }
  }

  void ApplyStopSyncChanges() {
    syncer::MetadataBatch all_metadata;
    metadata_db_.GetAllSyncMetadata(&all_metadata);

    std::unique_ptr<syncer::MetadataChangeList> delete_all_metadata =
        bridge()->CreateMetadataChangeList();
    for (const auto& [storage_key, metadata] : all_metadata.GetAllMetadata()) {
      delete_all_metadata->ClearMetadata(storage_key);
    }
    delete_all_metadata->ClearModelTypeState();

    bridge()->ApplyStopSyncChanges(std::move(delete_all_metadata));

    // After stopping sync, metadata is not tracked anymore.
    processor()->SetIsTrackingMetadata(false);
  }

  syncer::EntityMetadataMap GetAllMetadata() {
    auto metadata_batch = std::make_unique<syncer::MetadataBatch>();
    if (!metadata_db_.GetAllSyncMetadata(metadata_batch.get())) {
      ADD_FAILURE() << "Failed to read metadata from DB";
    }
    return metadata_batch->TakeAllMetadata();
  }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  sql::Database db_;
  sql::MetaTable meta_table_;
  HistorySyncMetadataDatabase metadata_db_;

  TestHistoryBackendForSync backend_;

  FakeModelTypeChangeProcessor fake_processor_;

  std::unique_ptr<HistorySyncBridge> bridge_;
};

TEST_F(HistorySyncBridgeTest, MergeDoesNotUploadData) {
  AddVisitToBackendAndAdvanceClock(GURL("https://www.url.com"),
                                   ui::PAGE_TRANSITION_LINK);

  MergeSyncData({});

  // The data should *not* have been uploaded to Sync.
  EXPECT_TRUE(processor()->GetEntities().empty());

  // The local data should still exist though.
  EXPECT_EQ(backend()->GetURLs().size(), 1u);
  EXPECT_EQ(backend()->GetVisits().size(), 1u);
}

TEST_F(HistorySyncBridgeTest, MergeAppliesRemoteChanges) {
  const std::string remote_cache_guid("remote_cache_guid");
  const GURL local_url("https://local.com");
  const GURL remote_url("https://remote.com");

  AddVisitToBackendAndAdvanceClock(local_url, ui::PAGE_TRANSITION_LINK);

  sync_pb::HistorySpecifics remote_entity = CreateSpecifics(
      base::Time::Now() - base::Minutes(1), remote_cache_guid, remote_url);

  MergeSyncData({remote_entity});

  // The local and remote data should both exist in the DB now.
  // Note: The ordering of the two entries in the backend doesn't really matter.
  ASSERT_EQ(backend()->GetURLs().size(), 2u);
  ASSERT_EQ(backend()->GetVisits().size(), 2u);
  EXPECT_EQ(backend()->GetURLs()[0].url(), local_url);
  EXPECT_EQ(backend()->GetURLs()[1].url(), remote_url);
  EXPECT_EQ(backend()->GetVisits()[0].url_id, backend()->GetURLs()[0].id());
  EXPECT_EQ(backend()->GetVisits()[1].url_id, backend()->GetURLs()[1].id());
  EXPECT_EQ(backend()->GetVisits()[1].originator_cache_guid, remote_cache_guid);
}

TEST_F(HistorySyncBridgeTest, MergeMergesRemoteChanges) {
  const GURL remote_url("https://remote.com");

  sync_pb::HistorySpecifics remote_entity = CreateSpecifics(
      base::Time::Now() - base::Minutes(1), "remote_cache_guid", remote_url);

  // Start Sync the first time, so the remote data gets written to the local DB.
  MergeSyncData({remote_entity});
  ASSERT_EQ(backend()->GetURLs().size(), 1u);
  ASSERT_EQ(backend()->GetVisits().size(), 1u);
  ASSERT_EQ(backend()->GetVisits()[0].visit_duration, base::TimeDelta());

  // Stop Sync, then start it again so the same data gets downloaded again.
  ApplyStopSyncChanges();
  // ...but the data has been updated in the meantime.
  remote_entity.set_visit_duration_micros(1000);
  MergeSyncData({remote_entity});

  // The entries in the local DB should have been updated (*not* duplicated).
  ASSERT_EQ(backend()->GetURLs().size(), 1u);
  ASSERT_EQ(backend()->GetVisits().size(), 1u);
  EXPECT_EQ(backend()->GetVisits()[0].visit_duration, base::Microseconds(1000));
}

TEST_F(HistorySyncBridgeTest, MergeIgnoresInvalidVisits) {
  const std::string remote_cache_guid("remote_cache_guid");
  const GURL remote_url("https://remote.com");

  // Create a bunch of remote entities that are invalid in various ways.
  sync_pb::HistorySpecifics missing_cache_guid =
      CreateSpecifics(base::Time::Now() - base::Minutes(1), "", remote_url);

  sync_pb::HistorySpecifics missing_visit_time =
      CreateSpecifics(base::Time(), remote_cache_guid, remote_url);
  missing_visit_time.clear_visit_time_windows_epoch_micros();

  sync_pb::HistorySpecifics no_redirects = CreateSpecifics(
      base::Time::Now() - base::Minutes(2), remote_cache_guid, remote_url);
  no_redirects.clear_redirect_entries();

  sync_pb::HistorySpecifics too_old = CreateSpecifics(
      base::Time::Now() - TestHistoryBackendForSync::kExpiryThreshold -
          base::Hours(1),
      remote_cache_guid, remote_url);

  sync_pb::HistorySpecifics too_new = CreateSpecifics(
      base::Time::Now() + base::Days(7), remote_cache_guid, remote_url);

  // ...and a single valid one.
  sync_pb::HistorySpecifics valid = CreateSpecifics(
      base::Time::Now() - base::Minutes(10), remote_cache_guid, remote_url);

  MergeSyncData({missing_cache_guid, missing_visit_time, no_redirects, too_old,
                 too_new, valid});

  // None of the invalid entities should've made it into the DB.
  ASSERT_EQ(backend()->GetURLs().size(), 1u);
  ASSERT_EQ(backend()->GetVisits().size(), 1u);
  EXPECT_EQ(backend()->GetURLs()[0].url(), remote_url);
  EXPECT_EQ(backend()->GetVisits()[0].originator_cache_guid, remote_cache_guid);
}

TEST_F(HistorySyncBridgeTest, UploadsNewLocalVisit) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Visit a URL.
  auto [url_row, visit_row] = AddVisitToBackendAndAdvanceClock(
      GURL("https://www.url.com"),
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_TYPED |
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR));

  // Notify the bridge about the visit - it should be sent to the processor.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row.transition, url_row,
      visit_row.visit_time);

  const std::string storage_key =
      HistorySyncMetadataDatabase::StorageKeyFromVisitTime(
          visit_row.visit_time);
  EXPECT_EQ(processor()->GetEntities().size(), 1u);
  ASSERT_EQ(processor()->GetEntities().count(storage_key), 1u);
  const syncer::EntityData& entity = processor()->GetEntities().at(storage_key);

  // Spot check some fields of the resulting entity.
  ASSERT_TRUE(entity.specifics.has_history());
  const sync_pb::HistorySpecifics& history = entity.specifics.history();
  EXPECT_EQ(base::Time::FromDeltaSinceWindowsEpoch(
                base::Microseconds(history.visit_time_windows_epoch_micros())),
            visit_row.visit_time);
  EXPECT_EQ(history.originator_cache_guid(), processor()->TrackedCacheGuid());
  ASSERT_EQ(history.redirect_entries_size(), 1);
  EXPECT_EQ(history.redirect_entries(0).url(), url_row.url());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(
      syncer::FromSyncPageTransition(
          history.page_transition().core_transition()),
      ui::PAGE_TRANSITION_TYPED));
  EXPECT_FALSE(history.page_transition().forward_back());
  EXPECT_TRUE(history.page_transition().from_address_bar());
}

TEST_F(HistorySyncBridgeTest, UploadsUpdatedLocalVisit) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Visit a URL.
  auto [url_row, visit_row] = AddVisitToBackendAndAdvanceClock(
      GURL("https://www.url.com"), ui::PAGE_TRANSITION_TYPED);

  // Notify the bridge about the visit - it should be sent to the processor.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row.transition, url_row,
      visit_row.visit_time);

  const std::string storage_key =
      HistorySyncMetadataDatabase::StorageKeyFromVisitTime(
          visit_row.visit_time);
  EXPECT_EQ(processor()->GetEntities().size(), 1u);
  EXPECT_EQ(processor()->GetEntities().count(storage_key), 1u);

  // Update the visit by adding a duration.
  const base::TimeDelta visit_duration = base::Seconds(10);
  visit_row.visit_duration = visit_duration;
  ASSERT_TRUE(backend()->UpdateVisit(visit_row));
  bridge()->OnVisitUpdated(visit_row);

  // The updated data should have been sent to the processor.
  EXPECT_EQ(processor()->GetEntities().size(), 1u);
  ASSERT_EQ(processor()->GetEntities().count(storage_key), 1u);
  const syncer::EntityData& entity = processor()->GetEntities().at(storage_key);
  EXPECT_EQ(
      base::Microseconds(entity.specifics.history().visit_duration_micros()),
      visit_duration);
}

TEST_F(HistorySyncBridgeTest, UploadsLocalVisitWithRedirects) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Create a redirect chain with 3 entries.
  URLRow url_row1(GURL("https://url1.com"));
  URLID url_id1 = backend()->AddURL(url_row1);
  url_row1.set_id(url_id1);
  URLRow url_row2(GURL("https://url2.com"));
  URLID url_id2 = backend()->AddURL(url_row2);
  url_row2.set_id(url_id2);
  URLRow url_row3(GURL("https://url3.com"));
  URLID url_id3 = backend()->AddURL(url_row3);
  url_row3.set_id(url_id3);

  const base::Time visit_time = base::Time::Now();

  VisitRow visit_row1;
  visit_row1.url_id = url_id1;
  visit_row1.visit_time = visit_time;
  visit_row1.transition = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CHAIN_START |
      ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  VisitID visit_id1 = backend()->AddVisit(visit_row1);
  VisitRow visit_row2;
  visit_row2.referring_visit = visit_id1;
  visit_row2.url_id = url_id2;
  visit_row2.visit_time = visit_time;
  visit_row2.transition = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_SERVER_REDIRECT);
  VisitID visit_id2 = backend()->AddVisit(visit_row2);
  VisitRow visit_row3;
  visit_row3.referring_visit = visit_id2;
  visit_row3.url_id = url_id3;
  visit_row3.visit_time = visit_time;
  visit_row3.transition = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_LINK | ui::PAGE_TRANSITION_CHAIN_END);
  backend()->AddVisit(visit_row3);

  // Notify the bridge about all of the visits.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row1.transition, url_row1, visit_time);
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row2.transition, url_row2, visit_time);
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row3.transition, url_row3, visit_time);

  // The whole chain should have resulting in a single entity being Put().
  const std::string storage_key =
      HistorySyncMetadataDatabase::StorageKeyFromVisitTime(visit_time);
  EXPECT_EQ(processor()->GetEntities().size(), 1u);
  ASSERT_EQ(processor()->GetEntities().count(storage_key), 1u);
  const syncer::EntityData& entity = processor()->GetEntities().at(storage_key);

  // Check that the resulting entity contains the redirect chain.
  ASSERT_TRUE(entity.specifics.has_history());
  const sync_pb::HistorySpecifics& history = entity.specifics.history();
  EXPECT_EQ(base::Time::FromDeltaSinceWindowsEpoch(
                base::Microseconds(history.visit_time_windows_epoch_micros())),
            visit_time);
  EXPECT_EQ(history.originator_cache_guid(), processor()->TrackedCacheGuid());
  ASSERT_EQ(history.redirect_entries_size(), 3);
  EXPECT_EQ(history.redirect_entries(0).url(), url_row1.url());
  EXPECT_EQ(history.redirect_entries(1).url(), url_row2.url());
  EXPECT_EQ(history.redirect_entries(2).url(), url_row3.url());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(
      syncer::FromSyncPageTransition(
          history.page_transition().core_transition()),
      ui::PAGE_TRANSITION_LINK));
}

TEST_F(HistorySyncBridgeTest, UntracksEntitiesAfterCommit) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Visit some URLs.
  auto [url_row1, visit_row1] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url1.com"), ui::PAGE_TRANSITION_TYPED);
  auto [url_row2, visit_row2] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url2.com"), ui::PAGE_TRANSITION_LINK);

  // Notify the bridge about the visits - they should be sent to the processor.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row1.transition, url_row1,
      visit_row1.visit_time);
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row2.transition, url_row2,
      visit_row2.visit_time);

  EXPECT_EQ(processor()->GetEntities().size(), 2u);
  // The metadata for these entities should now be tracked.
  EXPECT_EQ(GetAllMetadata().size(), 2u);

  // Simulate a successful commit, which results in an ApplySyncChanges() call
  // to the bridge, updating the committed entities' metadata.
  std::vector<std::string> updated_storage_keys;
  for (const auto& [storage_key, metadata] : GetAllMetadata()) {
    processor()->MarkEntitySynced(storage_key);
    updated_storage_keys.push_back(storage_key);
  }
  ApplySyncChanges({}, updated_storage_keys);

  // Now the metadata should not be tracked anymore.
  EXPECT_EQ(GetAllMetadata().size(), 0u);
  EXPECT_TRUE(GetAllMetadata().empty());
}

TEST_F(HistorySyncBridgeTest, UntracksRemoteEntities) {
  // Start Sync with an initial remote entity.
  MergeSyncData(
      {CreateSpecifics(base::Time::Now() - base::Seconds(10),
                       "remote_cache_guid", GURL("https://remote.com"))});
  ASSERT_EQ(backend()->GetURLs().size(), 1u);
  ASSERT_EQ(backend()->GetVisits().size(), 1u);
  ASSERT_EQ(backend()->GetVisits()[0].visit_duration, base::TimeDelta());

  // The entity should have been untracked immediately.
  EXPECT_TRUE(GetAllMetadata().empty());

  // Another remote entity comes in.
  ApplySyncChanges(
      {CreateSpecifics(base::Time::Now() - base::Seconds(5),
                       "remote_cache_guid", GURL("https://remote2.com"))});

  // This entity should also have been untracked immediately.
  EXPECT_TRUE(GetAllMetadata().empty());
}

TEST_F(HistorySyncBridgeTest, DoesNotUntrackEntityPendingCommit) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Visit a URL locally.
  auto [url_row1, visit_row1] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url1.com"), ui::PAGE_TRANSITION_TYPED);

  const std::string storage_key =
      HistorySyncMetadataDatabase::StorageKeyFromVisitTime(
          visit_row1.visit_time);

  // Notify the bridge about the visit.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row1.transition, url_row1,
      visit_row1.visit_time);

  EXPECT_EQ(processor()->GetEntities().size(), 1u);

  // The metadata for this entity should now be tracked.
  ASSERT_EQ(GetAllMetadata().size(), 1u);

  // Before the entity gets committed (and thus untracked), a remote entity
  // comes in.
  ApplySyncChanges({CreateSpecifics(base::Time::Now(), "remote_cache_guid",
                                    GURL("https://remote.com"))});

  // The remote entity should have been untracked immediately, but the local
  // entity pending commit should still be tracked.
  syncer::EntityMetadataMap metadata = GetAllMetadata();
  EXPECT_EQ(metadata.size(), 1u);
  EXPECT_EQ(metadata.count(storage_key), 1u);
}

TEST_F(HistorySyncBridgeTest, UntracksEntityOnIndividualDeletion) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Visit some URLs.
  auto [url_row1, visit_row1] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url1.com"), ui::PAGE_TRANSITION_TYPED);
  auto [url_row2, visit_row2] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url2.com"), ui::PAGE_TRANSITION_LINK);

  // Notify the bridge about the visits - they should be sent to the processor.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row1.transition, url_row1,
      visit_row1.visit_time);
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row2.transition, url_row2,
      visit_row2.visit_time);
  ASSERT_EQ(GetAllMetadata().size(), 2u);

  EXPECT_EQ(processor()->GetEntities().size(), 2u);

  // Now, *before* the entities get committed successfully (and thus would get
  // untracked anyway), delete the first URL+visit and notify the bridge. This
  // should not result in any Put() or Delete() calls to the processor
  // (deletions are handled through the separate HISTORY_DELETE_DIRECTIVES data
  // type), but it should untrack the deleted entity.
  backend()->RemoveURLAndVisits(url_row1.id());

  bridge()->OnVisitDeleted(visit_row1);
  bridge()->OnURLsDeleted(/*history_backend=*/nullptr, /*all_history=*/false,
                          /*expired=*/false, {url_row1}, /*favicon_urls=*/{});
  // The metadata for the first (deleted) entity should be gone, but the
  // metadata for the second entity should still exist.
  EXPECT_EQ(GetAllMetadata().size(), 1u);
}

TEST_F(HistorySyncBridgeTest, UntracksAllEntitiesOnAllHistoryDeletion) {
  // Start syncing (with no data yet).
  MergeSyncData({});

  // Add some visits to the DB.
  auto [url_row1, visit_row1] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url1.com"), ui::PAGE_TRANSITION_TYPED);
  auto [url_row2, visit_row2] = AddVisitToBackendAndAdvanceClock(
      GURL("https://url2.com"), ui::PAGE_TRANSITION_LINK);

  // Notify the bridge about the visits - they should be sent to the processor.
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row1.transition, url_row1,
      visit_row1.visit_time);
  bridge()->OnURLVisited(
      /*history_backend=*/nullptr, visit_row2.transition, url_row2,
      visit_row2.visit_time);
  ASSERT_EQ(GetAllMetadata().size(), 2u);

  EXPECT_EQ(processor()->GetEntities().size(), 2u);

  // Now, *before* the entities get committed successfully (and thus would get
  // untracked anyway), simulate a delete-all-history operation. This should not
  // result in any Put() or Delete() calls on the processor (deletions are
  // handled through the separate HISTORY_DELETE_DIRECTIVES data type), but it
  // should untrack all entities.
  backend()->Clear();
  // Deleting all history does *not* result in OnVisitDeleted() calls, and also
  // does not include the actual deleted URLs in OnURLsDeleted().
  bridge()->OnURLsDeleted(/*history_backend=*/nullptr, /*all_history=*/true,
                          /*expired=*/false, /*deleted_rows=*/{},
                          /*favicon_urls=*/{});

  EXPECT_TRUE(GetAllMetadata().empty());
}

}  // namespace

}  // namespace history

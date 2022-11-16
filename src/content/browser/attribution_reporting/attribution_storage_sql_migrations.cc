// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/attribution_reporting/attribution_storage_sql_migrations.h"

#include "base/check.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "content/browser/attribution_reporting/common_source_info.h"
#include "content/browser/attribution_reporting/rate_limit_table.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace content {

namespace {

bool MigrateToVersion34(sql::Database* db, sql::MetaTable* meta_table) {
  // Wrap each migration in its own transaction. This results in smaller
  // transactions, so it's less likely that a transaction's buffer will need to
  // spill to disk. Also, if the database grows a lot and Chrome stops (user
  // quit, process kill, etc.) during the migration process, per-migration
  // transactions make it more likely that we'll make forward progress each time
  // Chrome stops.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  // Create the new aggregatable_report_metadata table with initial_report_time.
  // This follows the steps documented at
  // https://sqlite.org/lang_altertable.html#otheralter. Other approaches, like
  // using "ALTER ... ADD COLUMN" require setting a DEFAULT value for the column
  // which is undesirable.
  static constexpr char kNewAggregatableReportMetadataTableSql[] =
      "CREATE TABLE IF NOT EXISTS new_aggregatable_report_metadata("
      "aggregation_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "source_id INTEGER NOT NULL,"
      "trigger_time INTEGER NOT NULL,"
      "debug_key INTEGER,"
      "external_report_id TEXT NOT NULL,"
      "report_time INTEGER NOT NULL,"
      "failed_send_attempts INTEGER NOT NULL,"
      "initial_report_time INTEGER NOT NULL)";
  if (!db->Execute(kNewAggregatableReportMetadataTableSql))
    return false;

  // Transfer the existing aggregatable_report_metadata rows to the new table,
  // using the report_time value for initial_report_time.
  static constexpr char kPopulateNewAggregatableReportMetadataSql[] =
      "INSERT INTO new_aggregatable_report_metadata SELECT "
      "aggregation_id,source_id,trigger_time,debug_key,external_report_id,"
      "report_time,failed_send_attempts,report_time "
      "FROM aggregatable_report_metadata";
  if (!db->Execute(kPopulateNewAggregatableReportMetadataSql))
    return false;

  static constexpr char kDropOldAggregatableReportMetadataTableSql[] =
      "DROP TABLE aggregatable_report_metadata";
  if (!db->Execute(kDropOldAggregatableReportMetadataTableSql))
    return false;

  static constexpr char kRenameAggregatableReportMetadataTableSql[] =
      "ALTER TABLE new_aggregatable_report_metadata "
      "RENAME TO aggregatable_report_metadata";
  if (!db->Execute(kRenameAggregatableReportMetadataTableSql))
    return false;

  // Create the pre-existing aggregatable_report_metadata table indices on the
  // new table.

  static constexpr char kAggregateSourceIdIndexSql[] =
      "CREATE INDEX IF NOT EXISTS aggregate_source_id_idx "
      "ON aggregatable_report_metadata(source_id)";
  if (!db->Execute(kAggregateSourceIdIndexSql))
    return false;

  static constexpr char kAggregateTriggerTimeIndexSql[] =
      "CREATE INDEX IF NOT EXISTS aggregate_trigger_time_idx "
      "ON aggregatable_report_metadata(trigger_time)";
  if (!db->Execute(kAggregateTriggerTimeIndexSql))
    return false;

  static constexpr char kAggregateReportTimeIndexSql[] =
      "CREATE INDEX IF NOT EXISTS aggregate_report_time_idx "
      "ON aggregatable_report_metadata(report_time)";
  if (!db->Execute(kAggregateReportTimeIndexSql))
    return false;

  meta_table->SetVersionNumber(34);
  return transaction.Commit();
}

bool MigrateToVersion35(sql::Database* db, sql::MetaTable* meta_table) {
  // Wrap each migration in its own transaction. See comment in
  // `MigrateToVersion34`.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  static constexpr char kNewRateLimitsTableSql[] =
      "CREATE TABLE IF NOT EXISTS new_rate_limits"
      "(id INTEGER PRIMARY KEY NOT NULL,"
      "scope INTEGER NOT NULL,"
      "source_id INTEGER NOT NULL,"
      "source_site TEXT NOT NULL,"
      "source_origin TEXT NOT NULL,"
      "destination_site TEXT NOT NULL,"
      "destination_origin TEXT NOT NULL,"
      "reporting_origin TEXT NOT NULL,"
      "time INTEGER NOT NULL,"
      "expiry_time INTEGER NOT NULL)";
  if (!db->Execute(kNewRateLimitsTableSql))
    return false;

  // Transfer the existing rows to the new table, inserting `base::Time()`
  // as default values for the expiry_time column.
  static constexpr char kPopulateNewRateLimitsTableSql[] =
      "INSERT INTO new_rate_limits SELECT "
      "id,scope,source_id,source_site,source_origin,destination_site,"
      "destination_origin,reporting_origin,time,0 "
      "FROM rate_limits";
  if (!db->Execute(kPopulateNewRateLimitsTableSql))
    return false;

  static constexpr char kDropOldRateLimitsTableSql[] = "DROP TABLE rate_limits";
  if (!db->Execute(kDropOldRateLimitsTableSql))
    return false;

  static constexpr char kRenameRateLimitsTableSql[] =
      "ALTER TABLE new_rate_limits RENAME TO rate_limits";
  if (!db->Execute(kRenameRateLimitsTableSql))
    return false;

  // Update the expiry_time column for the existing source rows.
  static constexpr char kUpdateSourceExpiryTimeSql[] =
      "UPDATE rate_limits SET expiry_time=? WHERE id=?";
  sql::Statement update_source_expiry_time_statement(
      db->GetUniqueStatement(kUpdateSourceExpiryTimeSql));

  static_assert(static_cast<int>(RateLimitTable::Scope::kSource) == 0,
                "update `scope=0` clause below");

  static constexpr char kGetRateLimitsSql[] =
      "SELECT R.id,R.time,I.expiry_time "
      "FROM rate_limits R "
      "LEFT JOIN sources I ON R.source_id=I.source_id "
      "WHERE R.scope=0 "
      "ORDER BY R.id";
  sql::Statement get_rate_limits_statement(
      db->GetUniqueStatement(kGetRateLimitsSql));
  while (get_rate_limits_statement.Step()) {
    base::Time source_expiry_time;
    if (get_rate_limits_statement.GetColumnType(2) == sql::ColumnType::kNull) {
      // Use maximum expiry time if there's no matching source.
      source_expiry_time = get_rate_limits_statement.ColumnTime(1) +
                           kDefaultAttributionSourceExpiry;
    } else {
      source_expiry_time = get_rate_limits_statement.ColumnTime(2);
    }

    update_source_expiry_time_statement.Reset(/*clear_bound_vars=*/true);

    update_source_expiry_time_statement.BindTime(0, source_expiry_time);
    update_source_expiry_time_statement.BindInt64(
        1, get_rate_limits_statement.ColumnInt64(0));
    if (!update_source_expiry_time_statement.Run())
      return false;
  }

  if (!get_rate_limits_statement.Succeeded())
    return false;

  // Create the rate_limits table indices on the new table.
  static constexpr char kRateLimitSourceSiteReportingOriginIndexSql[] =
      "CREATE INDEX IF NOT EXISTS rate_limit_source_site_reporting_origin_idx "
      "ON rate_limits"
      "(scope,source_site,reporting_origin)";
  if (!db->Execute(kRateLimitSourceSiteReportingOriginIndexSql))
    return false;

  static constexpr char kRateLimitReportingOriginIndexSql[] =
      "CREATE INDEX IF NOT EXISTS rate_limit_reporting_origin_idx "
      "ON rate_limits(scope,destination_site,source_site)";
  if (!db->Execute(kRateLimitReportingOriginIndexSql))
    return false;

  static constexpr char kRateLimitTimeIndexSql[] =
      "CREATE INDEX IF NOT EXISTS rate_limit_time_idx ON rate_limits(time)";
  if (!db->Execute(kRateLimitTimeIndexSql))
    return false;

  static constexpr char kRateLimitImpressionIdIndexSql[] =
      "CREATE INDEX IF NOT EXISTS rate_limit_source_id_idx "
      "ON rate_limits(source_id)";
  if (!db->Execute(kRateLimitImpressionIdIndexSql))
    return false;

  meta_table->SetVersionNumber(35);
  return transaction.Commit();
}

}  // namespace

bool UpgradeAttributionStorageSqlSchema(sql::Database* db,
                                        sql::MetaTable* meta_table) {
  DCHECK(db);
  DCHECK(meta_table);

  base::ThreadTicks start_timestamp = base::ThreadTicks::Now();

  if (meta_table->GetVersionNumber() == 33) {
    if (!MigrateToVersion34(db, meta_table))
      return false;
  }
  if (meta_table->GetVersionNumber() == 34) {
    if (!MigrateToVersion35(db, meta_table))
      return false;
  }
  // Add similar if () blocks for new versions here.

  base::UmaHistogramMediumTimes("Conversions.Storage.MigrationTime",
                                base::ThreadTicks::Now() - start_timestamp);
  return true;
}

}  // namespace content

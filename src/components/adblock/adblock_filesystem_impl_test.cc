/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_filesystem_impl.h"

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_path_override.h"
#include "components/adblock/adblock_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockFileSystemImplTest : public testing::Test {
 public:
  AdblockFileSystemImplTest() {}
  ~AdblockFileSystemImplTest() override {}

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    files_ = {base::FilePath(FILE_PATH_LITERAL("patterns.ini")),
              base::FilePath(FILE_PATH_LITERAL("prefs.json"))};

    dummy_content_ = "Dummy content";

    ASSERT_TRUE(src_temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(dest_temp_dir_.CreateUniqueTempDir());

    // create files in src dir
    for (const auto& file : files_) {
      base::FilePath src_file = src_temp_dir_.GetPath().Append(file);
      ASSERT_TRUE(base::WriteFile(src_file, dummy_content_));
    }
  }

  AdblockFileSystemImplTest(const AdblockFileSystemImplTest&) = delete;
  AdblockFileSystemImplTest& operator=(const AdblockFileSystemImplTest&) =
      delete;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::vector<base::FilePath> files_;
  base::StringPiece dummy_content_;
  base::ScopedTempDir src_temp_dir_;
  base::ScopedTempDir dest_temp_dir_;
};

TEST_F(AdblockFileSystemImplTest, AttemptMigrationIfRequiredEnabled) {
  ASSERT_TRUE(pref_service_.GetBoolean(
      prefs::kAdblockFileSystemMigrationAttemptRequired));

  base::ScopedPathOverride path_override(base::DIR_HOME,
                                         src_temp_dir_.GetPath());
  base::MockOnceClosure reply_closure;
  EXPECT_CALL(reply_closure, Run());

  ASSERT_TRUE(AdblockFileSystemImpl::AttemptMigrationIfRequired(
      &pref_service_, dest_temp_dir_.GetPath(), reply_closure.Get()));

  task_environment_.RunUntilIdle();

  ASSERT_FALSE(pref_service_.GetBoolean(
      prefs::kAdblockFileSystemMigrationAttemptRequired));

  // check that files were moved
  for (const auto& file : files_) {
    base::FilePath src_file = src_temp_dir_.GetPath().Append(file);
    ASSERT_FALSE(base::PathExists(src_file));

    base::FilePath dest_file = dest_temp_dir_.GetPath().Append(file);
    ASSERT_TRUE(base::PathExists(dest_file));

    std::string content;
    ASSERT_TRUE(base::ReadFileToString(dest_file, &content));
    ASSERT_EQ(dummy_content_, content);
  }
}

TEST_F(AdblockFileSystemImplTest, AttemptMigrationIfRequiredDisabled) {
  pref_service_.SetBoolean(prefs::kAdblockFileSystemMigrationAttemptRequired,
                           false);

  base::ScopedPathOverride path_override(base::DIR_HOME,
                                         src_temp_dir_.GetPath());
  base::MockOnceClosure reply_closure;
  EXPECT_CALL(reply_closure, Run());

  ASSERT_FALSE(AdblockFileSystemImpl::AttemptMigrationIfRequired(
      &pref_service_, dest_temp_dir_.GetPath(), reply_closure.Get()));

  task_environment_.RunUntilIdle();

  // check that files are not moved
  for (const auto& file : files_) {
    base::FilePath src_file = src_temp_dir_.GetPath().Append(file);
    ASSERT_TRUE(base::PathExists(src_file));

    base::FilePath dest_file = dest_temp_dir_.GetPath().Append(file);
    ASSERT_FALSE(base::PathExists(dest_file));
  }
}

}  // namespace adblock

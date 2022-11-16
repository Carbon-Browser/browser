// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/updater/unittest_util_win.h"

#include <windows.h>

#include <string>

#include "base/base_paths_win.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/win/registry.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/win/win_constants.h"
#include "chrome/updater/win/win_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

namespace {

void CreateAppCommandRegistryHelper(UpdaterScope scope,
                                    const std::wstring& app_id,
                                    const std::wstring& cmd_id,
                                    const std::wstring& cmd_line,
                                    bool auto_run_on_os_upgrade) {
  CreateAppClientKey(scope, app_id);
  base::win::RegKey command_key;
  EXPECT_EQ(command_key.Create(UpdaterScopeToHKeyRoot(scope),
                               GetAppCommandKeyName(app_id, cmd_id).c_str(),
                               Wow6432(KEY_WRITE)),
            ERROR_SUCCESS);
  EXPECT_EQ(command_key.WriteValue(kRegValueCommandLine, cmd_line.c_str()),
            ERROR_SUCCESS);
  if (auto_run_on_os_upgrade) {
    EXPECT_EQ(command_key.WriteValue(kRegValueAutoRunOnOSUpgrade, 1U),
              ERROR_SUCCESS);
  }
}

}  // namespace

std::wstring GetClientKeyName(const std::wstring& app_id) {
  return base::StrCat({CLIENTS_KEY, app_id});
}

std::wstring GetAppCommandKeyName(const std::wstring& app_id,
                                  const std::wstring& command_id) {
  return base::StrCat(
      {GetClientKeyName(app_id), L"\\", kRegKeyCommands, L"\\", command_id});
}

void CreateAppClientKey(UpdaterScope scope, const std::wstring& app_id) {
  base::win::RegKey client_key;
  EXPECT_EQ(
      client_key.Create(UpdaterScopeToHKeyRoot(scope),
                        GetClientKeyName(app_id).c_str(), Wow6432(KEY_WRITE)),
      ERROR_SUCCESS);
}

void DeleteAppClientKey(UpdaterScope scope, const std::wstring& app_id) {
  base::win::RegKey(UpdaterScopeToHKeyRoot(scope), L"", Wow6432(DELETE))
      .DeleteKey(GetClientKeyName(app_id).c_str());
}

void CreateAppCommandRegistry(UpdaterScope scope,
                              const std::wstring& app_id,
                              const std::wstring& cmd_id,
                              const std::wstring& cmd_line) {
  CreateAppCommandRegistryHelper(scope, app_id, cmd_id, cmd_line, false);
}

void CreateAppCommandOSUpgradeRegistry(UpdaterScope scope,
                                       const std::wstring& app_id,
                                       const std::wstring& cmd_id,
                                       const std::wstring& cmd_line) {
  CreateAppCommandRegistryHelper(scope, app_id, cmd_id, cmd_line, true);
}

void SetupCmdExe(UpdaterScope scope,
                 base::CommandLine& cmd_exe_command_line,
                 base::ScopedTempDir& temp_programfiles_dir) {
  constexpr wchar_t kCmdExe[] = L"cmd.exe";

  base::FilePath system_path;
  ASSERT_TRUE(base::PathService::Get(base::DIR_SYSTEM, &system_path));

  const base::FilePath cmd_exe_system_path = system_path.Append(kCmdExe);
  if (scope == UpdaterScope::kUser) {
    cmd_exe_command_line = base::CommandLine(cmd_exe_system_path);
    return;
  }

  base::FilePath programfiles_path;
  ASSERT_TRUE(
      base::PathService::Get(base::DIR_PROGRAM_FILES, &programfiles_path));
  ASSERT_TRUE(
      temp_programfiles_dir.CreateUniqueTempDirUnderPath(programfiles_path));
  base::FilePath cmd_exe_path;
  cmd_exe_path = temp_programfiles_dir.GetPath().Append(kCmdExe);

  ASSERT_TRUE(base::CopyFile(cmd_exe_system_path, cmd_exe_path));
  cmd_exe_command_line = base::CommandLine(cmd_exe_path);
}

}  // namespace updater

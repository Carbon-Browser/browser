// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UPDATER_UNITTEST_UTIL_WIN_H_
#define CHROME_UPDATER_UNITTEST_UTIL_WIN_H_

#include <string>

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "chrome/updater/updater_scope.h"

namespace updater {

// Returns the registry path `Software\{CompanyName}\Update\Clients\{app_id}`.
std::wstring GetClientKeyName(const std::wstring& app_id);

// Returns the registry path
// `Software\{CompanyName}\Update\Clients\{app_id}\Commands\{command_id}`.
std::wstring GetAppCommandKeyName(const std::wstring& app_id,
                                  const std::wstring& command_id);

// Creates the key `{HKLM\HKCU}\Software\{CompanyName}\Update\Clients\{app_id}`.
// `{HKLM\HKCU}` is determined by `scope`.
void CreateAppClientKey(UpdaterScope scope, const std::wstring& app_id);

// Deletes the key `{HKLM\HKCU}\Software\{CompanyName}\Update\Clients\{app_id}`.
// `{HKLM\HKCU}` is determined by `scope`.
void DeleteAppClientKey(UpdaterScope scope, const std::wstring& app_id);

// Creates the key
// `{HKRoot}\Software\{CompanyName}\Update\Clients\{app_id}\Commands\{cmd_id}`,
// and adds a `CommandLine` REG_SZ entry with the value `cmd_line`. `{HKRoot}`
// is determined by `scope`.
void CreateAppCommandRegistry(UpdaterScope scope,
                              const std::wstring& app_id,
                              const std::wstring& cmd_id,
                              const std::wstring& cmd_line);

// Similar to `CreateAppCommandRegistry`, and then marks the AppCommand to run
// on OS upgrades.
void CreateAppCommandOSUpgradeRegistry(UpdaterScope scope,
                                       const std::wstring& app_id,
                                       const std::wstring& cmd_id,
                                       const std::wstring& cmd_line);

// Returns the path to "cmd.exe" in `cmd_exe_command_line` based on the current
// test scope:
// * "%systemroot%\system32\cmd.exe" for user `scope`.
// * "%programfiles%\`temp_parent_dir`\cmd.exe" for system `scope`.
// `temp_parent_dir` is owned by the caller.
void SetupCmdExe(UpdaterScope scope,
                 base::CommandLine& cmd_exe_command_line,
                 base::ScopedTempDir& temp_parent_dir);

}  // namespace updater

#endif  // CHROME_UPDATER_UNITTEST_UTIL_WIN_H_

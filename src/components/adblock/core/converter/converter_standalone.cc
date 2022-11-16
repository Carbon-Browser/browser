/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "components/adblock/core/converter/converter.h"

#if BUILDFLAG(IS_WIN)
#include "base/strings/sys_string_conversions.h"
#endif  // BUILDFLAG(IS_WIN)

namespace {

bool Convert(base::FilePath input_path, GURL url, base::FilePath output_path) {
  if (!url.is_valid()) {
    LOG(ERROR) << "[eyeo] Filter list URL not valid: " << url;
    return false;
  }
  std::string content;
  if (!base::ReadFileToString(input_path, &content)) {
    LOG(ERROR) << "[eyeo] Could not open input file " << input_path;
    return false;
  }
  std::stringstream input(content);
  auto converter_result = adblock::Converter().Convert(input, {url, true});

  if (!base::WriteFile(
          output_path,
          reinterpret_cast<const char*>(converter_result.data->data()),
          converter_result.data->size())) {
    LOG(ERROR) << "[eyeo] Could not write output file " << output_path;
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  auto* command_line = base::CommandLine::ForCurrentProcess();

  const auto positional_arguments = command_line->GetArgs();
  if (positional_arguments.size() != 3u) {
    LOG(ERROR) << "Usage: " << command_line->GetProgram()
               << " [INPUT_FILE] [FILTER_LIST_URL] [OUTPUT_FILE]";
    return 1;
  }

  // We need to make the path absolute because base::ReadFileToString() fails
  // for paths with `..` components.
  const auto input_path =
      base::MakeAbsoluteFilePath(base::FilePath(positional_arguments[0]));

#if BUILDFLAG(IS_WIN)
  const auto url = GURL(base::SysWideToUTF8(positional_arguments[1]));
#else
  const auto url = GURL(positional_arguments[1]);
#endif
  const auto output_path = base::FilePath(positional_arguments[2]);

  if (!Convert(input_path, url, output_path)) {
    return 1;
  }
  return 0;
}

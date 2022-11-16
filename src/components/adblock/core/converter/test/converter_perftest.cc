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

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/converter/converter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class ConverterPerfTest : public testing::Test {
 public:
  void MeasureConversionTime(std::string filename) {
    base::FilePath source_file;
    ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &source_file));
    source_file = source_file.AppendASCII("components")
                      .AppendASCII("test")
                      .AppendASCII("data")
                      .AppendASCII("adblock")
                      .AppendASCII(filename);
    std::string content;
    ASSERT_TRUE(base::ReadFileToString(source_file, &content));
    std::stringstream input(std::move(content));
    base::ElapsedTimer timer;
    auto buffer = Converter().Convert(input, {CustomFiltersUrl()});
    LOG(INFO) << "[eyeo] Time to convert " << filename << ": "
              << timer.Elapsed();
  }
};

TEST_F(ConverterPerfTest, ConvertEasylistTime) {
  MeasureConversionTime("easylist.txt");
}

TEST_F(ConverterPerfTest, ConvertExceptionrulesTime) {
  MeasureConversionTime("exceptionrules.txt");
}

}  // namespace adblock

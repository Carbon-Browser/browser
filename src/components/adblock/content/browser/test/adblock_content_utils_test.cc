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

#include "components/adblock/content/browser/adblock_content_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace utils {

void DetectResourceType(const std::string& prefix, const std::string postfix) {
  EXPECT_EQ(ContentType::Other, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + "." + postfix), -1));
  EXPECT_EQ(ContentType::Other, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".xxx" + postfix), -1));
  EXPECT_EQ(ContentType::Script, utils::ConvertToAdblockResourceType(
                                     GURL(prefix + ".gif.js" + postfix), -1));
  EXPECT_EQ(ContentType::Script, utils::ConvertToAdblockResourceType(
                                     GURL(prefix + ".js" + postfix), -1));
  EXPECT_EQ(ContentType::Subdocument, utils::ConvertToAdblockResourceType(
                                          GURL(prefix + ".htm" + postfix), -1));
  EXPECT_EQ(ContentType::Subdocument,
            utils::ConvertToAdblockResourceType(
                GURL(prefix + ".html" + postfix), -1));
  EXPECT_EQ(ContentType::Stylesheet, utils::ConvertToAdblockResourceType(
                                         GURL(prefix + ".css" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".png" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".jpg" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".jpeg" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".gif" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".GIF" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".bmp" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".ico" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".webp" + postfix), -1));
  EXPECT_EQ(ContentType::Image, utils::ConvertToAdblockResourceType(
                                    GURL(prefix + ".apng" + postfix), -1));
  EXPECT_EQ(ContentType::Font, utils::ConvertToAdblockResourceType(
                                   GURL(prefix + ".ttf" + postfix), -1));
  EXPECT_EQ(ContentType::Font, utils::ConvertToAdblockResourceType(
                                   GURL(prefix + ".woff" + postfix), -1));
}

TEST(AdblockUtils, DetectResourceTypeGeneric) {
  EXPECT_EQ(ContentType::Other,
            utils::ConvertToAdblockResourceType(GURL(""), -1));
  // After DPD-86 we no longer need to detect Web Socket type because
  // it takes different code path and is given explicitly. This is why
  // detection threats it as any other URL and looks at the file and detects
  // type of that.
  EXPECT_EQ(ContentType::Script, utils::ConvertToAdblockResourceType(
                                     GURL("wss://test.com/file.js"), -1));

  DetectResourceType("https://test.com/file", "");
}

TEST(AdblockUtils, DetectResourceTypeDomain) {
  DetectResourceType("https://test.js/path/file", "");
}

TEST(AdblockUtils, DetectResourceTypeFolder) {
  DetectResourceType("https://test.com/path.css/file", "");
}

TEST(AdblockUtils, DetectResourceTypeArgument) {
  DetectResourceType("https://test.com/file", "?arg1=file.htm");
  DetectResourceType("https://test.com/file", "?arg1=file.jpeg&arg2=file.xxx");
}

TEST(AdblockUtils, DetectResourceTypeFragment) {
  DetectResourceType("https://test.com/file", "#fragment.ttf");
}

TEST(AdblockUtils, DetectResourceTypeFilename) {
  DetectResourceType("https://test.com/file.js", "#fragment.ttf");
}

TEST(AdblockUtils, DetectResourceTypeArgumentAndFragment) {
  DetectResourceType("https://test.com/file", "?arg1=file.htm#fragment.ttf");
  DetectResourceType("https://test.com/file",
                     "?arg1=file.jpeg&arg2=file.xxx#fragment.ttf");
}

TEST(AdblockUtils, DetectResourceTypeUserNamePasswordArgumentAndFragment) {
  DetectResourceType("https://username:password@test.com/file",
                     "?arg1=file.htm#fragment.ttf");
  DetectResourceType("https://username:password@test.com/file",
                     "?arg1=file.jpeg&arg2=file.xxx#fragment.ttf");
}

}  // namespace utils
}  // namespace adblock

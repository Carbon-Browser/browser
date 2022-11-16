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

#include "components/adblock/core/subscription/url_keyword_extractor.h"

#include <algorithm>
#include <cctype>

#include "base/strings/string_util.h"
#include "components/adblock/core/common/keyword_extractor_utils.h"

namespace adblock {
namespace {

bool IsKeywordCharacter(char c) {
  return std::isalnum(c) || c == '%';
}

}  // namespace

absl::optional<std::string> UrlKeywordExtractor::GetNextKeyword() {
  std::string current_keyword;
  do {
    current_keyword.clear();
    if (end_of_last_keyword_ == input_.end())
      return absl::nullopt;
    const auto* start_of_next_keyword =
        std::find_if(end_of_last_keyword_, input_.end(), &IsKeywordCharacter);
    if (start_of_next_keyword == input_.end())
      return absl::nullopt;
    const auto* end_of_next_keyword = std::find_if_not(
        start_of_next_keyword, input_.end(), &IsKeywordCharacter);
    for (const auto* i = start_of_next_keyword; i < end_of_next_keyword; i++) {
      current_keyword.push_back(base::ToLowerASCII(*i));
    }
    end_of_last_keyword_ = end_of_next_keyword;
  } while (utils::IsBadKeyword(current_keyword));
  return current_keyword;
}

UrlKeywordExtractor::UrlKeywordExtractor(base::StringPiece url)
    : input_(url.data(), url.size()), end_of_last_keyword_(input_.begin()) {}
UrlKeywordExtractor::~UrlKeywordExtractor() = default;

}  // namespace adblock

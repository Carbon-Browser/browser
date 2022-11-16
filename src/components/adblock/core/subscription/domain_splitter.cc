
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

#include "components/adblock/core/subscription/domain_splitter.h"

#include "base/strings/string_util.h"

namespace adblock {

DomainSplitter::DomainSplitter(base::StringPiece domain)
    : domain_(base::TrimString(domain, ".", base::TRIM_ALL)) {}

absl::optional<base::StringPiece> DomainSplitter::FindNextSubdomain() {
  const auto old_dot_pos = dot_pos_;
  if (dot_pos_ < domain_.size()) {
    // Find next dot in domain, for future iteration to consume.
    dot_pos_ = domain_.find('.', dot_pos_ + 1);
    // First run is special because we found no dots yet.
    if (old_dot_pos == 0)
      return domain_;
    // Return the part of domain *after* the dot we found in previous iteration.
    return domain_.substr(old_dot_pos + 1);
  }
  // Reached end of domain_.
  // Reset in anticipation of future calls to FindNextSubdomain().
  dot_pos_ = 0;
  return absl::nullopt;
}

}  // namespace adblock

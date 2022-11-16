
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

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_

#include "absl/types/optional.h"

#include "base/strings/string_piece.h"

namespace adblock {

// When constructed with a full domain like "aaa.bbb.ccc.com", subsequent calls
// to FindNextSubdomain() will yield "aaa.bbb.ccc.com", "bbb.ccc.com",
// "ccc.com", "com" and then nullopt.
class DomainSplitter {
 public:
  // |domain| must outlive this, no copy made.
  explicit DomainSplitter(base::StringPiece domain);
  // Returns reference to part of |domain|.
  absl::optional<base::StringPiece> FindNextSubdomain();

 private:
  const base::StringPiece domain_;
  size_t dot_pos_ = 0;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_DOMAIN_SPLITTER_H_

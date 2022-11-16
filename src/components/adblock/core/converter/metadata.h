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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_METADATA_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_METADATA_H_

#include <istream>

#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace adblock {

class Metadata {
 public:
  static absl::optional<Metadata> Parse(std::istream& stream);
  static Metadata Default();

  Metadata(const Metadata& other);
  Metadata(Metadata&& other);
  Metadata& operator=(const Metadata& other);
  Metadata& operator=(Metadata&& other);
  ~Metadata();

  const std::string& GetHomepage() const;
  const absl::optional<GURL>& GetRedirectUrl() const;

  const std::string& GetTitle() const;
  const std::string& GetVersion() const;

  const base::TimeDelta& GetExpires() const;

  static constexpr base::TimeDelta kDefaultExpirationInterval = base::Days(5);
  static constexpr base::TimeDelta kMaxExpirationInterval = base::Days(14);
  static constexpr base::TimeDelta kMinExpirationInterval = base::Hours(1);

 private:
  Metadata();

  bool ParseComment(const std::string& comment);
  void ParseExpirationTime(const std::string& expiration_value);

  std::string homepage_;
  absl::optional<GURL> redirect_url_;
  std::string title_;
  std::string version_;
  base::TimeDelta expires_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_METADATA_H_

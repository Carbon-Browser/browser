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

#include "components/adblock/core/converter/metadata.h"

#include "base/cxx17_backports.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

bool ParseAdblockHeader(std::istream& stream) {
  static re2::RE2 adblock_header_re("^\\[Adblock.*\\]");
  std::string adblock_header;

  if (!std::getline(stream, adblock_header)) {
    return false;
  }

  base::TrimWhitespaceASCII(adblock_header, base::TRIM_ALL, &adblock_header);

  if (!re2::RE2::FullMatch(
          re2::StringPiece(adblock_header.data(), adblock_header.size()),
          adblock_header_re)) {
    DLOG(ERROR)
        << "[eyeo] Invalid filter list. Should start with [Adblock Plus "
           "<x>.<y>]. Won't parse.";
    return false;
  }
  return true;
}

}  // namespace

namespace adblock {

// static
absl::optional<Metadata> Metadata::Parse(std::istream& stream) {
  Metadata metadata;
  std::string line;

  if (!ParseAdblockHeader(stream)) {
    return {};
  }

  auto position_in_stream = stream.tellg();
  while (std::getline(stream, line)) {
    base::TrimWhitespaceASCII(line, base::TRIM_ALL, &line);
    if (!metadata.ParseComment(line)) {
      // NOTE: Rewind stream after last header line
      stream.seekg(position_in_stream, std::ios_base::beg);
      break;
    }
    position_in_stream = stream.tellg();
  }
  return metadata;
}

// static
Metadata Metadata::Default() {
  Metadata metadata;
  return metadata;
}

Metadata::Metadata() : expires_(kDefaultExpirationInterval) {}
Metadata::Metadata(const Metadata& other) = default;
Metadata::Metadata(Metadata&& other) = default;
Metadata& Metadata::operator=(const Metadata& other) = default;
Metadata& Metadata::operator=(Metadata&& other) = default;
Metadata::~Metadata() = default;

const std::string& Metadata::GetHomepage() const {
  return homepage_;
}

const absl::optional<GURL>& Metadata::GetRedirectUrl() const {
  return redirect_url_;
}

const std::string& Metadata::GetTitle() const {
  return title_;
}

const std::string& Metadata::GetVersion() const {
  return version_;
}

const base::TimeDelta& Metadata::GetExpires() const {
  return expires_;
}

bool Metadata::ParseComment(const std::string& comment) {
  static re2::RE2 comment_re("^!\\s*(.*?)\\s*:\\s*(.*)");
  std::string key, value;

  if (!re2::RE2::FullMatch(re2::StringPiece(comment.data(), comment.size()),
                           comment_re, &key, &value))
    return false;

  key = base::ToLowerASCII(key);
  if (key == "homepage") {
    homepage_ = value;
  } else if (key == "redirect") {
    auto redirect_url = GURL(value);
    if (redirect_url.is_valid()) {
      redirect_url_ = redirect_url;
    } else {
      DLOG(WARNING) << "[eyeo] Invalid redirect URL: " << value
                    << ". Will not redirect.";
    }
  } else if (key == "title")
    title_ = value;
  else if (key == "version")
    version_ = value;
  else if (key == "expires")
    ParseExpirationTime(value);

  return true;
}

// NOTE: This is done by the logic described here:
// https://eyeo.gitlab.io/adblockplus/abc/core-spec/#appendix-filter-list-syntax
void Metadata::ParseExpirationTime(const std::string& expiration_value) {
  static re2::RE2 expiration_time_re("\\s*([0-9]+)\\s*(h)?.*");
  std::string expiration_unit;
  uint64_t expiration_time;

  if (!re2::RE2::FullMatch(
          re2::StringPiece(expiration_value.data(), expiration_value.size()),
          expiration_time_re, &expiration_time, &expiration_unit)) {
    DLOG(INFO) << "[eyeo] Invalid expiration time format: " << expiration_value
               << ". Will use default value of "
               << kDefaultExpirationInterval.InDays() << " days.";
    return;
  }
  expires_ = base::clamp(expiration_unit == "h" ? base::Hours(expiration_time)
                                                : base::Days(expiration_time),
                         kMinExpirationInterval, kMaxExpirationInterval);
}

}  // namespace adblock

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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_CONVERTER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_CONVERTER_H_

#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_piece_forward.h"
#include "components/adblock/core/common/converter_result.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/converter/snippet_tokenizer.h"
#include "url/gurl.h"

namespace adblock {

struct ConverterData;
class Metadata;
struct UrlFilterData;

struct ConverterConfig {
  GURL url_{};
  bool allow_privileged_ = false;
};

/**
 * Convert text ABP subscription data from stream to the flat buffer
 */
class Converter {
 public:
  Converter();
  explicit Converter(
      std::unique_ptr<SnippetTokenizer> converter_snippet_tokenizer_);

  ~Converter();

  ConverterResult Convert(std::istream& stream, ConverterConfig config) const;
  std::unique_ptr<FlatbufferData> Convert(
      const std::vector<std::string>& filters,
      ConverterConfig config) const;

 private:
  enum class ContentFilterType {
    ElemHide,
    ElemHideException,
    ElemHideEmulation,
    Snippet,
    Invalid
  };
  ContentFilterType FromContentFilterType(const char value) const;
  void ParseLine(base::StringPiece line, ConverterData& data) const;
  std::unique_ptr<FlatbufferData> Pack(ConverterData& data,
                                       const Metadata& metadata) const;
  void ParseContentFilter(base::StringPiece domains_source,
                          base::StringPiece selector,
                          ContentFilterType type,
                          ConverterData& data,
                          base::StringPiece full_filter_line) const;
  void ParseUrlFilter(base::StringPiece original_pattern,
                      ConverterData& data) const;
  bool ValidateUrlFilter(base::StringPiece original_pattern,
                         const UrlFilterData& filter,
                         const ConverterData& data) const;
  void AddUrlFilterToIndex(base::StringPiece keyword_pattern,
                           UrlFilterData& filter,
                           ConverterData& data) const;
  bool ParseOption(base::StringPiece value,
                   UrlFilterData& filter,
                   ConverterData& data) const;
  bool ParseRewrite(base::StringPiece value, UrlFilterData& filter) const;
  bool ParseDomains(base::StringPiece value,
                    UrlFilterData& filter,
                    ConverterData& data) const;
  bool ParseSitekeys(base::StringPiece value,
                     UrlFilterData& filter,
                     ConverterData& data) const;
  bool ParseCsp(base::StringPiece value, UrlFilterData& filter) const;
  bool ParseHeaderFilter(base::StringPiece value, UrlFilterData& filter) const;
  bool ParseFilterType(base::StringPiece option, UrlFilterData& filter) const;
  bool ParseContentType(base::StringPiece option,
                        bool inverse,
                        UrlFilterData& filter) const;
  void FilterToRegexp(std::string& value) const;
  void ParseSnippetFilter(const base::span<std::string>& include_domains,
                          const base::span<std::string>& exclude_domains,
                          base::StringPiece selector,
                          ConverterData& data) const;
  std::unique_ptr<SnippetTokenizer> converter_snippet_tokenizer_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_CONVERTER_H_

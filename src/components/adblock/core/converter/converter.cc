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

#include "components/adblock/core/converter/converter.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "absl/types/optional.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/converter/filter_keyword_extractor.h"
#include "components/adblock/core/converter/metadata.h"
#include "components/adblock/core/converter/snippet_tokenizer_impl.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "third_party/icu/source/i18n/unicode/regex.h"
#include "third_party/re2/src/re2/re2.h"

namespace adblock {

bool IsValidRegex(const base::StringPiece& pattern) {
  re2::RE2::Options options;
  options.set_never_capture(true);
  options.set_log_errors(false);
  const re2::RE2 re2_pattern(pattern.data(), options);
  if (re2_pattern.ok())
    return true;
  const icu::UnicodeString icu_pattern(pattern.data(), pattern.length());
  UErrorCode status = U_ZERO_ERROR;
  const icu::RegexMatcher matcher(icu_pattern, 0u, status);
  if (U_FAILURE(status))
    return false;
  return true;
}

bool IsExcludedDomain(base::StringPiece domain) {
  return !domain.empty() && domain.front() == '~';
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>>
CreateVectorOfSharedStrings(const base::span<std::string>& strings,
                            flatbuffers::FlatBufferBuilder& builder) {
  std::vector<flatbuffers::Offset<flatbuffers::String>> shared_strings;
  std::transform(
      strings.begin(), strings.end(), std::back_inserter(shared_strings),
      [&](const std::string& s) { return builder.CreateSharedString(s); });
  return builder.CreateVector(std::move(shared_strings));
}

enum class UrlFilterType {
  Subresource,
  Popup,
  Csp,
  Header,
  Document,
  Genericblock,
  Elemhide,
  Generichide,
  Rewrite,
  Unknown,
};

UrlFilterType UrlFilterTypeFromString(base::StringPiece value) {
  if (value == "popup")
    return UrlFilterType::Popup;
  else if (value == "csp")
    return UrlFilterType::Csp;
  else if (value == "header")
    return UrlFilterType::Header;
  else if (value == "document")
    return UrlFilterType::Document;
  else if (value == "genericblock")
    return UrlFilterType::Genericblock;
  else if (value == "elemhide")
    return UrlFilterType::Elemhide;
  else if (value == "generichide")
    return UrlFilterType::Generichide;
  return UrlFilterType::Unknown;
}

bool IsRegexFilter(const base::StringPiece& pattern) {
  if (pattern.size() > 2 && pattern.front() == '/' && pattern.back() == '/')
    return true;
  return false;
}

ContentType ContentTypeFromString(base::StringPiece value) {
  if (value == "other" || value == "xbl" || value == "dtd")
    return ContentType::Other;
  else if (value == "script")
    return ContentType::Script;
  else if (value == "image" || value == "background")
    return ContentType::Image;
  else if (value == "stylesheet")
    return ContentType::Stylesheet;
  else if (value == "object")
    return ContentType::Object;
  else if (value == "subdocument")
    return ContentType::Subdocument;
  else if (value == "websocket")
    return ContentType::Websocket;
  else if (value == "webrtc")
    return ContentType::Webrtc;
  else if (value == "ping")
    return ContentType::Ping;
  else if (value == "xmlhttprequest")
    return ContentType::Xmlhttprequest;
  else if (value == "media")
    return ContentType::Media;
  else if (value == "font")
    return ContentType::Font;
  else
    return ContentType::Unknown;
}

std::string EscapeSelector(const base::StringPiece& value) {
  std::string escaped;
  base::ReplaceChars(value, "{", "\\7b ", &escaped);
  base::ReplaceChars(escaped, "}", "\\7d ", &escaped);
  return escaped;
}

class Buffer : public FlatbufferData {
 public:
  explicit Buffer(flatbuffers::DetachedBuffer&& buffer)
      : buffer_(std::move(buffer)) {}

  const uint8_t* data() const override { return buffer_.data(); }

  size_t size() const override { return buffer_.size(); }

 private:
  flatbuffers::DetachedBuffer buffer_;
};

struct UrlFilterData {
  std::string pattern;
  absl::optional<std::string> csp_filter;
  absl::optional<std::string> header_filter;
  bool is_allowing = false;
  bool match_case = false;
  bool is_regex = false;
  uint32_t content_type = 0;
  // A single filter line can define multiple filter types, ex. "$popup,image",
  // would mean UrlFilterType::Popup and UrlFilterType::Subresource with
  // ContentType::Image.
  std::unordered_set<UrlFilterType> filter_types;
  flat::ThirdParty third_party = flat::ThirdParty_Ignore;
  absl::optional<flat::AbpResource> rewrite;
  std::vector<flatbuffers::Offset<flatbuffers::String>> sitekeys;
  std::vector<flatbuffers::Offset<flatbuffers::String>> include_domains;
  std::vector<flatbuffers::Offset<flatbuffers::String>> exclude_domains;
};

struct ConverterData {
  using UrlFilterIndex =
      std::map<std::string, std::vector<flatbuffers::Offset<flat::UrlFilter>>>;
  using ElemhideIndex = std::unordered_map<
      std::string,
      std::vector<flatbuffers::Offset<flat::ElemHideFilter>>>;
  using SnippetIndex =
      std::map<std::string,
               std::vector<flatbuffers::Offset<flat::SnippetFilter>>>;

  std::string FindCandidateKeyword(UrlFilterIndex& index,
                                   base::StringPiece pattern_text);
  void AddUrlFilterToIndex(UrlFilterIndex& index,
                           absl::optional<base::StringPiece> pattern_text,
                           flatbuffers::Offset<flat::UrlFilter> filter);

  void AddElemhideFilterForDomains(
      ElemhideIndex& index,
      base::span<std::string> include_domains,
      flatbuffers::Offset<flat::ElemHideFilter> filter) const;

  void AddSnippetFilterForDomains(
      SnippetIndex& index,
      base::span<std::string> domains,
      flatbuffers::Offset<flat::SnippetFilter> filter) const;

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>>
  WriteUrlFilterIndex(const UrlFilterIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>>>
  WriteElemhideFilterIndex(const ElemhideIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>>>
  WriteSnippetFilterIndex(const SnippetIndex& index);

  flatbuffers::FlatBufferBuilder builder_;
  flatbuffers::Offset<flat::SubscriptionMetadata> metadata_;
  UrlFilterIndex url_subresource_block_;
  UrlFilterIndex url_subresource_allow_;
  UrlFilterIndex url_popup_block_;
  UrlFilterIndex url_popup_allow_;
  UrlFilterIndex url_document_allow_;
  UrlFilterIndex url_elemhide_allow_;
  UrlFilterIndex url_generichide_allow_;
  UrlFilterIndex url_genericblock_allow_;
  UrlFilterIndex url_csp_block_;
  UrlFilterIndex url_csp_allow_;
  UrlFilterIndex url_rewrite_block_;
  UrlFilterIndex url_rewrite_allow_;
  UrlFilterIndex url_header_allow_;
  UrlFilterIndex url_header_block_;
  ElemhideIndex elemhide_exception_index_;
  ElemhideIndex elemhide_index_;
  ElemhideIndex elemhide_emulation_index_;
  SnippetIndex snippet_index_;
  ConverterConfig config_;
};

std::string ConverterData::FindCandidateKeyword(UrlFilterIndex& index,
                                                base::StringPiece value) {
  FilterKeywordExtractor keyword_extractor(value);
  size_t last_size = std::numeric_limits<size_t>::max();
  std::string keyword;
  while (auto current_keyword = keyword_extractor.GetNextKeyword()) {
    std::string candidate = *current_keyword;
    auto it = index.find(candidate);
    auto size = it != index.end() ? it->second.size() : 0;

    if (size < last_size ||
        (size == last_size && candidate.size() > keyword.size())) {
      last_size = size;
      keyword = candidate;
    }
  }
  return keyword;
}

void ConverterData::AddUrlFilterToIndex(
    UrlFilterIndex& index,
    absl::optional<base::StringPiece> pattern_text,
    flatbuffers::Offset<flat::UrlFilter> filter) {
  const auto keyword =
      pattern_text ? FindCandidateKeyword(index, *pattern_text) : "";
  index[keyword].push_back(filter);
}

void ConverterData::AddElemhideFilterForDomains(
    ElemhideIndex& index,
    base::span<std::string> include_domains,
    flatbuffers::Offset<flat::ElemHideFilter> filter) const {
  if (include_domains.empty()) {
    // This is a generic filter, we add those under "" index.
    index[""].push_back(filter);
  } else {
    // Index this filter under each domain it is included for.
    for (const auto& domain : include_domains)
      index[domain].push_back(filter);
  }
}

void ConverterData::AddSnippetFilterForDomains(
    SnippetIndex& index,
    base::span<std::string> domains,
    flatbuffers::Offset<flat::SnippetFilter> filter) const {
  for (const auto& domain : domains)
    index[domain].push_back(filter);
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>>
ConverterData::WriteUrlFilterIndex(const UrlFilterIndex& index) {
  std::vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateUrlFiltersByKeyword(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }

  return builder_.CreateVector(offsets);
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>>>
ConverterData::WriteElemhideFilterIndex(const ElemhideIndex& index) {
  std::vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateElemHideFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }
  // Filters must be sorted (by domain), in order for LookupByKey() to work
  // correctly. This can be also achieved by making ElemhideIndex an ordered
  // map, but profiling shows sorting an unordered_map at the end is faster by
  // about 15% (on exceptionrules.txt).
  return builder_.CreateVectorOfSortedTables(offsets.data(), offsets.size());
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>>>
ConverterData::WriteSnippetFilterIndex(const SnippetIndex& index) {
  std::vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateSnippetFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }

  return builder_.CreateVector(offsets);
}

Converter::Converter()
    : converter_snippet_tokenizer_(std::make_unique<SnippetTokenizerImpl>()) {}
Converter::Converter(
    std::unique_ptr<SnippetTokenizer> converter_snippet_tokenizer)
    : converter_snippet_tokenizer_(std::move(converter_snippet_tokenizer)) {}

Converter::~Converter() = default;

ConverterResult Converter::Convert(std::istream& stream,
                                   ConverterConfig config) const {
  ConverterData data;
  data.config_ = std::move(config);

  ConverterResult result;
  auto metadata = Metadata::Parse(stream);
  if (!metadata.has_value()) {
    result.status = ConverterResult::Error;
    return result;
  }

  if (metadata->GetRedirectUrl().has_value()) {
    result.status = ConverterResult::Redirect;
    result.redirect_url = metadata->GetRedirectUrl().value();
    return result;
  }

  std::string line;
  while (std::getline(stream, line)) {
    base::TrimWhitespaceASCII(line, base::TRIM_ALL, &line);

    if (!line.empty())
      ParseLine(line, data);
  }

  result.data = Pack(data, metadata.value());
  return result;
}

std::unique_ptr<FlatbufferData> Converter::Convert(
    const std::vector<std::string>& filters,
    ConverterConfig config) const {
  ConverterData data;
  data.config_ = std::move(config);

  std::string line;
  for (const auto& cur : filters) {
    base::TrimWhitespaceASCII(cur, base::TRIM_ALL, &line);

    if (!line.empty())
      ParseLine(line, data);
  }

  return Pack(data, Metadata::Default());
}

void Converter::ParseLine(base::StringPiece line, ConverterData& data) const {
  if (base::StartsWith(line, "!"))
    return;

  auto pos = line.find('#');
  if (pos != std::string::npos && line.size() > pos + 2) {
    ContentFilterType type = FromContentFilterType(line[pos + 1]);

    if (type != ContentFilterType::Invalid &&
        (type == ContentFilterType::ElemHide || line[pos + 2] == '#')) {
      const std::string domains(base::ToLowerASCII(line.substr(0, pos)));
      line.remove_prefix(pos + (type == ContentFilterType::ElemHide ? 2 : 3));
      ParseContentFilter(domains, line, type, data, line);
    }
  } else {
    ParseUrlFilter(line, data);
  }
}

std::unique_ptr<FlatbufferData> Converter::Pack(
    ConverterData& data,
    const Metadata& metadata) const {
  data.metadata_ = flat::CreateSubscriptionMetadata(
      data.builder_, data.builder_.CreateString(CurrentSchemaVersion()),
      data.builder_.CreateString(data.config_.url_.spec()),
      data.builder_.CreateString(metadata.GetHomepage()),
      data.builder_.CreateString(metadata.GetTitle()),
      data.builder_.CreateString(metadata.GetVersion()),
      metadata.GetExpires().InMilliseconds());

  auto subscription = flat::CreateSubscription(
      data.builder_, data.metadata_,
      data.WriteUrlFilterIndex(data.url_subresource_block_),
      data.WriteUrlFilterIndex(data.url_subresource_allow_),
      data.WriteUrlFilterIndex(data.url_popup_block_),
      data.WriteUrlFilterIndex(data.url_popup_allow_),
      data.WriteUrlFilterIndex(data.url_document_allow_),
      data.WriteUrlFilterIndex(data.url_elemhide_allow_),
      data.WriteUrlFilterIndex(data.url_generichide_allow_),
      data.WriteUrlFilterIndex(data.url_genericblock_allow_),
      data.WriteUrlFilterIndex(data.url_csp_block_),
      data.WriteUrlFilterIndex(data.url_csp_allow_),
      data.WriteUrlFilterIndex(data.url_rewrite_block_),
      data.WriteUrlFilterIndex(data.url_rewrite_allow_),
      data.WriteUrlFilterIndex(data.url_header_block_),
      data.WriteUrlFilterIndex(data.url_header_allow_),
      data.WriteElemhideFilterIndex(data.elemhide_index_),
      data.WriteElemhideFilterIndex(data.elemhide_emulation_index_),
      data.WriteElemhideFilterIndex(data.elemhide_exception_index_),
      data.WriteSnippetFilterIndex(data.snippet_index_));

  data.builder_.Finish(subscription, "ABP1");
  return std::make_unique<Buffer>(data.builder_.Release());
}

Converter::ContentFilterType Converter::FromContentFilterType(
    const char value) const {
  switch (value) {
    case '#':
      return ContentFilterType::ElemHide;
    case '@':
      return ContentFilterType::ElemHideException;
    case '?':
      return ContentFilterType::ElemHideEmulation;
    case '$':
      return ContentFilterType::Snippet;
    default:
      return ContentFilterType::Invalid;
  }
}

void Converter::ParseContentFilter(base::StringPiece domains_source,
                                   base::StringPiece selector,
                                   ContentFilterType type,
                                   ConverterData& data,
                                   base::StringPiece full_filter_line) const {
  auto domains = base::SplitString(domains_source, ",", base::TRIM_WHITESPACE,
                                   base::SPLIT_WANT_NONEMPTY);

  const bool is_snippet = type == ContentFilterType::Snippet;
  const bool emu_or_snippet =
      type == ContentFilterType::ElemHideEmulation || is_snippet;

  if (emu_or_snippet) {
    // emulation filter or snippet filter
    // require that domain have at least one subdomain or is localhost
    domains.erase(base::ranges::remove_if(domains,
                                          [](const base::StringPiece& domain) {
                                            return (domain != "localhost") &&
                                                   (domain.find('.') ==
                                                    base::StringPiece::npos);
                                          }),
                  domains.end());
  }

  // Split domains into include and exclude domains.
  const auto it_first_include_domain =
      std::partition(domains.begin(), domains.end(), &IsExcludedDomain);

  const base::span<std::string> exclude_domains(domains.begin(),
                                                it_first_include_domain);
  const base::span<std::string> include_domains(it_first_include_domain,
                                                domains.end());

  if (emu_or_snippet && include_domains.empty()) {
    DLOG(ERROR)
        << "[eyeo] Filter \"" << full_filter_line
        << "\" has no include domains specified. Elemhide emulation and "
           "snippet filters require include domains.";
    return;
  }

  // Remove the ~ prefix that indicates an exclude domain.
  for (auto& domain : exclude_domains)
    base::RemoveChars(domain, "~", &domain);

  if (is_snippet) {
    if (data.config_.allow_privileged_) {
      ParseSnippetFilter(include_domains, exclude_domains, selector, data);
    } else {
      LOG(WARNING) << "[eyeo] Snippet filter is not allowed: "
                   << full_filter_line;
    }

    return;
  }

  const flatbuffers::Offset<flatbuffers::String> filter_text_unused = 0;

  auto offset = flat::CreateElemHideFilter(
      data.builder_, filter_text_unused,
      emu_or_snippet
          ? data.builder_.CreateString(selector.data(), selector.size())
          : data.builder_.CreateString(EscapeSelector(selector)),
      CreateVectorOfSharedStrings(include_domains, data.builder_),
      CreateVectorOfSharedStrings(exclude_domains, data.builder_));

  // Insert the filter under the correct index.
  switch (type) {
    case ContentFilterType::ElemHide:
      data.AddElemhideFilterForDomains(data.elemhide_index_, include_domains,
                                       offset);
      break;
    case ContentFilterType::ElemHideException:
      data.AddElemhideFilterForDomains(data.elemhide_exception_index_,
                                       include_domains, offset);
      break;
    case ContentFilterType::ElemHideEmulation:
      data.AddElemhideFilterForDomains(data.elemhide_emulation_index_,
                                       include_domains, offset);
      break;
    default:
      break;
  }
}

void Converter::ParseUrlFilter(base::StringPiece original_pattern,
                               ConverterData& data) const {
  UrlFilterData filter;
  base::StringPiece keyword_pattern = original_pattern;
  filter.third_party = flat::ThirdParty_Ignore;
  filter.is_allowing = base::StartsWith(keyword_pattern, "@@");
  if (filter.is_allowing)
    keyword_pattern.remove_prefix(2);

  size_t options_start_pos = keyword_pattern.rfind('$');
  // '$' can be part of raw regexp, so try rfind.
  // TODO (DPD-1277): Support filters that contain multiple '$'
  if (options_start_pos != std::string::npos &&
      !IsRegexFilter(keyword_pattern)) {
    if (options_start_pos != keyword_pattern.size() - 1) {
      for (const auto& cur : base::SplitString(
               keyword_pattern.substr(options_start_pos + 1), ",",
               base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
        if (!ParseOption(cur, filter, data)) {
          return;
        }
      }
    }
    keyword_pattern.remove_suffix(keyword_pattern.size() - options_start_pos);
  }

  if (filter.content_type == 0)
    filter.content_type = ContentType::Default;

  base::RemoveChars(keyword_pattern, base::kWhitespaceASCII, &filter.pattern);
  filter.is_regex = IsRegexFilter(filter.pattern);
  if (filter.is_regex) {
    filter.pattern = filter.pattern.substr(1, filter.pattern.size() - 2);
  } else {
    FilterToRegexp(filter.pattern);
  }

  if (ValidateUrlFilter(original_pattern, filter, data)) {
    AddUrlFilterToIndex(keyword_pattern, filter, data);
  }
}

bool Converter::ValidateUrlFilter(base::StringPiece original_pattern,
                                  const UrlFilterData& filter,
                                  const ConverterData& data) const {
  // Since we might join filters regex into a single regex pattern, we need
  // to validate each filter pattern. In case if one is invalid its not added
  // to the bundle and invalidate the other patterns.
  if (!IsValidRegex(filter.pattern)) {
    LOG(WARNING)
        << "[eyeo] Trying to convert into flatbuffer an invalid filter "
           "with pattern: "
        << original_pattern;
    return false;
  }

  if (filter.filter_types.count(UrlFilterType::Header) &&
      !data.config_.allow_privileged_) {
    LOG(WARNING) << "[eyeo] Header filter is not allowed: " << original_pattern;
    return false;
  }

  if (filter.filter_types.count(UrlFilterType::Rewrite)) {
    // rewrite MUST be restricted to at least one domain using the domain filter
    // option. rewrite MUST NOT be used together with the third-party filter
    // option, but MAY be used with ~third-party. The filter pattern MUST either
    // be a star (*) or start with a domain anchor double pipe (||).
    if (!filter.rewrite ||
        (!base::StartsWith(filter.pattern, R"(^[\w\-]+:\/+(?:[^\/]+\.)?)") &&
         !filter.pattern.empty()) ||
        filter.third_party == flat::ThirdParty_ThirdPartyOnly ||
        filter.include_domains.empty()) {
      LOG(WARNING) << "[eyeo] Rewrite filter is not allowed: "
                   << original_pattern;
      return false;
    }
  }

  return true;
}

void Converter::AddUrlFilterToIndex(base::StringPiece keyword_pattern,
                                    UrlFilterData& filter,
                                    ConverterData& data) const {
  auto offset = flat::CreateUrlFilter(
      data.builder_, {}, data.builder_.CreateString(filter.pattern),
      filter.match_case, filter.content_type, filter.third_party,
      data.builder_.CreateVector(filter.sitekeys),
      data.builder_.CreateVector(filter.include_domains),
      data.builder_.CreateVector(filter.exclude_domains),
      filter.rewrite
          ? flat::CreateRewrite(data.builder_, filter.rewrite.value())
          : flatbuffers::Offset<flat::Rewrite>(),
      filter.csp_filter
          ? data.builder_.CreateSharedString(filter.csp_filter.value())
          : flatbuffers::Offset<flatbuffers::String>(),
      filter.header_filter
          ? data.builder_.CreateSharedString(filter.header_filter.value())
          : flatbuffers::Offset<flatbuffers::String>());

  const absl::optional<base::StringPiece> pattern_text =
      filter.is_regex ? absl::optional<base::StringPiece>() : keyword_pattern;

  if (filter.header_filter) {
    data.AddUrlFilterToIndex(
        filter.is_allowing ? data.url_header_allow_ : data.url_header_block_,
        pattern_text, offset);
    return;
  }

  // If we didn't parse out any specific filter type, the default is
  // Subresource.
  if (filter.filter_types.empty()) {
    filter.filter_types.insert(UrlFilterType::Subresource);
  }

  for (auto filter_type : filter.filter_types) {
    switch (filter_type) {
      case UrlFilterType::Subresource:
        data.AddUrlFilterToIndex(filter.is_allowing
                                     ? data.url_subresource_allow_
                                     : data.url_subresource_block_,
                                 pattern_text, offset);
        break;
      case UrlFilterType::Genericblock:
        data.AddUrlFilterToIndex(data.url_genericblock_allow_, pattern_text,
                                 offset);
        break;
      case UrlFilterType::Generichide:
        data.AddUrlFilterToIndex(data.url_generichide_allow_, pattern_text,
                                 offset);
        break;
      case UrlFilterType::Document:
        data.AddUrlFilterToIndex(data.url_document_allow_, pattern_text,
                                 offset);
        break;
      case UrlFilterType::Elemhide:
        data.AddUrlFilterToIndex(data.url_elemhide_allow_, pattern_text,
                                 offset);
        break;
      case UrlFilterType::Popup:
        data.AddUrlFilterToIndex(
            filter.is_allowing ? data.url_popup_allow_ : data.url_popup_block_,
            pattern_text, offset);
        break;
      case UrlFilterType::Csp:
        data.AddUrlFilterToIndex(
            filter.is_allowing ? data.url_csp_allow_ : data.url_csp_block_,
            pattern_text, offset);
        break;
      case UrlFilterType::Rewrite:
        data.AddUrlFilterToIndex(filter.is_allowing ? data.url_rewrite_allow_
                                                    : data.url_rewrite_block_,
                                 pattern_text, offset);
        break;
      case UrlFilterType::Header:
        NOTREACHED();
        break;
      default:
        // Unsupported filter type
        break;
    }
  }
}

// Returns false if the option is improperly formed, which indicates the
// filter is invalid.
bool Converter::ParseOption(base::StringPiece input,
                            UrlFilterData& filter,
                            ConverterData& data) const {
  if (input.empty())
    return true;

  base::StringPiece option = input;
  base::StringPiece value;
  bool inverse = false;
  auto pos = option.find('=');

  if (pos != std::string::npos) {
    value = option.substr(pos + 1);
    option.remove_suffix(option.size() - pos);
  }

  std::string trimmed_option;
  base::RemoveChars(option, base::kWhitespaceASCII, &trimmed_option);
  trimmed_option = base::ToLowerASCII(trimmed_option);

  if (trimmed_option.front() == '~') {
    trimmed_option = trimmed_option.substr(1);
    inverse = true;
  }

  if (trimmed_option == "match-case") {
    filter.match_case = !inverse;
  } else if (trimmed_option == "third-party") {
    filter.third_party = inverse ? flat::ThirdParty_FirstPartyOnly
                                 : flat::ThirdParty_ThirdPartyOnly;
  } else if (trimmed_option == "rewrite") {
    return ParseRewrite(value, filter);
  } else if (trimmed_option == "domain") {
    return ParseDomains(value, filter, data);
  } else if (trimmed_option == "sitekey") {
    return ParseSitekeys(value, filter, data);
  } else if (trimmed_option == "csp") {
    return ParseCsp(value, filter);
  } else if (trimmed_option == "header") {
    if (data.config_.allow_privileged_) {
      return ParseHeaderFilter(value, filter);
    } else {
      LOG(WARNING) << "[eyeo] Header filter is not allowed: " << input;
    }
  } else {
    // Filter type options (ex. $document) and content type options (ex. $image)
    // have no value to parse, but instead *are* values that need to be
    // reflected in filter metadata. We will only learn which of those types
    // we're dealing with once we've tried to parse them.
    return ParseFilterType(trimmed_option, filter) &&
           ParseContentType(trimmed_option, inverse, filter);
  }

  return true;
}

bool Converter::ParseRewrite(base::StringPiece value,
                             UrlFilterData& filter) const {
  if (value == "abp-resource:blank-text")
    filter.rewrite = flat::AbpResource_BlankText;
  else if (value == "abp-resource:blank-css")
    filter.rewrite = flat::AbpResource_BlankCss;
  else if (value == "abp-resource:blank-js")
    filter.rewrite = flat::AbpResource_BlankJs;
  else if (value == "abp-resource:blank-html")
    filter.rewrite = flat::AbpResource_BlankHtml;
  else if (value == "abp-resource:blank-mp3")
    filter.rewrite = flat::AbpResource_BlankMp3;
  else if (value == "abp-resource:blank-mp4")
    filter.rewrite = flat::AbpResource_BlankMp4;
  else if (value == "abp-resource:1x1-transparent-gif")
    filter.rewrite = flat::AbpResource_TransparentGif1x1;
  else if (value == "abp-resource:2x2-transparent-png")
    filter.rewrite = flat::AbpResource_TransparentPng2x2;
  else if (value == "abp-resource:3x2-transparent-png")
    filter.rewrite = flat::AbpResource_TransparentPng3x2;
  else if (value == "abp-resource:32x32-transparent-png")
    filter.rewrite = flat::AbpResource_TransparentPng32x32;
  else {
    LOG(WARNING) << "[eyeo] Invalid Rewrite filter. Unknown url: " << value;
    return false;
  }

  filter.filter_types.insert(UrlFilterType::Rewrite);
  return true;
}

// Returns false if the domains are improperly formed, which indicates the
// filter is invalid.
bool Converter::ParseDomains(base::StringPiece value,
                             UrlFilterData& filter,
                             ConverterData& data) const {
  if (value.empty())
    return false;

  auto domains =
      base::SplitString(base::ToLowerASCII(value), "|", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);

  domains.erase(std::remove(domains.begin(), domains.end(), "~"),
                domains.end());

  if (domains.empty())
    return false;

  for (const auto& cur : domains) {
    if (IsExcludedDomain(cur)) {
      filter.exclude_domains.push_back(
          data.builder_.CreateSharedString(cur.substr(1)));
    } else {
      filter.include_domains.push_back(data.builder_.CreateSharedString(cur));
    }
  }

  return true;
}

// Returns false if the sitekey is improperly formed, which indicates the filter
// is invalid.
bool Converter::ParseSitekeys(base::StringPiece value,
                              UrlFilterData& filter,
                              ConverterData& data) const {
  std::string nospace;
  base::RemoveChars(value, base::kWhitespaceASCII, &nospace);
  value = nospace;

  if (value.empty())
    return false;

  auto sitekeys =
      base::SplitString(base::ToUpperASCII(value), "|", base::KEEP_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
  std::sort(sitekeys.begin(), sitekeys.end());
  filter.sitekeys.reserve(sitekeys.size());

  for (const auto& cur : sitekeys)
    filter.sitekeys.push_back(data.builder_.CreateString(cur));

  return true;
}

// Returns false if the CSP is improperly formed, which indicates the filter
// is invalid.
bool Converter::ParseCsp(base::StringPiece value, UrlFilterData& filter) const {
  // CSP directives are mandatory for blocking CSP filters but not for allowing
  // ones
  if (value.empty() && !filter.is_allowing) {
    LOG(WARNING)
        << "[eyeo] Invalid CSP filter. Blocking CSP filter requires directives";
    return false;
  }

  static re2::RE2 invalid_csp(
      "(;|^) "
      "?(base-uri|referrer|report-to|report-uri|upgrade-insecure-requests)\\b");

  if (re2::RE2::PartialMatch(re2::StringPiece(value.data(), value.size()),
                             invalid_csp)) {
    LOG(WARNING) << "[eyeo] Invalid CSP filter. Invalid directives: " << value;
    return false;
  }

  filter.csp_filter = std::string(value.data(), value.size());
  filter.filter_types.insert(UrlFilterType::Csp);

  return true;
}

bool Converter::ParseHeaderFilter(base::StringPiece value,
                                  UrlFilterData& filter) const {
  // Header directives are mandatory for blocking filters but not for allowing
  // ones
  if (value.empty() && !filter.is_allowing) {
    LOG(WARNING) << "[eyeo] Invalid header filter. Blocking header filter "
                    "requires directives";
    return false;
  }

  auto value_str = std::string(value.data(), value.size());
  // replace \x2c with actual ,
  static re2::RE2 r1("([^\\\\])\\\\x2c");
  re2::RE2::GlobalReplace(&value_str, r1, "\\1,");

  // remove extra escape for \\x2c which left
  static re2::RE2 r2("\\\\x2c");
  re2::RE2::GlobalReplace(&value_str, r2, "x2c");

  filter.header_filter = value_str;

  return true;
}

// Returns false if the filter type is unsupported or invalid. No filter type
// is correct and the function returns true, this indicates a default,
// Subresource filter type.
bool Converter::ParseFilterType(base::StringPiece option,
                                UrlFilterData& filter) const {
  UrlFilterType type = UrlFilterTypeFromString(option);
  if (type == UrlFilterType::Unknown)
    return true;

  filter.filter_types.insert(type);
  return true;
}

// Parses content type. It is legal for a filter to not have a content type,
// in that case it applies to all content types.
bool Converter::ParseContentType(base::StringPiece option,
                                 bool inverse,
                                 UrlFilterData& filter) const {
  ContentType type = ContentTypeFromString(option);

  if (type == ContentType::Unknown) {
    // TODO since this function is called *last* when parsing a filter, if it
    // fails to parse |option|, it means it's an unknown option. We could return
    // false to indicate the filter is invalid. However, this would be fragile,
    // as changing the order to parse ContentType before FilterType would be
    // enough to break the converter. An |option| that isn't a valid ContentType
    // could turn out to be a valid FilterType.
    // We shouldn't depend on order of operations this much, refactor this in
    // DPD-784.
    return true;
  }

  filter.filter_types.insert(UrlFilterType::Subresource);

  if (inverse) {
    if (filter.content_type == 0)
      filter.content_type = ContentType::Default;

    filter.content_type &= ~type;
  } else
    filter.content_type |= type;

  return true;
}

void Converter::FilterToRegexp(std::string& value) const {
  static re2::RE2 multi_start("\\*+");
  re2::RE2::GlobalReplace(&value, multi_start, "*");

  if (!value.empty() && value.front() == '*')
    value.erase(0, 1);

  if (!value.empty() && value.back() == '*')
    value.pop_back();

  // remove anchors following separator placeholder
  static re2::RE2 r1("\\^\\|$");
  re2::RE2::GlobalReplace(&value, r1, "^");

  // escape special symbols
  static re2::RE2 r2("\\W");
  re2::RE2::GlobalReplace(&value, r2, R"(\\\0)");

  // replace wildcards by .*
  static re2::RE2 r3("\\\\\\*");
  re2::RE2::GlobalReplace(&value, r3, ".*");

  // process separator placeholders (all ANSI characters but alphanumeric
  // characters and _%.-)
  static re2::RE2 r4("\\\\\\^");
  re2::RE2::GlobalReplace(
      &value, r4,
      R"((?:[\\x00-\\x24\\x26-\\x2C\\x2F\\x3A-\\x40\\x5B-\\x5E\\x60\\x7B-\\x7F]|$))");

  // process extended anchor at expression start
  static re2::RE2 r5("^\\\\\\|\\\\\\|");
  re2::RE2::GlobalReplace(&value, r5, R"(^[\\w\\-]+:\\/+(?:[^\\/]+\\.)?)");

  // process anchor at expression start
  static re2::RE2 r6("^\\\\\\|");
  re2::RE2::GlobalReplace(&value, r6, "^");

  // process anchor at expression end
  static re2::RE2 r7("\\\\\\|$");
  re2::RE2::GlobalReplace(&value, r7, "$");
}

void Converter::ParseSnippetFilter(
    const base::span<std::string>& include_domains,
    const base::span<std::string>& exclude_domains,
    base::StringPiece selector,
    ConverterData& data) const {
  // See parseScript in adblockpluscore/lib/snippet.js. base::StringTokenizer
  // allows to do something similar, but it fails to tokenize for the case:
  // function 'aaa'bbb'ccc';
  // which is valid 1 argument call. Let us do it with custom implementation.

  auto script = converter_snippet_tokenizer_->Tokenize(selector);
  if (script.empty())
    return;

  std::vector<flatbuffers::Offset<adblock::flat::SnippetFunctionCall>> offsets;
  offsets.reserve(script.size());
  for (const auto& cur : script) {
    offsets.push_back(flat::CreateSnippetFunctionCall(
        data.builder_, data.builder_.CreateSharedString(cur.front()),
        data.builder_.CreateVectorOfStrings(++cur.begin(), cur.end())));
  }

  const flatbuffers::Offset<flatbuffers::String> filter_text_unused = 0;
  auto offset = flat::CreateSnippetFilter(
      data.builder_, filter_text_unused,
      CreateVectorOfSharedStrings(include_domains, data.builder_),
      CreateVectorOfSharedStrings(exclude_domains, data.builder_),
      data.builder_.CreateVector(offsets));
  data.AddSnippetFilterForDomains(data.snippet_index_, include_domains, offset);
}

}  // namespace adblock

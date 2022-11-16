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

#include "components/adblock/core/converter/snippet_tokenizer_impl.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/third_party/icu/icu_utf.h"

namespace adblock {

SnippetTokenizerImpl::SnippetTokenizerImpl() = default;

SnippetTokenizerImpl::~SnippetTokenizerImpl() = default;

SnippetTokenizerImpl::SnippetScript SnippetTokenizerImpl::Tokenize(
    const base::StringPiece& input) {
  SnippetScript script;
  token_.clear();
  arguments_.clear();
  escape_ = false;
  quotes_just_closed_ = false;
  within_quotes_ = false;
  for (char ch : input) {
    if (escape_) {
      AddEscape(ch);
      quotes_just_closed_ = false;
    } else if (ch == '\\') {
      escape_ = true;
      quotes_just_closed_ = false;
    } else if (ch == '\'') {
      within_quotes_ = !within_quotes_;
      quotes_just_closed_ = !within_quotes_;
    } else if (within_quotes_ || (ch != ';' && !base::IsAsciiWhitespace(ch))) {
      token_ += ch;
      quotes_just_closed_ = false;
    } else {
      AddArgument();
      if (ch == ';')
        AddFunctionCall(script);
      quotes_just_closed_ = false;
    }
  }
  AddArgument();
  AddFunctionCall(script);
  return script;
}

void SnippetTokenizerImpl::AddEscape(const char ch) {
  escape_ = false;
  switch (ch) {
    case 'r':
      token_ += '\r';
      break;
    case 'n':
      token_ += '\n';
      break;
    case 't':
      token_ += '\t';
      break;
    case 'u':
      token_ += "\\u";
      break;
    default:
      token_ += ch;
  }
}

void SnippetTokenizerImpl::AddArgument() {
  if (quotes_just_closed_ || !token_.empty()) {
    arguments_.push_back(token_);
    token_.clear();
  }
}

void SnippetTokenizerImpl::AddFunctionCall(SnippetScript& script) {
  // if within quote whole script is invalid
  // or if detected escape char but ended
  if (arguments_.empty() || within_quotes_ || escape_)
    return;
  script.push_back(arguments_);
  arguments_.clear();
}

}  // namespace adblock

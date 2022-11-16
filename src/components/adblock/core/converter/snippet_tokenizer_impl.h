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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_IMPL_H_

#include <vector>

#include "base/strings/string_piece.h"
#include "components/adblock/core/converter/snippet_tokenizer.h"

namespace adblock {

class SnippetTokenizerImpl : public SnippetTokenizer {
 public:
  SnippetTokenizerImpl();
  ~SnippetTokenizerImpl() override;

  SnippetScript Tokenize(const base::StringPiece& input) override;

 private:
  std::vector<std::string> arguments_;
  std::string token_;
  bool escape_ = false;
  bool quotes_just_closed_ = false;
  bool within_quotes_ = false;

  void AddEscape(const char ch);
  void AddArgument();
  void AddFunctionCall(SnippetScript& script);
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_IMPL_H_

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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_H_

#include <vector>

#include "base/strings/string_piece.h"

namespace adblock {

class SnippetTokenizer {
 public:
  using SnippetScript = std::vector<std::vector<std::string>>;
  virtual SnippetScript Tokenize(const base::StringPiece& input) = 0;
  virtual ~SnippetTokenizer() = default;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_SNIPPET_TOKENIZER_H_

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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_CONVERTER_RESULT_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_CONVERTER_RESULT_H_

#include <memory>

#include "components/adblock/core/common/flatbuffer_data.h"
#include "url/gurl.h"

namespace adblock {

//  TODO(atokodi): https://jira.eyeo.com/browse/DPD-1212
struct ConverterResult {
  ConverterResult();
  ConverterResult(ConverterResult&& other);
  ConverterResult& operator=(ConverterResult&& other);
  ~ConverterResult();

  enum { Ok, Redirect, Error } status;
  std::unique_ptr<FlatbufferData> data;
  GURL redirect_url;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_CONVERTER_RESULT_H_

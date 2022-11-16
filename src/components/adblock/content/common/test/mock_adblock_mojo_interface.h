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

#ifndef COMPONENTS_ADBLOCK_CONTENT_COMMON_TEST_MOCK_ADBLOCK_MOJO_INTERFACE_H_
#define COMPONENTS_ADBLOCK_CONTENT_COMMON_TEST_MOCK_ADBLOCK_MOJO_INTERFACE_H_

#include "components/adblock/content/common/mojom/adblock.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace adblock {

class MockAdblockMojoInterface : public mojom::AdblockInterface {
 public:
  MockAdblockMojoInterface();
  ~MockAdblockMojoInterface() override;

  MOCK_METHOD(void,
              CheckFilterMatch,
              (const GURL& request_url,
               int32_t resource_type,
               int32_t render_frame_id,
               mojom::AdblockInterface::CheckFilterMatchCallback),
              (override));
  MOCK_METHOD(void,
              Clone,
              (mojo::PendingReceiver<mojom::AdblockInterface>),
              (override));
  MOCK_METHOD(void,
              ProcessResponseHeaders,
              (const GURL& request_url,
               int32_t render_frame_id,
               const scoped_refptr<net::HttpResponseHeaders>&,
               const std::string& user_agent,
               mojom::AdblockInterface::ProcessResponseHeadersCallback),
              (override));
  MOCK_METHOD(void,
              CheckRewriteFilterMatch,
              (const GURL&,
               int32_t,
               mojom::AdblockInterface::CheckRewriteFilterMatchCallback),
              (override));
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CONTENT_COMMON_TEST_MOCK_ADBLOCK_MOJO_INTERFACE_H_

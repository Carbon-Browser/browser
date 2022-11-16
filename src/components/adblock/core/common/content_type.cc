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

#include "components/adblock/core/common/content_type.h"

namespace adblock {

std::string ContentTypeToString(ContentType content_type) {
  switch (content_type) {
    case ContentType::Unknown:
      return "UNKNOWN";
    case ContentType::Other:
      return "OTHER";
    case ContentType::Script:
      return "SCRIPT";
    case ContentType::Image:
      return "IMAGE";
    case ContentType::Stylesheet:
      return "STYLESHEET";
    case ContentType::Object:
      return "OBJECT";
    case ContentType::Subdocument:
      return "SUBDOCUMENT";
    case ContentType::Websocket:
      return "WEBSOCKET";
    case ContentType::Webrtc:
      return "WEBRTC";
    case ContentType::Ping:
      return "PING";
    case ContentType::Xmlhttprequest:
      return "XMLHTTPREQUEST";
    case ContentType::Media:
      return "MEDIA";
    case ContentType::Font:
      return "FONT";
    case ContentType::Default:
      return "DEFAULT";
  }
}

}  // namespace adblock

/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_CONTENT_TYPE_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_CONTENT_TYPE_H_

namespace adblock {

enum ContentType {
  CONTENT_TYPE_OTHER = 1,
  CONTENT_TYPE_SCRIPT = 2,
  CONTENT_TYPE_IMAGE = 4,
  CONTENT_TYPE_STYLESHEET = 8,
  CONTENT_TYPE_OBJECT = 16,
  CONTENT_TYPE_SUBDOCUMENT = 32,
  CONTENT_TYPE_WEBSOCKET = 128,
  CONTENT_TYPE_WEBRTC = 256,
  CONTENT_TYPE_PING = 1024,
  CONTENT_TYPE_XMLHTTPREQUEST = 2048,
  CONTENT_TYPE_MEDIA = 16384,
  CONTENT_TYPE_FONT = 32768,
  CONTENT_TYPE_POPUP = 1 << 24,
  CONTENT_TYPE_CSP = 1 << 25,
  CONTENT_TYPE_HEADER = 1 << 26,
  CONTENT_TYPE_DOCUMENT = 1 << 27,
  CONTENT_TYPE_GENERICBLOCK = 1 << 28,
  CONTENT_TYPE_ELEMHIDE = 1 << 29,
  CONTENT_TYPE_GENERICHIDE = 1 << 30
};

}

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_CONTENT_TYPE_H_
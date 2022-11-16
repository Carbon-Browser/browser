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

#include <string>

#include "base/containers/flat_map.h"
#include "components/adblock/content/browser/adblock_content_utils.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace adblock::utils {

ContentType DetectResourceType(const GURL& url) {
  static base::flat_map<std::string, ContentType> content_type_map(
      {// javascript
       {".js", ContentType::Script},
       // css
       {".css", ContentType::Stylesheet},
       // image filename
       {".gif", ContentType::Image},
       {".apng", ContentType::Image},
       {".png", ContentType::Image},
       {".jpe", ContentType::Image},
       {".jpg", ContentType::Image},
       {".jpeg", ContentType::Image},
       {".bmp", ContentType::Image},
       {".ico", ContentType::Image},
       {".webp", ContentType::Image},
       // fonts
       {".ttf", ContentType::Font},
       {".woff", ContentType::Font},
       // document ( html )
       {".html", ContentType::Subdocument},
       {".htm", ContentType::Subdocument}});

  const std::string file_name(url.ExtractFileName());

  // get file extension
  const size_t pos = file_name.find_last_of('.');
  if (pos == std::string::npos) {
    return ContentType::Other;
  }
  const std::string file_extension(base::ToLowerASCII(file_name.substr(pos)));

  const auto content_type = content_type_map.find(file_extension);

  if (content_type_map.end() == content_type) {
    return ContentType::Other;
  }
  return content_type->second;
}

ContentType ConvertToAdblockResourceType(const GURL& url,
                                         int32_t resource_type) {
  if (resource_type <
      static_cast<int32_t>(blink::mojom::ResourceType::kMinValue))
    return DetectResourceType(url);
  if (resource_type >
      static_cast<int32_t>(blink::mojom::ResourceType::kMaxValue))
    return ContentType::Other;

  const auto blink_resource_type =
      static_cast<blink::mojom::ResourceType>(resource_type);
  switch (blink_resource_type) {
    case blink::mojom::ResourceType::kImage:
    case blink::mojom::ResourceType::kFavicon:
      return ContentType::Image;

    case blink::mojom::ResourceType::kXhr:
      return ContentType::Xmlhttprequest;

    case blink::mojom::ResourceType::kStylesheet:
      return ContentType::Stylesheet;

    case blink::mojom::ResourceType::kScript:
      return ContentType::Script;

    case blink::mojom::ResourceType::kFontResource:
      return ContentType::Font;

    case blink::mojom::ResourceType::kObject:
      return ContentType::Object;

    case blink::mojom::ResourceType::kSubFrame:
      // though subframe is a visual element we will elemhide it later
      // (see DP-617 for details)
      return ContentType::Subdocument;

    case blink::mojom::ResourceType::kMedia:
      return ContentType::Media;

    case blink::mojom::ResourceType::kPing:
      return ContentType::Ping;

    default:
      break;
  }

  return ContentType::Other;
}

}  // namespace adblock::utils

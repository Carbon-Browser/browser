/*                                                                            \
 * This file is part of eyeo Chromium SDK,                                    \
 * Copyright (C) 2006-present eyeo GmbH                                       \
 *                                                                            \
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify  \
 * it under the terms of the GNU General Public License version 3 as          \
 * published by the Free Software Foundation.                                 \
 *                                                                            \
 * eyeo Chromium SDK is distributed in the hope that it will be useful,       \
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             \
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              \
 * GNU General Public License for more details.                               \
 *                                                                            \
 * You should have received a copy of the GNU General Public License          \
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>. \
 */

#include "chrome/browser/ui/webui/adblock_internals/adblock_internals_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/adblock/content/browser/adblock_internals_page_handler.h"
#include "components/adblock/core/common/web_ui_constants.h"
#include "components/grit/adblock_internals_resources.h"
#include "components/grit/adblock_internals_resources_map.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"

namespace adblock {

AdblockInternalsUI::AdblockInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      this->web_ui()->GetWebContents()->GetBrowserContext(),
      adblock::kChromeUIAdblockInternalsHost);
  webui::SetupWebUIDataSource(source,
                              base::make_span(kAdblockInternalsResources,
                                              kAdblockInternalsResourcesSize),
                              IDR_ADBLOCK_INTERNALS_ADBLOCK_INTERNALS_HTML);
}

AdblockInternalsUI::~AdblockInternalsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(AdblockInternalsUI)

void AdblockInternalsUI::BindInterface(
    mojo::PendingReceiver<mojom::adblock_internals::AdblockInternalsPageHandler>
        receiver) {
  handler_ = std::make_unique<AdblockInternalsPageHandler>(
      Profile::FromWebUI(web_ui())->GetOriginalProfile(), std::move(receiver));
}

}  // namespace adblock

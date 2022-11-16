// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/manage_mirrorsync/manage_mirrorsync_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/manage_mirrorsync_resources.h"
#include "chrome/grit/manage_mirrorsync_resources_map.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace chromeos {

ManageMirrorSyncUI::ManageMirrorSyncUI(content::WebUI* web_ui)
    : ui::MojoWebDialogUI{web_ui} {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIManageMirrorSyncHost);
  auto* profile = Profile::FromWebUI(web_ui);
  webui::SetupWebUIDataSource(source,
                              base::make_span(kManageMirrorsyncResources,
                                              kManageMirrorsyncResourcesSize),
                              IDR_MANAGE_MIRRORSYNC_INDEX_HTML);

  content::WebUIDataSource::Add(profile, source);
}

ManageMirrorSyncUI::~ManageMirrorSyncUI() = default;

void ManageMirrorSyncUI::BindInterface(
    mojo::PendingReceiver<
        chromeos::manage_mirrorsync::mojom::PageHandlerFactory>
        pending_receiver) {
  if (factory_receiver_.is_bound()) {
    factory_receiver_.reset();
  }
  factory_receiver_.Bind(std::move(pending_receiver));
}

void ManageMirrorSyncUI::CreatePageHandler(
    mojo::PendingReceiver<chromeos::manage_mirrorsync::mojom::PageHandler>
        receiver) {
  page_handler_ =
      std::make_unique<ManageMirrorSyncPageHandler>(std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(ManageMirrorSyncUI)

}  // namespace chromeos

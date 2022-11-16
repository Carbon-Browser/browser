// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/side_panel/side_panel_service.h"

#include <cstddef>
#include <memory>

#include "base/no_destructor.h"
#include "chrome/common/extensions/api/side_panel.h"
#include "chrome/common/extensions/api/side_panel/side_panel_info.h"
#include "components/sessions/core/session_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace extensions {

namespace {

api::side_panel::PanelOptions GetPanelOptionsFromManifest(
    const Extension& extension) {
  auto path = SidePanelInfo::GetDefaultPath(&extension);
  api::side_panel::PanelOptions options;
  if (!path.empty()) {
    options.path = std::make_unique<std::string>(std::string(path));
    options.enabled = std::make_unique<bool>(true);
  }
  return options;
}

// TODO(crbug.com/1332599): Add a Clone() method for generated types.
api::side_panel::PanelOptions CloneOptions(
    const api::side_panel::PanelOptions& options) {
  auto clone = api::side_panel::PanelOptions::FromValue(
      base::Value(std::move(options.ToValue()->GetDict())));
  return clone ? std::move(*clone) : api::side_panel::PanelOptions();
}

}  // namespace

SidePanelService::~SidePanelService() = default;

SidePanelService::SidePanelService(content::BrowserContext* context)
    : browser_context_(context) {}

api::side_panel::PanelOptions SidePanelService::GetOptions(
    const Extension& extension,
    absl::optional<TabId> id) {
  auto extension_panel_options = panels_.find(extension.id());

  // Get default path from manifest if nothing was stored in this service for
  // the calling extension.
  if (extension_panel_options == panels_.end()) {
    return GetPanelOptionsFromManifest(extension);
  }

  TabId default_tab_id = SessionID::InvalidValue().id();
  TabId tab_id = id.has_value() ? id.value() : default_tab_id;
  TabPanelOptions& tab_panel_options = extension_panel_options->second;

  // The specific `tab_id` may have already been saved.
  if (tab_id != default_tab_id) {
    auto specific_tab_options = tab_panel_options.find(tab_id);
    if (specific_tab_options != tab_panel_options.end())
      return CloneOptions(specific_tab_options->second);
  }

  // Fall back to the default tab if no tab ID was specified or entries for the
  // specific tab weren't found.
  auto default_options = tab_panel_options.find(default_tab_id);
  if (default_options != tab_panel_options.end()) {
    auto options = CloneOptions(default_options->second);
    return options;
  }

  // Fall back to the manifest-specified options as a last resort.
  return GetPanelOptionsFromManifest(extension);
}

// Upsert to merge `panels_[extension_id][tab_id]` with `set_options`.
void SidePanelService::SetOptions(const Extension& extension,
                                  api::side_panel::PanelOptions options) {
  TabId tab_id = SessionID::InvalidValue().id();
  if (options.tab_id)
    tab_id = *options.tab_id;
  TabPanelOptions& extension_panel_options = panels_[extension.id()];
  auto it = extension_panel_options.find(tab_id);
  if (it == extension_panel_options.end()) {
    extension_panel_options[tab_id] = std::move(options);
  } else {
    auto& existing_options = it->second;
    if (options.path)
      existing_options.path = std::move(options.path);
    if (options.enabled)
      existing_options.enabled = std::move(options.enabled);
  }
}

// static
BrowserContextKeyedAPIFactory<SidePanelService>*
SidePanelService::GetFactoryInstance() {
  static base::NoDestructor<BrowserContextKeyedAPIFactory<SidePanelService>>
      instance;
  return instance.get();
}

// static
SidePanelService* SidePanelService::Get(content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<SidePanelService>::Get(context);
}

}  // namespace extensions

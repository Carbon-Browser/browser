// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/webui/ui_bundled/user_actions_ui.h"

#import "components/grit/user_actions_ui_resources.h"
#import "components/grit/user_actions_ui_resources_map.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#import "ios/chrome/browser/webui/ui_bundled/user_actions_handler.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_data_source.h"

UserActionsUI::UserActionsUI(web::WebUIIOS* web_ui, const std::string& host)
    : web::WebUIIOSController(web_ui, host) {
  web_ui->AddMessageHandler(std::make_unique<UserActionsHandler>());

  // Set up the chrome://user-actions/ source.
  web::WebUIIOSDataSource* html_source =
      web::WebUIIOSDataSource::Create(kChromeUIUserActionsHost);
  html_source->AddResourcePaths(kUserActionsUiResources);
  html_source->AddResourcePath("", IDR_USER_ACTIONS_UI_USER_ACTIONS_HTML);
  web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), html_source);
}

UserActionsUI::~UserActionsUI() {}

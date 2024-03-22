// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This source code is a part of eyeo Chromium SDK.
// Use of this source code is governed by the GPLv3 that can be found in the
// components/adblock/LICENSE file.

#include "android_webview/browser/aw_web_ui_controller_factory.h"

#include "android_webview/browser/adblock/aw_adblock_internals_ui.h"
#include "base/memory/ptr_util.h"
#include "components/adblock/core/common/web_ui_constants.h"
#include "components/safe_browsing/content/browser/web_ui/safe_browsing_ui.h"
#include "components/safe_browsing/core/common/web_ui_constants.h"
#include "content/public/browser/web_ui.h"
#include "url/gurl.h"

using content::WebUI;
using content::WebUIController;

namespace {

const WebUI::TypeID kSafeBrowsingID = &kSafeBrowsingID;
const WebUI::TypeID kAdblockInternalsID = &kAdblockInternalsID;

// A function for creating a new WebUI. The caller owns the return value, which
// may be nullptr (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunctionPointer)(WebUI* web_ui,
                                                        const GURL& url);

// Template for defining WebUIFactoryFunctionPointer.
template <class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

WebUIFactoryFunctionPointer GetWebUIFactoryFunctionPointer(const GURL& url) {
  // WebUI pages here must remain in the base module instead of being moved to
  // the Developer UI Dynamic Feature Module (DevUI DFM). Therefore the hosts
  // here must not appear in IsWebUiHostInDevUiDfm().
  if (url.host() == safe_browsing::kChromeUISafeBrowsingHost) {
    return &NewWebUI<safe_browsing::SafeBrowsingUI>;
  }

  if (url.host() == adblock::kChromeUIAdblockInternalsHost) {
    return &NewWebUI<adblock::AwAdblockInternalsUI>;
  }

  return nullptr;
}

WebUI::TypeID GetWebUITypeID(const GURL& url) {
  if (url.host() == safe_browsing::kChromeUISafeBrowsingHost) {
    return kSafeBrowsingID;
  }

  if (url.host() == adblock::kChromeUIAdblockInternalsHost) {
    return kAdblockInternalsID;
  }

  return WebUI::kNoWebUI;
}

}  // namespace

namespace android_webview {

// static
AwWebUIControllerFactory* AwWebUIControllerFactory::GetInstance() {
  return base::Singleton<AwWebUIControllerFactory>::get();
}

AwWebUIControllerFactory::AwWebUIControllerFactory() {}

AwWebUIControllerFactory::~AwWebUIControllerFactory() {}

WebUI::TypeID AwWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUITypeID(url);
}

bool AwWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

std::unique_ptr<WebUIController>
AwWebUIControllerFactory::CreateWebUIControllerForURL(WebUI* web_ui,
                                                      const GURL& url) {
  WebUIFactoryFunctionPointer function = GetWebUIFactoryFunctionPointer(url);
  if (!function)
    return nullptr;

  return base::WrapUnique((*function)(web_ui, url));
}

}  // namespace android_webview

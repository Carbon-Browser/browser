// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_service/publishers/standalone_browser_extension_apps_factory.h"

#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/apps/app_service/publishers/standalone_browser_extension_apps.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/services/app_service/public/cpp/app_types.h"

namespace apps {

/******** StandaloneBrowserExtensionAppsFactoryForApp ********/

// static
StandaloneBrowserExtensionApps*
StandaloneBrowserExtensionAppsFactoryForApp::GetForProfile(Profile* profile) {
  return static_cast<StandaloneBrowserExtensionApps*>(
      StandaloneBrowserExtensionAppsFactoryForApp::GetInstance()
          ->GetServiceForBrowserContext(profile, true /* create */));
}

// static
StandaloneBrowserExtensionAppsFactoryForApp*
StandaloneBrowserExtensionAppsFactoryForApp::GetInstance() {
  return base::Singleton<StandaloneBrowserExtensionAppsFactoryForApp>::get();
}

// static
void StandaloneBrowserExtensionAppsFactoryForApp::ShutDownForTesting(
    content::BrowserContext* context) {
  auto* factory = GetInstance();
  factory->BrowserContextShutdown(context);
  factory->BrowserContextDestroyed(context);
}

StandaloneBrowserExtensionAppsFactoryForApp::
    StandaloneBrowserExtensionAppsFactoryForApp()
    : BrowserContextKeyedServiceFactory(
          "StandaloneBrowserExtensionAppsForApp",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(apps::AppServiceProxyFactory::GetInstance());
}

KeyedService*
StandaloneBrowserExtensionAppsFactoryForApp::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new StandaloneBrowserExtensionApps(
      AppServiceProxyFactory::GetForProfile(
          Profile::FromBrowserContext(context)),
      AppType::kStandaloneBrowserChromeApp);
}

content::BrowserContext*
StandaloneBrowserExtensionAppsFactoryForApp::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* const profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }

  // Use OTR profile for Guest Session.
  if (profile->IsGuestSession()) {
    return profile->IsOffTheRecord()
               ? chrome::GetBrowserContextOwnInstanceInIncognito(context)
               : nullptr;
  }

  return BrowserContextKeyedServiceFactory::GetBrowserContextToUse(context);
}

/******** StandaloneBrowserExtensionAppsFactoryForExtension ********/

// static
StandaloneBrowserExtensionApps*
StandaloneBrowserExtensionAppsFactoryForExtension::GetForProfile(
    Profile* profile) {
  return static_cast<StandaloneBrowserExtensionApps*>(
      StandaloneBrowserExtensionAppsFactoryForExtension::GetInstance()
          ->GetServiceForBrowserContext(profile, true /* create */));
}

// static
StandaloneBrowserExtensionAppsFactoryForExtension*
StandaloneBrowserExtensionAppsFactoryForExtension::GetInstance() {
  return base::Singleton<
      StandaloneBrowserExtensionAppsFactoryForExtension>::get();
}

// static
void StandaloneBrowserExtensionAppsFactoryForExtension::ShutDownForTesting(
    content::BrowserContext* context) {
  auto* factory = GetInstance();
  factory->BrowserContextShutdown(context);
  factory->BrowserContextDestroyed(context);
}

StandaloneBrowserExtensionAppsFactoryForExtension::
    StandaloneBrowserExtensionAppsFactoryForExtension()
    : BrowserContextKeyedServiceFactory(
          "StandaloneBrowserExtensionAppsForExtension",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(apps::AppServiceProxyFactory::GetInstance());
}

KeyedService*
StandaloneBrowserExtensionAppsFactoryForExtension::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new StandaloneBrowserExtensionApps(
      AppServiceProxyFactory::GetForProfile(
          Profile::FromBrowserContext(context)),
      AppType::kStandaloneBrowserExtension);
}

content::BrowserContext*
StandaloneBrowserExtensionAppsFactoryForExtension::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* const profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }

  // Use OTR profile for Guest Session.
  if (profile->IsGuestSession()) {
    return profile->IsOffTheRecord()
               ? chrome::GetBrowserContextOwnInstanceInIncognito(context)
               : nullptr;
  }

  return BrowserContextKeyedServiceFactory::GetBrowserContextToUse(context);
}

}  // namespace apps

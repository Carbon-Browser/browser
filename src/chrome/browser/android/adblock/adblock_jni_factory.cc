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

#include "chrome/browser/android/adblock/adblock_jni_factory.h"

#include <memory>

#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/adblock/resource_classification_runner_factory.h"
#include "chrome/browser/android/adblock/adblock_jni.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
AdblockJNI* AdblockJNIFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockJNI*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
AdblockJNIFactory* AdblockJNIFactory::GetInstance() {
  static base::NoDestructor<AdblockJNIFactory> instance;
  return instance.get();
}

AdblockJNIFactory::AdblockJNIFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockJNI",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AdblockControllerFactory::GetInstance());
  DependsOn(ResourceClassificationRunnerFactory::GetInstance());
}

AdblockJNIFactory::~AdblockJNIFactory() = default;

KeyedService* AdblockJNIFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AdblockJNI(
      ResourceClassificationRunnerFactory::GetForBrowserContext(context),
      AdblockControllerFactory::GetForBrowserContext(context));
}

content::BrowserContext* AdblockJNIFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool AdblockJNIFactory::ServiceIsCreatedWithBrowserContext() const {
  // This avoids manual instantiation in chrome_browser_main.cc
  return true;
}

bool AdblockJNIFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace adblock

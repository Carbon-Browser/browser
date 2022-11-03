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

#include "chrome/browser/android/adblock/adblock_jni_factory.h"

#include <memory>

#include "chrome/browser/adblock/adblock_controller_factory.h"
#include "chrome/browser/adblock/adblock_request_classifier_factory.h"
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
  DependsOn(AdblockRequestClassifierFactory::GetInstance());
}

AdblockJNIFactory::~AdblockJNIFactory() = default;

KeyedService* AdblockJNIFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AdblockJNI(
      AdblockRequestClassifierFactory::GetForBrowserContext(context),
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

}  // namespace adblock

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

#include "chrome/browser/adblock/adblock_controller_factory.h"

#include <memory>

#include "chrome/browser/adblock/adblock_platform_embedder_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/adblock_controller_impl.h"
#include "components/adblock/adblock_prefs.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
AdblockController* AdblockControllerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
AdblockControllerFactory* AdblockControllerFactory::GetInstance() {
  static base::NoDestructor<AdblockControllerFactory> instance;
  return instance.get();
}

AdblockControllerFactory::AdblockControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockController",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AdblockPlatformEmbedderFactory::GetInstance());
}

AdblockControllerFactory::~AdblockControllerFactory() = default;

KeyedService* AdblockControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto embedder = AdblockPlatformEmbedderFactory::GetForBrowserContext(context);
  auto controller = std::make_unique<AdblockControllerImpl>(
      Profile::FromBrowserContext(context)->GetPrefs(), embedder);
  embedder->CreateFilterEngine(controller.get());
  return controller.release();
}

content::BrowserContext* AdblockControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

void AdblockControllerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  adblock::prefs::RegisterProfilePrefs(registry);
}

bool AdblockControllerFactory::ServiceIsCreatedWithBrowserContext() const {
  // Creating this service has an important side-effect of creating filter
  // engine in AdblockPlatformEmbedder. This is not ideal but solves a circular
  // dependency - Controller needs to register as Embedder's observer, Embedder
  // needs Controller to read initial state.
  return true;
}

}  // namespace adblock

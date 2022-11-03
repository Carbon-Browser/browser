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

#include "chrome/browser/adblock/adblock_sitekey_storage_factory.h"

#include <memory>

#include "chrome/browser/adblock/adblock_platform_embedder_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/adblock_sitekey_storage_libabp_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
AdblockSitekeyStorage* AdblockSitekeyStorageFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockSitekeyStorage*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
AdblockSitekeyStorageFactory* AdblockSitekeyStorageFactory::GetInstance() {
  static base::NoDestructor<AdblockSitekeyStorageFactory> instance;
  return instance.get();
}

AdblockSitekeyStorageFactory::AdblockSitekeyStorageFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSitekeyStorage",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AdblockPlatformEmbedderFactory::GetInstance());
}

AdblockSitekeyStorageFactory::~AdblockSitekeyStorageFactory() = default;

KeyedService* AdblockSitekeyStorageFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AdblockSitekeyStorageLibabpImpl(
      AdblockPlatformEmbedderFactory::GetForBrowserContext(context));
}

content::BrowserContext* AdblockSitekeyStorageFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

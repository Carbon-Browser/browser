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

#include "chrome/browser/adblock/adblock_element_hider_factory.h"

#include <memory>

#include "chrome/browser/adblock/adblock_platform_embedder_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/adblock_element_hider_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
AdblockElementHider* AdblockElementHiderFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockElementHider*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
AdblockElementHiderFactory* AdblockElementHiderFactory::GetInstance() {
  static base::NoDestructor<AdblockElementHiderFactory> instance;
  return instance.get();
}

AdblockElementHiderFactory::AdblockElementHiderFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockElementHider",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AdblockPlatformEmbedderFactory::GetInstance());
}

AdblockElementHiderFactory::~AdblockElementHiderFactory() = default;

KeyedService* AdblockElementHiderFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AdblockElementHiderImpl(
      AdblockPlatformEmbedderFactory::GetForBrowserContext(context));
}

content::BrowserContext* AdblockElementHiderFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

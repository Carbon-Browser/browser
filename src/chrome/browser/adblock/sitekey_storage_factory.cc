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

#include "chrome/browser/adblock/sitekey_storage_factory.h"

#include <memory>

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/core/sitekey_storage_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
SitekeyStorage* SitekeyStorageFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SitekeyStorage*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
SitekeyStorageFactory* SitekeyStorageFactory::GetInstance() {
  static base::NoDestructor<SitekeyStorageFactory> instance;
  return instance.get();
}

SitekeyStorageFactory::SitekeyStorageFactory()
    : BrowserContextKeyedServiceFactory(
          "SitekeyStorage",
          BrowserContextDependencyManager::GetInstance()) {}

SitekeyStorageFactory::~SitekeyStorageFactory() = default;

KeyedService* SitekeyStorageFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SitekeyStorageImpl();
}

content::BrowserContext* SitekeyStorageFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

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

#include "chrome/browser/adblock/subscription_persistent_metadata_factory.h"

#include <memory>

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/core/subscription/subscription_persistent_metadata_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
SubscriptionPersistentMetadata*
SubscriptionPersistentMetadataFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SubscriptionPersistentMetadata*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
SubscriptionPersistentMetadataFactory*
SubscriptionPersistentMetadataFactory::GetInstance() {
  static base::NoDestructor<SubscriptionPersistentMetadataFactory> instance;
  return instance.get();
}

SubscriptionPersistentMetadataFactory::SubscriptionPersistentMetadataFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSubscriptionPersistentMetadata",
          BrowserContextDependencyManager::GetInstance()) {}
SubscriptionPersistentMetadataFactory::
    ~SubscriptionPersistentMetadataFactory() = default;

KeyedService* SubscriptionPersistentMetadataFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SubscriptionPersistentMetadataImpl(
      Profile::FromBrowserContext(context)->GetPrefs());
}

content::BrowserContext*
SubscriptionPersistentMetadataFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

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

#include "chrome/browser/adblock/subscription_updater_factory.h"

#include <memory>

#include "chrome/browser/adblock/subscription_persistent_metadata_factory.h"
#include "chrome/browser/adblock/subscription_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/core/subscription/subscription_updater_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {
namespace {
constexpr auto kInitialDelay = base::Seconds(30);
constexpr auto kCheckInterval = base::Hours(1);
}  // namespace

// static
SubscriptionUpdater* SubscriptionUpdaterFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SubscriptionUpdater*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
SubscriptionUpdaterFactory* SubscriptionUpdaterFactory::GetInstance() {
  static base::NoDestructor<SubscriptionUpdaterFactory> instance;
  return instance.get();
}

SubscriptionUpdaterFactory::SubscriptionUpdaterFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSubscriptionUpdater",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SubscriptionServiceFactory::GetInstance());
  DependsOn(SubscriptionPersistentMetadataFactory::GetInstance());
}
SubscriptionUpdaterFactory::~SubscriptionUpdaterFactory() = default;

KeyedService* SubscriptionUpdaterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto service = std::make_unique<SubscriptionUpdaterImpl>(
      Profile::FromBrowserContext(context)->GetPrefs(),
      SubscriptionServiceFactory::GetForBrowserContext(context),
      SubscriptionPersistentMetadataFactory::GetForBrowserContext(context),
      kInitialDelay, kCheckInterval);
  service->StartSchedule();
  return service.release();
}

content::BrowserContext* SubscriptionUpdaterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool SubscriptionUpdaterFactory::ServiceIsCreatedWithBrowserContext() const {
  // Create the service ASAP via the factory framework to avoid patching
  // chrome_browser_main.cc with pointless GetForBrowserContext() call.
  return true;
}

bool SubscriptionUpdaterFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace adblock

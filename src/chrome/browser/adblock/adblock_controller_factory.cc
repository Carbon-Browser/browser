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

#include "chrome/browser/adblock/adblock_controller_factory.h"

#include <memory>

#include "chrome/browser/adblock/subscription_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/core/adblock_controller_impl.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
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
  DependsOn(SubscriptionServiceFactory::GetInstance());
}

AdblockControllerFactory::~AdblockControllerFactory() = default;

KeyedService* AdblockControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto controller = std::make_unique<AdblockControllerImpl>(
      Profile::FromBrowserContext(context)->GetPrefs(),
      SubscriptionServiceFactory::GetForBrowserContext(context),
      g_browser_process->GetApplicationLocale());
  controller->SynchronizeWithPrefsWhenPossible();
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

}  // namespace adblock

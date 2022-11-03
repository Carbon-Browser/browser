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

#include "chrome/browser/adblock/adblock_telemetry_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/adblock/adblock_telemetry_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace adblock {

// static
AdblockTelemetryService* AdblockTelemetryServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AdblockTelemetryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
// static
AdblockTelemetryServiceFactory* AdblockTelemetryServiceFactory::GetInstance() {
  static base::NoDestructor<AdblockTelemetryServiceFactory> instance;
  return instance.get();
}

AdblockTelemetryServiceFactory::AdblockTelemetryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockTelemetryService",
          BrowserContextDependencyManager::GetInstance()) {}

AdblockTelemetryServiceFactory::~AdblockTelemetryServiceFactory() {}

KeyedService* AdblockTelemetryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<AdblockTelemetryService> service =
      std::make_unique<AdblockTelemetryService>(
          Profile::FromBrowserContext(context)->GetPrefs(),
          g_browser_process->system_network_context_manager()
              ->GetSharedURLLoaderFactory());
  service->Start();
  return static_cast<KeyedService*>(service.release());
}

content::BrowserContext* AdblockTelemetryServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool AdblockTelemetryServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

void AdblockTelemetryServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  adblock::AdblockTelemetryService::RegisterProfilePrefs(registry);
}

bool AdblockTelemetryServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  // This avoids manual instantiation in chrome_browser_main.cc
#if defined(ABP_TELEMETRY_CLIENT_ID)
  return true;
#else
  return false;
#endif
}

}  // namespace adblock

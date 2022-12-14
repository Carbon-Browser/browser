// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/profile_network_context_service_factory.h"

#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/certificate_provider/certificate_provider_service_factory.h"
#endif

ProfileNetworkContextService*
ProfileNetworkContextServiceFactory::GetForContext(
    content::BrowserContext* browser_context) {
  return static_cast<ProfileNetworkContextService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

ProfileNetworkContextServiceFactory*
ProfileNetworkContextServiceFactory::GetInstance() {
  return base::Singleton<ProfileNetworkContextServiceFactory>::get();
}

ProfileNetworkContextServiceFactory::ProfileNetworkContextServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ProfileNetworkContextService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  DependsOn(chromeos::CertificateProviderServiceFactory::GetInstance());
#endif
  DependsOn(PrivacySandboxSettingsFactory::GetInstance());
}

ProfileNetworkContextServiceFactory::~ProfileNetworkContextServiceFactory() {}

KeyedService* ProfileNetworkContextServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new ProfileNetworkContextService(Profile::FromBrowserContext(profile));
}

content::BrowserContext*
ProfileNetworkContextServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Create separate service for incognito profiles.
  return context;
}

bool ProfileNetworkContextServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

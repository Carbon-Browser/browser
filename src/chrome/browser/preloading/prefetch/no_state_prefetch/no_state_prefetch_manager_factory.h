// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRELOADING_PREFETCH_NO_STATE_PREFETCH_NO_STATE_PREFETCH_MANAGER_FACTORY_H_
#define CHROME_BROWSER_PRELOADING_PREFETCH_NO_STATE_PREFETCH_NO_STATE_PREFETCH_MANAGER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

class Profile;

namespace prerender {

class NoStatePrefetchManager;

// Singleton that owns all NoStatePrefetchManagers and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated NoStatePrefetchManager.
class NoStatePrefetchManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the NoStatePrefetchManager for |context|.
  static NoStatePrefetchManager* GetForBrowserContext(
      content::BrowserContext* context);

  static NoStatePrefetchManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<NoStatePrefetchManagerFactory>;

  NoStatePrefetchManagerFactory();
  ~NoStatePrefetchManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace prerender

#endif  // CHROME_BROWSER_PRELOADING_PREFETCH_NO_STATE_PREFETCH_NO_STATE_PREFETCH_MANAGER_FACTORY_H_

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

#include "chrome/browser/adblock/adblock_platform_embedder_factory.h"

#include <memory>

#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "components/adblock/adblock_platform_embedder_impl.h"
#include "components/adblock/adblock_utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace adblock {

// static
scoped_refptr<AdblockPlatformEmbedder>
AdblockPlatformEmbedderFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockPlatformEmbedder*>(
      GetInstance()->GetServiceForBrowserContext(context, true).get());
}
// static
AdblockPlatformEmbedderFactory* AdblockPlatformEmbedderFactory::GetInstance() {
  static base::NoDestructor<AdblockPlatformEmbedderFactory> instance;
  return instance.get();
}

AdblockPlatformEmbedderFactory::AdblockPlatformEmbedderFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
          "AdblockPlatformEmbedder",
          BrowserContextDependencyManager::GetInstance()) {}

AdblockPlatformEmbedderFactory::~AdblockPlatformEmbedderFactory() = default;

scoped_refptr<RefcountedKeyedService>
AdblockPlatformEmbedderFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  const std::string locale =
      g_browser_process ? g_browser_process->GetApplicationLocale() : "";
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      g_browser_process && g_browser_process->system_network_context_manager()
          ? g_browser_process->system_network_context_manager()
                ->GetSharedURLLoaderFactory()
          : nullptr;

  base::FilePath storage_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &storage_dir);

  scoped_refptr<adblock::AdblockPlatformEmbedder> embedder{
      base::MakeRefCounted<adblock::AdblockPlatformEmbedderImpl>(
          utils::CreateABPTaskRunner(), base::ThreadTaskRunnerHandle::Get(),
          url_loader_factory, locale, std::move(storage_dir),
          Profile::FromBrowserContext(context)->GetPrefs())};
  return embedder;
}

content::BrowserContext* AdblockPlatformEmbedderFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

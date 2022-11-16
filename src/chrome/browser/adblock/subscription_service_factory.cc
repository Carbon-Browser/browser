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

#include "chrome/browser/adblock/subscription_service_factory.h"

#include <fstream>
#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/adblock/subscription_persistent_metadata_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/converter/converter.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/ongoing_subscription_request_impl.h"
#include "components/adblock/core/subscription/preloaded_subscription_provider_impl.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock/core/subscription/subscription_downloader_impl.h"
#include "components/adblock/core/subscription/subscription_persistent_storage_impl.h"
#include "components/adblock/core/subscription/subscription_service_impl.h"
#include "components/adblock/core/subscription/subscription_validator_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace adblock {
namespace {

constexpr net::BackoffEntry::Policy kRetryBackoffPolicy = {
    0,               // Number of initial errors to ignore.
    5000,            // Initial delay in ms.
    2.0,             // Factor by which the waiting time will be multiplied.
    0.2,             // Fuzzing percentage.
    60 * 60 * 1000,  // Maximum delay in ms.
    -1,              // Never discard the entry.
    false,           // Use initial delay.
};

std::unique_ptr<OngoingSubscriptionRequest> MakeOngoingSubscriptionRequest(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  return std::make_unique<OngoingSubscriptionRequestImpl>(
      prefs, &kRetryBackoffPolicy, url_loader_factory);
}

ConverterResult ConvertFileToFlatbuffer(const GURL& subscription_url,
                                        const base::FilePath& path) {
  TRACE_EVENT1("eyeo", "ConvertFileToFlatbuffer", "url",
               subscription_url.spec());
  std::ifstream input_stream(path.AsUTF8Unsafe());
  if (!input_stream.is_open() || !input_stream.good()) {
    ConverterResult result;
    result.status = ConverterResult::Error;
    return result;
  }
  return Converter().Convert(
      input_stream,
      {subscription_url, config::AllowPrivilegedFilters(subscription_url)});
}

scoped_refptr<InstalledSubscription> CustomFilterConverter(
    const std::vector<std::string>& filters) {
  auto raw_data = Converter().Convert(filters, {CustomFiltersUrl(), true});
  return base::MakeRefCounted<InstalledSubscriptionImpl>(
      std::move(raw_data), Subscription::InstallationState::Installed,
      base::Time());
}

}  // namespace

// static
SubscriptionService* SubscriptionServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SubscriptionService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}
// static
SubscriptionServiceFactory* SubscriptionServiceFactory::GetInstance() {
  static base::NoDestructor<SubscriptionServiceFactory> instance;
  return instance.get();
}

SubscriptionServiceFactory::SubscriptionServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSubscriptionService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SubscriptionPersistentMetadataFactory::GetInstance());
}
SubscriptionServiceFactory::~SubscriptionServiceFactory() = default;

KeyedService* SubscriptionServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* prefs = Profile::FromBrowserContext(context)->GetPrefs();
  auto main_thread_task_runner = base::SequencedTaskRunnerHandle::Get();
  auto* persistent_metadata =
      SubscriptionPersistentMetadataFactory::GetForBrowserContext(context);
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      g_browser_process && g_browser_process->system_network_context_manager()
          ? g_browser_process->system_network_context_manager()
                ->GetSharedURLLoaderFactory()
          : nullptr;

  auto storage = std::make_unique<SubscriptionPersistentStorageImpl>(
      context->GetPath().AppendASCII("adblock_subscriptions"),
      base::MakeRefCounted<SubscriptionValidatorImpl>(
          prefs, CurrentSchemaVersion(), main_thread_task_runner),
      persistent_metadata);
  auto downloader = std::make_unique<SubscriptionDownloaderImpl>(
      utils::GetAppInfo(),
      base::BindRepeating(&MakeOngoingSubscriptionRequest,
                          Profile::FromBrowserContext(context)->GetPrefs(),
                          url_loader_factory),
      base::BindRepeating(&ConvertFileToFlatbuffer), persistent_metadata);

  return new SubscriptionServiceImpl(
      prefs, std::move(storage), std::move(downloader),
      std::make_unique<PreloadedSubscriptionProviderImpl>(prefs),
      base::BindRepeating(&CustomFilterConverter), persistent_metadata);
}

content::BrowserContext* SubscriptionServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace adblock

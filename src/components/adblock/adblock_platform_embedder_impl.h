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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_IMPL_H_

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

#include <memory>
#include <string>

namespace AdblockPlus {
struct IV8IsolateProvider;
}  // namespace AdblockPlus

class PrefService;

namespace adblock {

class AdblockPlatformEmbedderImpl final
    : public AdblockPlatformEmbedder,
      public AdblockPlus::IFilterEngine::EventObserver {
 public:
  AdblockPlatformEmbedderImpl(
      scoped_refptr<utils::TaskRunnerWrapper> abp_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
      const std::string& locale,
      base::FilePath storage_dir,
      PrefService* pref_service);

  // RefcountedKeyedService overrides:
  void ShutdownOnUIThread() final;

  // AdblockPlatformEmbedderImpl overrides:
  void AddObserver(AdblockPlatformEmbedder::Observer* observer) final;
  void RemoveObserver(AdblockPlatformEmbedder::Observer* observer) final;
  void CreateFilterEngine(AdblockController* config) final;
  AdblockPlus::IFilterEngine* GetFilterEngine() final;
  AdblockPlus::Platform* GetPlatform() const final;
  scoped_refptr<base::SingleThreadTaskRunner> GetAbpTaskRunner() final;
  void OnSubscriptionEvent(AdblockPlus::IFilterEngine::SubscriptionEvent event,
                           const AdblockPlus::Subscription& subscription) final;
  void RunAfterFilterEngineCreated(base::OnceClosure) final;
  void ComposeFilterSuggestions(
      std::unique_ptr<AdblockPlus::IElement> element,
      base::OnceCallback<void(const std::vector<std::string>&)> results) final;

  void SetFilterEngine(AdblockPlus::IFilterEngine* engine);

 private:
  struct FilterEngineStartConfig {
    AllowedConnectionType allowed_connection_type{AllowedConnectionType::kWiFi};
    bool acceptable_ads_enabled{true};
  };

  ~AdblockPlatformEmbedderImpl() final;
  void SetUp(const FilterEngineStartConfig& config);
  void OnFilterEngineCreated(const AdblockPlus::IFilterEngine& engine);
  void NotifyEngineCreatedOnUI(const std::vector<Subscription>& recommended,
                               const std::vector<GURL>& listed,
                               const GURL& acceptable_ads);
  void NotifySubscriptionDownloadedOnUI(
      const AdblockPlus::Subscription& subscription);
  void FinishSetUpOnBG(const FilterEngineStartConfig& config);
  void PostFinishSetUpTask(const FilterEngineStartConfig& config);

  friend base::RefCountedThreadSafe<AdblockPlatformEmbedderImpl>;
  std::unique_ptr<AdblockPlus::Platform> platform_;
  base::Lock filter_engine_lock_;
  AdblockPlus::IFilterEngine* filter_engine_{nullptr};
  scoped_refptr<utils::TaskRunnerWrapper> abp_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_runner_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  std::vector<base::OnceClosure> pending_jobs_;
  std::string locale_;
  const base::FilePath storage_dir_;
  PrefService* pref_service_{nullptr};
  base::Time filter_engine_start_time_;
  base::ObserverList<AdblockPlatformEmbedder::Observer> observers_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_IMPL_H_

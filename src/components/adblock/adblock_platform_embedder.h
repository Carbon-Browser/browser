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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list_types.h"
#include "base/single_thread_task_runner.h"
#include "base/types/strong_alias.h"
#include "components/adblock/allowed_connection_type.h"
#include "components/adblock/subscription.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"
#include "url/gurl.h"

namespace AdblockPlus {
class Platform;
}  // namespace AdblockPlus

namespace adblock {

class AdblockController;
/**
 * @brief Owns the Platform, FilterEngine and an ABP task runner
 * that is responsible to call FilterEngine in the correct thread.
 * Responsible for initializing the V8 instance in which the Platform
 * and FilterEngine operate, and for starting up the FilterEngine.
 * Shared between UI and ABP threads.
 */
class AdblockPlatformEmbedder : public RefcountedKeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // All methods called on UI thread
    virtual void OnFilterEngineCreated(
        const std::vector<Subscription>& recommended,
        const std::vector<GURL>& listed,
        const GURL& acceptable_ads) = 0;
    // TODO move subscription update notifications to a core-agnostic interface.
    virtual void OnSubscriptionUpdated(const GURL& url) = 0;
  };

  explicit AdblockPlatformEmbedder(
      scoped_refptr<base::SequencedTaskRunner> task_runner_for_deletion);

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Start Filter engine creation which is asynchronous operation.
  // Notifies via Observer::OnFilterEngineCreated() about completion.
  virtual void CreateFilterEngine(AdblockController* config) = 0;
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetAbpTaskRunner() = 0;

  // Returns Filter engine. Will return nullptr till
  // Observer::OnFilterEngineCreated() is notified.
  // Is thread-safe.
  virtual AdblockPlus::IFilterEngine* GetFilterEngine() = 0;

  virtual AdblockPlus::Platform* GetPlatform() const = 0;

  // Lets clients schedule a deferred task that will execute once
  // |GetFilterEngine| returns a non-nullptr IFilterEngine.
  virtual void RunAfterFilterEngineCreated(base::OnceClosure) = 0;

  // TODO move this to somewhere that's not libadblockplus-specific. Solve
  // how to represent IElement in a core-agnostic way.
  virtual void ComposeFilterSuggestions(
      std::unique_ptr<AdblockPlus::IElement> element,
      base::OnceCallback<void(const std::vector<std::string>&)> results) = 0;

 protected:
  friend base::RefCountedThreadSafe<AdblockPlatformEmbedder>;
  ~AdblockPlatformEmbedder() override = default;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_PLATFORM_EMBEDDER_H_

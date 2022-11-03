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

#ifndef COMPONENTS_ADBLOCK_MOCK_ADBLOCK_PLATFORM_EMBEDDER_H_
#define COMPONENTS_ADBLOCK_MOCK_ADBLOCK_PLATFORM_EMBEDDER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/observer_list.h"
#include "components/adblock/adblock_platform_embedder.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace adblock {

class MockFilterEngine;

class MockAdblockPlatformEmbedder : public AdblockPlatformEmbedder {
 public:
  MockAdblockPlatformEmbedder(
      std::unique_ptr<MockFilterEngine> engine,
      scoped_refptr<base::SingleThreadTaskRunner> abp_runner);
  void ShutdownOnUIThread() override {}
  void AddObserver(AdblockPlatformEmbedder::Observer* observer) override;
  void RemoveObserver(AdblockPlatformEmbedder::Observer* observer) override;
  void CreateFilterEngine(AdblockController* config) override;
  scoped_refptr<base::SingleThreadTaskRunner> GetAbpTaskRunner() override;
  AdblockPlus::IFilterEngine* GetFilterEngine() override;
  AdblockPlus::Platform* GetPlatform() const override;
  void RunAfterFilterEngineCreated(base::OnceClosure) override;
  void ComposeFilterSuggestions(
      std::unique_ptr<AdblockPlus::IElement> element,
      base::OnceCallback<void(const std::vector<std::string>&)> results)
      override;
  void NotifyOnEngineCreated();
  void SetFilterEngine(std::unique_ptr<MockFilterEngine> engine);
  std::unique_ptr<MockFilterEngine> TakeFilterEngine();

 private:
  friend base::RefCountedThreadSafe<MockFilterEngine>;
  ~MockAdblockPlatformEmbedder() override;

  scoped_refptr<base::SingleThreadTaskRunner> abp_runner_;
  base::ObserverList<AdblockPlatformEmbedder::Observer> observers_;
  std::vector<base::OnceClosure> deferred_jobs_;
  std::unique_ptr<MockFilterEngine> engine_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_MOCK_ADBLOCK_PLATFORM_EMBEDDER_H_

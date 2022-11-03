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
#include "components/adblock/mock_adblock_platform_embedder.h"

#include "components/adblock/mock_filter_engine.h"

namespace adblock {

MockAdblockPlatformEmbedder::MockAdblockPlatformEmbedder(
    std::unique_ptr<MockFilterEngine> engine,
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner)
    : AdblockPlatformEmbedder(abp_runner),
      abp_runner_(abp_runner),
      engine_(std::move(engine)) {}

MockAdblockPlatformEmbedder::~MockAdblockPlatformEmbedder() = default;

void MockAdblockPlatformEmbedder::AddObserver(
    AdblockPlatformEmbedder::Observer* observer) {
  observers_.AddObserver(observer);
}

void MockAdblockPlatformEmbedder::RemoveObserver(
    AdblockPlatformEmbedder::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MockAdblockPlatformEmbedder::NotifyOnEngineCreated() {
  auto subscritions = engine_->GetListedSubscriptions();
  std::vector<GURL> listed;
  for (const auto& subscription : subscritions) {
    listed.push_back(GURL(subscription.GetUrl()));
  }

  auto recommended = engine_->FetchAvailableSubscriptions();
  GURL acceptable_ads(engine_->GetAAUrl());

  std::vector<Subscription> recommended_converted;
  for (auto& subscription : recommended) {
    auto title = subscription.GetTitle();
    auto languages = subscription.GetLanguages();
    auto url = subscription.GetUrl();

    recommended_converted.emplace_back(
        Subscription{GURL(url), title, languages});
  }

  for (auto& observer : observers_) {
    observer.OnFilterEngineCreated(recommended_converted, listed,
                                   acceptable_ads);
  }

  for (auto& job : deferred_jobs_)
    std::move(job).Run();
  deferred_jobs_.clear();
}

void MockAdblockPlatformEmbedder::CreateFilterEngine(
    AdblockController* config) {}

scoped_refptr<base::SingleThreadTaskRunner>
MockAdblockPlatformEmbedder::GetAbpTaskRunner() {
  return abp_runner_;
}

AdblockPlus::IFilterEngine* MockAdblockPlatformEmbedder::GetFilterEngine() {
  return engine_.get();
}

AdblockPlus::Platform* MockAdblockPlatformEmbedder::GetPlatform() const {
  // Only Network Delegate functionality uses that and it is tested separately
  // and used only with the real ABP Network implementation so we should be fine
  // with this nullptr here.
  return nullptr;
}

void MockAdblockPlatformEmbedder::RunAfterFilterEngineCreated(
    base::OnceClosure job) {
  deferred_jobs_.push_back(std::move(job));
}

void MockAdblockPlatformEmbedder::ComposeFilterSuggestions(
    std::unique_ptr<AdblockPlus::IElement> element,
    base::OnceCallback<void(const std::vector<std::string>&)> results) {}

void MockAdblockPlatformEmbedder::SetFilterEngine(
    std::unique_ptr<MockFilterEngine> engine) {
  engine_ = std::move(engine);
}

std::unique_ptr<MockFilterEngine>
MockAdblockPlatformEmbedder::TakeFilterEngine() {
  return std::move(engine_);
}

}  // namespace adblock

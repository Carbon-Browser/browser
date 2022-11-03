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

#include "components/adblock/adblock_resource_reader_impl.h"

#include "components/adblock/buildflags.h"

#if BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)
#include "base/logging.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "components/adblock/adblock_utils.h"
#include "components/grit/components_resources.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/resource/resource_bundle.h"
#endif  // BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)

namespace adblock {

#if BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)

std::unique_ptr<AdblockPlus::StringPreloadedFilterResponse>
ReadResourceInBackground(const std::string& url) {
  // By default use the English EasyList unless the exception rules is requested
  int resource_id = -1;
  if (url == "https://easylist-downloads.adblockplus.org/exceptionrules.txt") {
    resource_id = IDR_ADBLOCK_BUNDLED_EXCEPTIONRULES;
  } else if (base::EndsWith(url, "easylist.txt",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    resource_id = IDR_ADBLOCK_BUNDLED_EASYLIST;
  }

  if (resource_id != -1) {
    std::string content =
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            resource_id);

    DLOG(INFO) << "[ABP] Bundled subscription content size: " << content.size();

    return std::make_unique<AdblockPlus::StringPreloadedFilterResponse>(
        std::move(content));
  }

  DLOG(INFO) << "[ABP] Bundled subscription not found";

  return std::make_unique<AdblockPlus::StringPreloadedFilterResponse>();
}

void NotifyResourceRead(
    const AdblockPlus::IResourceReader::ReadCallback& done_callback,
    std::unique_ptr<AdblockPlus::StringPreloadedFilterResponse> data) {
  done_callback(std::move(data));
}

void OnResourceRead(
    const std::string& url,
    const AdblockPlus::IResourceReader::ReadCallback& done_callback,
    std::unique_ptr<AdblockPlus::StringPreloadedFilterResponse> data) {
  utils::BenchmarkOperation(
      "Notification after resource read for " + url,
      base::BindOnce(&NotifyResourceRead, done_callback, std::move(data)));
}

#endif  // BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)

AdblockResourceReaderImpl::AdblockResourceReaderImpl(
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner)
    : abp_runner_(abp_runner) {}

AdblockResourceReaderImpl::AdblockResourceReaderImpl(
    const AdblockResourceReaderImpl& other) = default;

AdblockResourceReaderImpl::AdblockResourceReaderImpl(
    AdblockResourceReaderImpl&& other) = default;

AdblockResourceReaderImpl::~AdblockResourceReaderImpl() = default;

void AdblockResourceReaderImpl::ReadPreloadedFilterList(
    const std::string& url,
    const ReadCallback& done_callback) const {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implementations provided to libabp have to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

#if BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)
  VLOG(1) << "[ABP] Resource " << url;

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&ReadResourceInBackground, url),
      base::BindOnce(&OnResourceRead, url, done_callback));
#else
  done_callback(std::make_unique<AdblockPlus::StringPreloadedFilterResponse>());
#endif  // BUILDFLAG(ENABLE_BUNDLED_SUBSCRIPTIONS)
}

}  // namespace adblock

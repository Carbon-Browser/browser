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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_RESOURCE_READER_IMPL_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_RESOURCE_READER_IMPL_H_

#include "third_party/libadblockplus/src/include/AdblockPlus/IResourceReader.h"

#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"

namespace adblock {
/**
 * @brief Extends ResourceReader to optionally provide preloaded subscriptions.
 * Preloaded subscriptions are filter lists bundled with the browser. They are
 * likely relatively old by the time the browser is installed on the end-user's
 * machine and bundling them increases the size of the installation package, but
 * they are available immediately and don't require deferring ad-filtering,
 * waiting for an initial download.
 */
class AdblockResourceReaderImpl final : public AdblockPlus::IResourceReader {
 public:
  explicit AdblockResourceReaderImpl(
      scoped_refptr<base::SingleThreadTaskRunner> abp_runner);
  AdblockResourceReaderImpl(const AdblockResourceReaderImpl& other);
  AdblockResourceReaderImpl(AdblockResourceReaderImpl&& other);
  ~AdblockResourceReaderImpl() final;

  void ReadPreloadedFilterList(const std::string& url,
                               const ReadCallback& done_callback) const final;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> abp_runner_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_RESOURCE_READER_IMPL_H_

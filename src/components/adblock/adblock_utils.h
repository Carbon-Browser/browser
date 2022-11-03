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

#ifndef COMPONENTS_ADBLOCK_ADBLOCK_UTILS_H_
#define COMPONENTS_ADBLOCK_ADBLOCK_UTILS_H_

#include <string>

#include "base/callback_forward.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterEngine.h"

class GURL;

namespace net {
class HttpResponseHeaders;
}

namespace adblock {
namespace utils {

struct AppInfo {
  AppInfo();
  ~AppInfo();
  AppInfo(const AppInfo&);
  std::string name;
  std::string version;
};

// A wrapper for SingleThreadTaskRunner that adds:
// - logging long-lasting operations for performance analysis,
// - ability to disallow execution to avoid race conditions during shutdown.
class TaskRunnerWrapper : public base::SingleThreadTaskRunner {
 public:
  explicit TaskRunnerWrapper(
      scoped_refptr<base::SingleThreadTaskRunner> wrapee);

  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;

  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;

  bool RunsTasksInCurrentSequence() const override;

  // Once called, no posted tasks will execute on this runner.
  void DisallowExecution();

 private:
  ~TaskRunnerWrapper() override;
  void RunTaskWrapper(const base::Location& from_here, base::OnceClosure task);

  bool execution_allowed_ = true;
  scoped_refptr<base::SingleThreadTaskRunner> wrapee_;
};

AdblockPlus::IFilterEngine::ContentType ConvertToAdblockResourceType(
    const GURL& url,
    int32_t resource_type);

std::string CreateDomainAllowlistingFilter(const std::string& domain);

scoped_refptr<TaskRunnerWrapper> CreateABPTaskRunner();

void BenchmarkOperation(const std::string& description, base::OnceClosure op);

std::string GetSitekeyHeader(
    const scoped_refptr<net::HttpResponseHeaders>& headers);

AppInfo GetAppInfo();

std::string SerializeLanguages(const std::vector<std::string> languages);

std::vector<std::string> DeserializeLanguages(const std::string languages);

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input);

}  // namespace utils
}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_ADBLOCK_UTILS_H_

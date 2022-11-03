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
#include "components/adblock/adblock_utils.h"

#include <numeric>

#include "base/containers/flat_map.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "components/adblock/adblock_constants.h"
#include "components/version_info/version_info.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace adblock {
namespace utils {

namespace {

constexpr char kLanguagesSeparator[] = ",";

}  // namespace

TaskRunnerWrapper::TaskRunnerWrapper(
    scoped_refptr<base::SingleThreadTaskRunner> wrapee)
    : wrapee_(wrapee) {}

bool TaskRunnerWrapper::PostDelayedTask(const base::Location& from_here,
                                        base::OnceClosure task,
                                        base::TimeDelta delay) {
  return wrapee_->PostDelayedTask(
      from_here,
      base::BindOnce(&TaskRunnerWrapper::RunTaskWrapper, this, from_here,
                     std::move(task)),
      delay);
}

bool TaskRunnerWrapper::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  return wrapee_->PostNonNestableDelayedTask(
      from_here,
      base::BindOnce(&TaskRunnerWrapper::RunTaskWrapper, this, from_here,
                     std::move(task)),
      delay);
}

bool TaskRunnerWrapper::RunsTasksInCurrentSequence() const {
  return wrapee_->RunsTasksInCurrentSequence();
}

void TaskRunnerWrapper::DisallowExecution() {
  DCHECK(wrapee_->RunsTasksInCurrentSequence());
  execution_allowed_ = false;
}

TaskRunnerWrapper::~TaskRunnerWrapper() = default;

void TaskRunnerWrapper::RunTaskWrapper(const base::Location& from_here,
                                       base::OnceClosure task) {
  DCHECK(wrapee_->RunsTasksInCurrentSequence());
  if (!execution_allowed_)
    return;
  BenchmarkOperation("Task posted from " + from_here.ToString(),
                     std::move(task));
}

AdblockPlus::IFilterEngine::ContentType DetectResourceType(const GURL& url) {
  static base::flat_map<std::string, AdblockPlus::IFilterEngine::ContentType>
      content_type_map(
          {// javascript
           {".js",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_SCRIPT},
           // css
           {".css",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_STYLESHEET},
           // image filename
           {".gif",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".apng",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".png",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".jpe",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".jpg",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".jpeg",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".bmp",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".ico",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           {".webp",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE},
           // fonts
           {".ttf", AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_FONT},
           {".woff",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_FONT},
           // document ( html )
           {".html",
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_SUBDOCUMENT},
           {".htm", AdblockPlus::IFilterEngine::ContentType::
                        CONTENT_TYPE_SUBDOCUMENT}});

  const std::string file_name(url.ExtractFileName());

  // get file extension
  const size_t pos = file_name.find_last_of('.');
  if (pos == std::string::npos) {
    return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_OTHER;
  }
  const std::string file_extension(base::ToLowerASCII(file_name.substr(pos)));

  const auto content_type = content_type_map.find(file_extension);

  if (content_type_map.end() == content_type) {
    return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_OTHER;
  }
  return content_type->second;
}

AdblockPlus::IFilterEngine::ContentType ConvertToAdblockResourceType(
    const GURL& url,
    int32_t resource_type) {
  if (resource_type <
      static_cast<int32_t>(blink::mojom::ResourceType::kMinValue))
    return DetectResourceType(url);
  if (resource_type >
      static_cast<int32_t>(blink::mojom::ResourceType::kMaxValue))
    return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_OTHER;

  const auto blink_resource_type =
      static_cast<blink::mojom::ResourceType>(resource_type);
  switch (blink_resource_type) {
    case blink::mojom::ResourceType::kMainFrame:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_GENERICBLOCK;

    case blink::mojom::ResourceType::kImage:
    case blink::mojom::ResourceType::kFavicon:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_IMAGE;

    case blink::mojom::ResourceType::kXhr:
      return AdblockPlus::IFilterEngine::ContentType::
          CONTENT_TYPE_XMLHTTPREQUEST;

    case blink::mojom::ResourceType::kStylesheet:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_STYLESHEET;

    case blink::mojom::ResourceType::kScript:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_SCRIPT;

    case blink::mojom::ResourceType::kFontResource:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_FONT;

    case blink::mojom::ResourceType::kObject:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_OBJECT;

    case blink::mojom::ResourceType::kSubFrame:
      // though subframe is a visual element we will elemhide it later
      // (see DP-617 for details)
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_SUBDOCUMENT;

    case blink::mojom::ResourceType::kMedia:
      return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_MEDIA;

    default:
      break;
  }

  return AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_OTHER;
}

std::string CreateDomainAllowlistingFilter(const std::string& domain) {
  return "@@||" + domain + "^$document,domain=" + domain;
}

scoped_refptr<TaskRunnerWrapper> CreateABPTaskRunner() {
  return base::MakeRefCounted<TaskRunnerWrapper>(
      base::ThreadPool::CreateSingleThreadTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
          base::SingleThreadTaskRunnerThreadMode::DEDICATED));
}

void BenchmarkOperation(const std::string& description, base::OnceClosure op) {
  constexpr int kWarnLongLastingOperationThresholdMs = 500;
  auto start = base::Time::Now();
  std::move(op).Run();
  auto lasted = base::Time::Now() - start;
  if (lasted >
      base::TimeDelta::FromMilliseconds(kWarnLongLastingOperationThresholdMs)) {
    LOG(WARNING) << "[ABP] " << description << " took "
                 << lasted.InMillisecondsF() << " ms";
  } else {
    VLOG(1) << "[ABP] " << description << " took " << lasted.InMillisecondsF()
            << " ms";
  }
}

std::string GetSitekeyHeader(
    const scoped_refptr<net::HttpResponseHeaders>& headers) {
  size_t iterator = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&iterator, &name, &value)) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (name == adblock::kSiteKeyHeaderKey) {
      return value;
    }
  }
  return {};
}

AppInfo::AppInfo() = default;

AppInfo::~AppInfo() = default;

AppInfo::AppInfo(const AppInfo&) = default;

AppInfo GetAppInfo() {
  AppInfo info;

#if defined(ABP_APPLICATION_NAME)
  info.name = ABP_APPLICATION_NAME;
#else
  info.name = version_info::GetProductName();
#endif
#if defined(ABP_APPLICATION_VERSION)
  info.version = ABP_APPLICATION_VERSION;
#else
  info.version = version_info::GetVersionNumber();
#endif

  return info;
}

std::string SerializeLanguages(const std::vector<std::string> languages) {
  if (languages.empty())
    return {};

  return std::accumulate(std::next(languages.begin()), languages.end(),
                         languages[0],
                         [](const std::string& a, const std::string& b) {
                           return a + kLanguagesSeparator + b;
                         });
}

std::vector<std::string> DeserializeLanguages(const std::string languages) {
  return base::SplitString(languages, kLanguagesSeparator,
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

std::vector<std::string> ConvertURLs(const std::vector<GURL>& input) {
  std::vector<std::string> output;
  output.reserve(input.size());
  std::transform(std::begin(input), std::end(input), std::back_inserter(output),
                 [](const GURL& gurl) { return gurl.spec(); });
  return output;
}

}  // namespace utils
}  // namespace adblock

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

#include "components/adblock/adblock_network_impl.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "components/adblock/adblock_utils.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace adblock {

using AdblockPlus::IWebRequest;

namespace {

std::string ReadDownloadedFile(const base::FilePath& path) {
  std::string content;
  base::ReadFileToString(path, &content);
  return content;
}

constexpr int64_t kMaxDownloadSize = 100 * 1024 * 1024;  // 100 Mb.
constexpr int kMaxRetries = 5;

void OnDownloadedFileRead(
    const GURL& url,
    AdblockPlus::ServerResponse&& server_response,
    base::OnceCallback<void(const AdblockPlus::ServerResponse&)> callback,
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
    const std::string& response) {
  server_response.responseText = response;
  VLOG(1) << "[ABP] Response for " << url.spec()
          << " (length): " << server_response.responseText.size();

  // This ends up called on the UI yet we do not want to send notification
  // on it in order to be on the safe side if the callback does something
  // expensive. This is also why AdblockNetworkImpl::GET() checks which thread
  // it's on - because the callback can trigger new GET()s from the
  // |abp_runner_| but network related object can only be accessed on UI.
  auto closure = base::BindOnce(std::move(callback), server_response);
  abp_runner->PostTask(
      FROM_HERE, base::BindOnce(&utils::BenchmarkOperation,
                                "Notification after download for " + url.spec(),
                                std::move(closure)));
}

void OnURLCompleted(
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    const GURL& url,
    base::OnceCallback<void(const AdblockPlus::ServerResponse&)> callback,
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
    base::FilePath path) {
  AdblockPlus::ServerResponse server_response;
  // Unknown. Will be set below if the headers are there and contains response
  // code.
  server_response.responseStatus = 0;
  if (url_loader->NetError() != net::OK) {
    LOG(ERROR) << "[ABP] Error for " << url << ": " << url_loader->NetError();
    server_response.status =
        AdblockPlus::IWebRequest::NS_CUSTOM_ERROR_BASE + url_loader->NetError();
  } else {
    server_response.status = AdblockPlus::IWebRequest::NS_OK;
  }
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers) {
    server_response.responseStatus =
        url_loader->ResponseInfo()->headers->response_code();
    LOG(INFO) << "[ABP] Response status for " << url.spec() << " : "
              << server_response.responseStatus;
    size_t iter = 0;
    std::string key, value;
    while (url_loader->ResponseInfo()->headers->EnumerateHeaderLines(
        &iter, &key, &value)) {
      server_response.responseHeaders.push_back(std::make_pair(key, value));
    }
  }

  if (path.empty()) {
    OnDownloadedFileRead(url, std::move(server_response), std::move(callback),
                         abp_runner, {});
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(&ReadDownloadedFile, path),
      base::BindOnce(OnDownloadedFileRead, url, std::move(server_response),
                     std::move(callback), abp_runner));
}

void Load(const char* method,
          const GURL& url,
          const AdblockPlus::HeaderList& headers,
          base::OnceCallback<void(const AdblockPlus::ServerResponse&)> callback,
          network::mojom::URLLoaderFactory* factory,
          scoped_refptr<base::SingleThreadTaskRunner> abp_runner) {
  if (!factory) {
    AdblockPlus::ServerResponse server_response;
    server_response.responseStatus = 0;
    OnDownloadedFileRead(url, std::move(server_response), std::move(callback),
                         abp_runner, {});
    return;
  }
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->method = method;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("adblockplus_webrequest", R"(
        semantics {
          sender: "libadblockplus"
          description:
            "An implementation of the interface required by libadblockplus "
            "in order to be able to provide network communication. "
            "The communication mostly boils down to downloading new filter "
            "upon initialization or user demand"
          trigger:
            "On libadblockplus demand - mostly for downloading new filter lists"
          data:
            "Custom HTTP headers containing no user data"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          setting:
            "You enable or disable this feature via 'Adblock Enable' pref."
          policy_exception_justification: "Not implemented."
          }
        })");
  for (const auto& header : headers) {
    request->headers.SetHeader(header.first, header.second);
  }

  auto url_loader =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  url_loader->SetTimeoutDuration(base::TimeDelta::FromSeconds(60));
  url_loader->SetRetryOptions(
      kMaxRetries, network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE |
                       network::SimpleURLLoader::RETRY_ON_NAME_NOT_RESOLVED);
  url_loader->SetAllowHttpErrorResults(true);

  url_loader->DownloadToTempFile(
      factory,
      base::BindOnce(&OnURLCompleted, std::move(url_loader), url,
                     std::move(callback), abp_runner),
      kMaxDownloadSize);
}

}  // namespace

AdblockNetworkImpl::AdblockNetworkImpl(
    scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
    scoped_refptr<base::SingleThreadTaskRunner> abp_runner,
    network::mojom::URLLoaderFactory* loader_factory)
    : ui_runner_(ui_runner),
      abp_runner_(abp_runner),
      factory_(loader_factory),
      weak_ptr_factory_(this) {
  DCHECK(abp_runner_->BelongsToCurrentThread());
}

AdblockNetworkImpl::~AdblockNetworkImpl() {
  DCHECK(abp_runner_->BelongsToCurrentThread());
}

void AdblockNetworkImpl::GET(const std::string& url,
                             const AdblockPlus::HeaderList& headers,
                             const IWebRequest::RequestCallback& callback) {
  DoRequest(net::HttpRequestHeaders::kGetMethod, url, headers, callback);
}

void AdblockNetworkImpl::HEAD(const std::string& url,
                              const AdblockPlus::HeaderList& headers,
                              const IWebRequest::RequestCallback& callback) {
  DoRequest(net::HttpRequestHeaders::kHeadMethod, url, headers, callback);
}

void AdblockNetworkImpl::DoRequest(
    const char* method,
    const std::string& url,
    const AdblockPlus::HeaderList& headers,
    const IWebRequest::RequestCallback& callback) {
  DCHECK(method);
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implemenations provided to libabp has to be called on "
         "the same thread. Otherwise thread safety can not be assured.";
  DLOG(INFO) << "[ABP] Request " << method << " " << url;

  auto gurl = GURL{url};
  auto notification =
      base::BindOnce(&AdblockNetworkImpl::NotifyResults,
                     weak_ptr_factory_.GetWeakPtr(), callback, gurl);
  ui_runner_->PostTask(FROM_HERE, base::BindOnce(&Load, method, gurl, headers,
                                                 std::move(notification),
                                                 factory_, abp_runner_));
}

void AdblockNetworkImpl::NotifyResults(
    const AdblockPlus::IWebRequest::RequestCallback& callback,
    const GURL& url,
    const AdblockPlus::ServerResponse& response) {
  DCHECK(abp_runner_->BelongsToCurrentThread())
      << "All implemenations provided to libabp has to be called on "
         "the same thread. Otherwise thread safety can not be assured.";

  callback(response);
}

}  // namespace adblock

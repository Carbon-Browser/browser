// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFETCH_PREFETCH_PROXY_CHROME_PREFETCH_SERVICE_DELEGATE_H_
#define CHROME_BROWSER_PREFETCH_PREFETCH_PROXY_CHROME_PREFETCH_SERVICE_DELEGATE_H_

#include <string>

#include "base/time/time.h"
#include "content/public/browser/prefetch_service_delegate.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

class Profile;
class PrefetchProxyOriginDecider;

class ChromePrefetchServiceDelegate : public content::PrefetchServiceDelegate {
 public:
  explicit ChromePrefetchServiceDelegate(
      content::BrowserContext* browser_context);
  ~ChromePrefetchServiceDelegate() override;

  ChromePrefetchServiceDelegate(const ChromePrefetchServiceDelegate&) = delete;
  ChromePrefetchServiceDelegate& operator=(
      const ChromePrefetchServiceDelegate&) = delete;

  // content::PrefetchServiceDelegate
  std::string GetMajorVersionNumber() override;
  std::string GetAcceptLanguageHeader() override;
  GURL GetDefaultPrefetchProxyHost() override;
  std::string GetAPIKey() override;
  void ReportOriginRetryAfter(const GURL& url,
                              base::TimeDelta retry_after) override;
  bool IsOriginOutsideRetryAfterWindow(const GURL& url) override;
  void ClearData() override;
  bool DisableDecoysBasedOnUserSettings() override;
  bool IsSomePreloadingEnabled() override;
  bool IsExtendedPreloadingEnabled() override;
  bool IsDomainInPrefetchAllowList(const GURL& referring_url) override;

 private:
  // The profile that |this| is associated with.
  raw_ptr<Profile> profile_;

  // Tracks "Retry-After" responses, and determines whether new prefetches are
  // eligible based on those responses.
  std::unique_ptr<PrefetchProxyOriginDecider> origin_decider_;
};

#endif  // CHROME_BROWSER_PREFETCH_PREFETCH_PROXY_CHROME_PREFETCH_SERVICE_DELEGATE_H_

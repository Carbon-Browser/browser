// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This source code is a part of eyeo Chromium SDK.
// Use of this source code is governed by the GPLv3 that can be found in the
// components/adblock/LICENSE file.

#ifndef CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include <memory>
#include <vector>

#include "base/threading/thread_checker.h"
#include "components/safe_browsing/content/common/safe_browsing.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/url_loader_throttle_provider.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/renderer/extension_throttle_manager.h"
#endif

class ChromeContentRendererClient;

// Instances must be constructed on the render thread, and then used and
// destructed on a single thread, which can be different from the render thread.
class URLLoaderThrottleProviderImpl : public blink::URLLoaderThrottleProvider {
 public:
  URLLoaderThrottleProviderImpl(
      blink::ThreadSafeBrowserInterfaceBrokerProxy* broker,
      blink::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client);

  URLLoaderThrottleProviderImpl& operator=(
      const URLLoaderThrottleProviderImpl&) = delete;

  ~URLLoaderThrottleProviderImpl() override;

  // blink::URLLoaderThrottleProvider implementation.
  std::unique_ptr<blink::URLLoaderThrottleProvider> Clone() override;
  blink::WebVector<std::unique_ptr<blink::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURLRequest& request) override;
  void SetOnline(bool is_online) override;

  // eyeo Chromium SDK: Changed private to protected to allow
  // AdblockURLLoaderThrottleProviderImpl to reuse copy ctor.
 protected:
  // This copy constructor works in conjunction with Clone(), not intended for
  // general use.
  URLLoaderThrottleProviderImpl(const URLLoaderThrottleProviderImpl& other);

  blink::URLLoaderThrottleProviderType type_;
  ChromeContentRendererClient* const chrome_content_renderer_client_;

  mojo::PendingRemote<safe_browsing::mojom::SafeBrowsing> safe_browsing_remote_;
  mojo::Remote<safe_browsing::mojom::SafeBrowsing> safe_browsing_;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionThrottleManager>
      extension_throttle_manager_;
#endif

  THREAD_CHECKER(thread_checker_);
};

#endif  // CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

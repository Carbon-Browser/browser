// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_
#define CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/devtools/protocol/page.h"
#include "components/webapps/browser/installable/installable_manager.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/blink/public/common/manifest/manifest.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/headless/headless_print_manager.h"
#include "components/printing/browser/print_to_pdf/pdf_print_result.h"
#endif  // BUILDFLAG(ENABLE_PRINTING)

namespace content {
struct InstallabilityError;
class WebContents;
}  // namespace content

class SkBitmap;

class PageHandler : public protocol::Page::Backend {
 public:
  PageHandler(scoped_refptr<content::DevToolsAgentHost> agent_host,
              content::WebContents* web_contents,
              protocol::UberDispatcher* dispatcher);

  PageHandler(const PageHandler&) = delete;
  PageHandler& operator=(const PageHandler&) = delete;

  ~PageHandler() override;

  void ToggleAdBlocking(bool enabled);

  // Page::Backend:
  protocol::Response Enable() override;
  protocol::Response Disable() override;
  protocol::Response SetAdBlockingEnabled(bool enabled) override;
  protocol::Response SetSPCTransactionMode(
      const protocol::String& mode) override;
  protocol::Response SetRPHRegistrationMode(
      const protocol::String& mode) override;
  void GetInstallabilityErrors(
      std::unique_ptr<GetInstallabilityErrorsCallback> callback) override;

  void GetManifestIcons(
      std::unique_ptr<GetManifestIconsCallback> callback) override;

  void PrintToPDF(std::optional<bool> landscape,
                  std::optional<bool> display_header_footer,
                  std::optional<bool> print_background,
                  std::optional<double> scale,
                  std::optional<double> paper_width,
                  std::optional<double> paper_height,
                  std::optional<double> margin_top,
                  std::optional<double> margin_bottom,
                  std::optional<double> margin_left,
                  std::optional<double> margin_right,
                  std::optional<protocol::String> page_ranges,
                  std::optional<protocol::String> header_template,
                  std::optional<protocol::String> footer_template,
                  std::optional<bool> prefer_css_page_size,
                  std::optional<protocol::String> transfer_mode,
                  std::optional<bool> generate_tagged_pdf,
                  std::optional<bool> generate_document_outline,
                  std::unique_ptr<PrintToPDFCallback> callback) override;

  void GetAppId(std::unique_ptr<GetAppIdCallback> callback) override;

 private:
  static void GotInstallabilityErrors(
      std::unique_ptr<GetInstallabilityErrorsCallback> callback,
      std::vector<content::InstallabilityError> installability_errors);

  static void GotManifestIcons(
      std::unique_ptr<GetManifestIconsCallback> callback,
      const SkBitmap* primary_icon);

  void OnDidGetManifest(std::unique_ptr<GetAppIdCallback> callback,
                        const webapps::InstallableData& data);

#if BUILDFLAG(ENABLE_PRINTING)
  void OnPDFCreated(bool return_as_stream,
                    std::unique_ptr<PrintToPDFCallback> callback,
                    print_to_pdf::PdfPrintResult print_result,
                    scoped_refptr<base::RefCountedMemory> data);
#endif

  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  base::WeakPtr<content::WebContents> web_contents_;

  bool enabled_ = false;

  base::WeakPtrFactory<PageHandler> weak_ptr_factory_{this};
};

#endif  // CHROME_BROWSER_DEVTOOLS_PROTOCOL_PAGE_HANDLER_H_

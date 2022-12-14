// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DESKTOP_CAPTURE_DESKTOP_CAPTURE_BASE_H_
#define CHROME_BROWSER_EXTENSIONS_API_DESKTOP_CAPTURE_DESKTOP_CAPTURE_BASE_H_

#include <array>
#include <map>
#include <memory>
#include <string>

#include "base/memory/singleton.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/desktop_media_picker_controller.h"
#include "chrome/browser/media/webrtc/desktop_media_picker_factory.h"
#include "chrome/common/extensions/api/desktop_capture.h"
#include "extensions/browser/extension_function.h"
#include "url/gurl.h"

namespace extensions {

class DesktopCaptureChooseDesktopMediaFunctionBase : public ExtensionFunction {
 public:
  // Used to set PickerFactory used to create mock DesktopMediaPicker instances
  // for tests. Calling tests keep ownership of the factory. Can be called with
  // |factory| set to NULL at the end of the test.
  static void SetPickerFactoryForTests(DesktopMediaPickerFactory* factory);

  DesktopCaptureChooseDesktopMediaFunctionBase();

  void Cancel();

 protected:
  ~DesktopCaptureChooseDesktopMediaFunctionBase() override;

  static const char kTargetNotFoundError[];

  // |exclude_system_audio| is piped from the original call. It is a constraint
  // that needs to be applied before the picker is shown to the user, as it
  // affects the picker. It is therefore provided to the extension function
  // rather than to getUserMedia(), as is the case for other constraints.
  //
  // |origin| is the origin for which the stream is created.
  //
  // |target_name| is the display name of the stream target.
  ResponseAction Execute(
      const std::vector<api::desktop_capture::DesktopCaptureSourceType>&
          sources,
      bool exclude_system_audio,
      content::RenderFrameHost* render_frame_host,
      const GURL& origin,
      const std::u16string target_name);

  // Returns the calling application name to show in the picker.
  std::string GetCallerDisplayName() const;

  int request_id_;

 private:
  void OnPickerDialogResults(
      const GURL& origin,
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      const std::string& err,
      content::DesktopMediaID source);

  std::unique_ptr<DesktopMediaPickerController> picker_controller_;
};

class DesktopCaptureCancelChooseDesktopMediaFunctionBase
    : public ExtensionFunction {
 public:
  DesktopCaptureCancelChooseDesktopMediaFunctionBase();

 protected:
  ~DesktopCaptureCancelChooseDesktopMediaFunctionBase() override;

 private:
  // ExtensionFunction overrides.
  ResponseAction Run() override;
};

class DesktopCaptureRequestsRegistry {
 public:
  DesktopCaptureRequestsRegistry();

  DesktopCaptureRequestsRegistry(const DesktopCaptureRequestsRegistry&) =
      delete;
  DesktopCaptureRequestsRegistry& operator=(
      const DesktopCaptureRequestsRegistry&) = delete;

  ~DesktopCaptureRequestsRegistry();

  static DesktopCaptureRequestsRegistry* GetInstance();

  void AddRequest(int process_id,
                  int request_id,
                  DesktopCaptureChooseDesktopMediaFunctionBase* handler);
  void RemoveRequest(int process_id, int request_id);
  void CancelRequest(int process_id, int request_id);

 private:
  friend struct base::DefaultSingletonTraits<DesktopCaptureRequestsRegistry>;

  struct RequestId {
    RequestId(int process_id, int request_id);

    // Need to use RequestId as a key in std::map<>.
    bool operator<(const RequestId& other) const;

    int process_id;
    int request_id;
  };

  using RequestsMap =
      std::map<RequestId, DesktopCaptureChooseDesktopMediaFunctionBase*>;

  RequestsMap requests_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DESKTOP_CAPTURE_DESKTOP_CAPTURE_BASE_H_

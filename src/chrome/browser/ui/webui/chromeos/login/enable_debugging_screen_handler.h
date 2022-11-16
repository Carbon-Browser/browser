// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENABLE_DEBUGGING_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENABLE_DEBUGGING_SCREEN_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

class PrefRegistrySimple;

namespace chromeos {

// Interface between enable debugging screen and its representation.
// Note, do not forget to call OnViewDestroyed in the dtor.
class EnableDebuggingScreenView
    : public base::SupportsWeakPtr<EnableDebuggingScreenView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{"debugging",
                                                       "EnableDebuggingScreen"};

  enum UIState {
    UI_STATE_ERROR = -1,
    UI_STATE_REMOVE_PROTECTION = 1,
    UI_STATE_SETUP = 2,
    UI_STATE_WAIT = 3,
    UI_STATE_DONE = 4,
  };

  virtual ~EnableDebuggingScreenView() = default;

  virtual void Show() = 0;
  virtual void UpdateUIState(UIState state) = 0;
};

// WebUI implementation of EnableDebuggingScreenView.
class EnableDebuggingScreenHandler : public EnableDebuggingScreenView,
                                     public BaseScreenHandler {
 public:
  using TView = EnableDebuggingScreenView;

  EnableDebuggingScreenHandler();

  EnableDebuggingScreenHandler(const EnableDebuggingScreenHandler&) = delete;
  EnableDebuggingScreenHandler& operator=(const EnableDebuggingScreenHandler&) =
      delete;

  ~EnableDebuggingScreenHandler() override;

  // EnableDebuggingScreenView implementation:
  void Show() override;
  void UpdateUIState(UIState state) override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // Registers Local State preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);
};

}  // namespace chromeos

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace ash {
using ::chromeos::EnableDebuggingScreenHandler;
using ::chromeos::EnableDebuggingScreenView;
}

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ENABLE_DEBUGGING_SCREEN_HANDLER_H_

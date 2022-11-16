// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_LOGIN_ENROLLMENT_AUTO_ENROLLMENT_CHECK_SCREEN_VIEW_H_
#define CHROME_BROWSER_ASH_LOGIN_ENROLLMENT_AUTO_ENROLLMENT_CHECK_SCREEN_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ash/login/oobe_screen.h"

namespace ash {

// Interface between auto-enrollment check screen and its representation.
class AutoEnrollmentCheckScreenView
    : public base::SupportsWeakPtr<AutoEnrollmentCheckScreenView> {
 public:
  inline constexpr static StaticOobeScreenId kScreenId{
      "auto-enrollment-check", "AutoEnrollmentCheckScreen"};

  virtual ~AutoEnrollmentCheckScreenView() = default;

  virtual void Show() = 0;
};

}  // namespace ash

// TODO(https://crbug.com/1164001): remove after the //chrome/browser/chromeos
// source migration is finished.
namespace chromeos {
using ::ash::AutoEnrollmentCheckScreenView;
}

#endif  // CHROME_BROWSER_ASH_LOGIN_ENROLLMENT_AUTO_ENROLLMENT_CHECK_SCREEN_VIEW_H_

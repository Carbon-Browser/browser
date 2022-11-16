// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/auto_enrollment_check_screen_handler.h"

#include "chrome/browser/ash/login/oobe_screen.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace chromeos {

AutoEnrollmentCheckScreenHandler::AutoEnrollmentCheckScreenHandler()
    : BaseScreenHandler(kScreenId) {}

void AutoEnrollmentCheckScreenHandler::Show() {
  ShowInWebUI();
}

void AutoEnrollmentCheckScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("autoEnrollmentCheckMessage",
               IDS_AUTO_ENROLLMENT_CHECK_SCREEN_MESSAGE);
}

}  // namespace chromeos

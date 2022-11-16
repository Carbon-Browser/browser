// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/bruschetta/fake_bruschetta_launcher.h"

#include "base/callback_forward.h"

namespace bruschetta {
FakeBruschettaLauncher::FakeBruschettaLauncher()
    : BruschettaLauncher("vm_name", nullptr) {}

FakeBruschettaLauncher::~FakeBruschettaLauncher() = default;

void FakeBruschettaLauncher::EnsureRunning(
    base::OnceCallback<void(BruschettaResult)> callback) {
  std::move(callback).Run(result_);
}
}  // namespace bruschetta

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_BRUSCHETTA_BRUSCHETTA_SERVICE_H_
#define CHROME_BROWSER_ASH_BRUSCHETTA_BRUSCHETTA_SERVICE_H_

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace bruschetta {

class BruschettaLauncher;

// A service to hold the separate modules that provide Bruschetta
// (third-party/generic VM) support within Chrome (files app integration, app
// service integration, etc).
class BruschettaService : public KeyedService {
 public:
  explicit BruschettaService(Profile* profile);
  ~BruschettaService() override;

  // Helper method to get the service instance for the given profile.
  static BruschettaService* GetForProfile(Profile* profile);

  // Returns a handle to the launcher for the vm specified by `vm_name`. Will
  // return a null pointer if the name isn't recognised.
  base::WeakPtr<BruschettaLauncher> GetLauncher(std::string vm_name);

  void SetLauncherForTesting(std::string vm_name,
                             std::unique_ptr<BruschettaLauncher> launcher);

 private:
  base::flat_map<std::string, std::unique_ptr<BruschettaLauncher>> launchers_;
};

}  // namespace bruschetta

#endif  // CHROME_BROWSER_ASH_BRUSCHETTA_BRUSCHETTA_SERVICE_H_

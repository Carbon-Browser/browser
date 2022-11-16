/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_

#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "components/adblock/core/subscription/subscription_validator.h"
#include "components/prefs/pref_service.h"

class PrefService;

namespace adblock {

// Stores the hash of FlatbufferData in a Tracked Pref.
class SubscriptionValidatorImpl final : public SubscriptionValidator {
 public:
  SubscriptionValidatorImpl(
      PrefService* pref_service,
      const std::string& current_schema_version,
      scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner);
  bool IsSignatureValid(const FlatbufferData& data,
                        const base::FilePath& path) const final;
  void StoreTrustedSignature(const FlatbufferData& data,
                             const base::FilePath& path) final;
  void RemoveStoredSignature(const base::FilePath& path) final;

 private:
  ~SubscriptionValidatorImpl() final;
  void StoreTrustedSignatureOnMainThread(std::string signature,
                                         const base::FilePath& path);
  void RemoveStoredSignatureInMainThread(const base::FilePath& path);

  PrefService* pref_service_;
  scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner_;
  base::Value initial_subscription_signatures_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_SUBSCRIPTION_VALIDATOR_IMPL_H_

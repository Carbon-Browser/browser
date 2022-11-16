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

#include "components/adblock/core/subscription/subscription_validator_impl.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/sha2.h"

namespace adblock {
namespace {

std::string ComputeSubscriptionHash(const FlatbufferData& buffer) {
  return base::Base64Encode(
      crypto::SHA256Hash(base::make_span(buffer.data(), buffer.size())));
}

// When the schema version used to create installed subscriptions is different
// from the schema version known by this browser, we should not attempt to read
// the flatbuffers - we would misinterpret their content.
// Clear the stored subscription signatures to indicate the files are invalid.
void ClearSignaturesIfSchemaVersionChanged(
    PrefService* pref_service,
    const std::string& current_schema_version) {
  if (pref_service->GetString(prefs::kLastUsedSchemaVersion) !=
      current_schema_version) {
    if (!pref_service->FindPreference(prefs::kSubscriptionSignatures)
             ->IsDefaultValue()) {
      LOG(INFO) << "[eyeo] Schema version has changed, invalidating stored "
                   "subscriptions.";
      pref_service->ClearPref(prefs::kSubscriptionSignatures);
    }
    pref_service->SetString(prefs::kLastUsedSchemaVersion,
                            current_schema_version);
  }
}

}  // namespace

SubscriptionValidatorImpl::SubscriptionValidatorImpl(
    PrefService* pref_service,
    const std::string& current_schema_version,
    scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner)
    : pref_service_(pref_service),
      main_thread_task_runner_(std::move(main_thread_task_runner)) {
  ClearSignaturesIfSchemaVersionChanged(pref_service_, current_schema_version);
  initial_subscription_signatures_ =
      pref_service_->Get(prefs::kSubscriptionSignatures)->Clone();
}

SubscriptionValidatorImpl::~SubscriptionValidatorImpl() = default;

bool SubscriptionValidatorImpl::IsSignatureValid(
    const FlatbufferData& data,
    const base::FilePath& path) const {
  const auto* expected_hash = initial_subscription_signatures_.FindStringKey(
      path.BaseName().AsUTF8Unsafe());
  if (!expected_hash) {
    DLOG(WARNING) << "[eyeo] " << path << " has no matching signature in prefs";
    return false;
  }
  if (*expected_hash != ComputeSubscriptionHash(data)) {
    DLOG(WARNING) << "[eyeo] " << path << " has invalid signature in prefs";
    return false;
  }
  return true;
}

void SubscriptionValidatorImpl::StoreTrustedSignature(
    const FlatbufferData& data,
    const base::FilePath& path) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SubscriptionValidatorImpl::StoreTrustedSignatureOnMainThread, this,
          ComputeSubscriptionHash(data), path));
}

void SubscriptionValidatorImpl::RemoveStoredSignature(
    const base::FilePath& path) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SubscriptionValidatorImpl::RemoveStoredSignatureInMainThread, this,
          path));
}

void SubscriptionValidatorImpl::StoreTrustedSignatureOnMainThread(
    std::string signature,
    const base::FilePath& path) {
  DictionaryPrefUpdate pref_update(pref_service_,
                                   prefs::kSubscriptionSignatures);
  pref_update->SetStringKey(path.BaseName().AsUTF8Unsafe(), signature);
}

void SubscriptionValidatorImpl::RemoveStoredSignatureInMainThread(
    const base::FilePath& path) {
  DictionaryPrefUpdate pref_update(pref_service_,
                                   prefs::kSubscriptionSignatures);
  pref_update->RemoveKey(path.BaseName().AsUTF8Unsafe());
}

}  // namespace adblock

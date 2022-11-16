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
#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/grit/components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {

class AdblockSubscriptionValidatorImplTest : public testing::Test {
 public:
  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

  scoped_refptr<SubscriptionValidatorImpl> MakeValidator(
      const std::string& current_schema_version) {
    return base::MakeRefCounted<SubscriptionValidatorImpl>(
        &pref_service_, current_schema_version,
        task_environment_.GetMainThreadTaskRunner());
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(AdblockSubscriptionValidatorImplTest, EmptyPrefMeansNoSignature) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  EXPECT_FALSE(validator->IsSignatureValid(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       StoredSignatureVisibleOnlyAfterRecreatingValidator) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  task_environment_.RunUntilIdle();
  // |IsSignatureValid| uses the initial state of prefs, not the
  // current state, to avoid race conditions. It will still return the old
  // result.
  EXPECT_FALSE(validator->IsSignatureValid(*buffer, path));
  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // The new initial state has the signature stored in previous life.
  EXPECT_TRUE(validator->IsSignatureValid(*buffer, path));
  // Only the file component of the path is the key, this allows moving
  // to a different storage directory if needed.
  base::FilePath reparented_path(FILE_PATH_LITERAL("parent/x.fb"));
  EXPECT_TRUE(validator->IsSignatureValid(*buffer, reparented_path));
}

TEST_F(AdblockSubscriptionValidatorImplTest, StoreAndRemoveSignature) {
  auto validator = MakeValidator(CurrentSchemaVersion());
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  validator->RemoveStoredSignature(path);
  task_environment_.RunUntilIdle();
  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature was removed so it's no longer returned.
  EXPECT_FALSE(validator->IsSignatureValid(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       SchemaVersionChangeInvalidatesSignatures) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  task_environment_.RunUntilIdle();

  // Simulate a schema version change by storing a kLastUsedSchemaVersion
  // different than the current one.
  pref_service_.SetString(prefs::kLastUsedSchemaVersion, "000");

  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // The signature is invalid because we're not allowed to read flatbuffers
  // created with a different schema version.
  EXPECT_FALSE(validator->IsSignatureValid(*buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest, SignatureStoredViaKey) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store a valid signature.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  task_environment_.RunUntilIdle();

  // When storing dictionary keys with dots in them, there's a difference if
  // you use "SetIntKey" vs "SetIntPath".
  // "Path" interprets dict["x.fb"]="hash" as {"x":{"fb":"hash"}} - the dot
  // indicates a deeper level of dict
  // "Key" interprets dict["x.fb"]="hash" as {"x.fb":"hash"} - the dot is part
  // of the key name
  const base::Value* pref = pref_service_.Get(prefs::kSubscriptionSignatures);
  ASSERT_TRUE(pref->FindStringKey("x.fb"));
  EXPECT_EQ(*pref->FindStringKey("x.fb"),
            base::Base64Encode(crypto::SHA256Hash(
                base::make_span(buffer->data(), buffer->size()))));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       DifferentBufferFailsSignatureValidation) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store signature for IDR_ADBLOCK_FLATBUFFER_EASYLIST subscription.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  task_environment_.RunUntilIdle();
  // Recreate the validator.
  validator = MakeValidator(CurrentSchemaVersion());
  // If a different buffer resides on the same path, the signature does not
  // match.
  auto different_buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES);
  EXPECT_FALSE(validator->IsSignatureValid(*different_buffer, path));
}

TEST_F(AdblockSubscriptionValidatorImplTest,
       NewSchemaVersionInvalidatesSubscriptions) {
  auto validator = MakeValidator(CurrentSchemaVersion());

  // Store signature for IDR_ADBLOCK_FLATBUFFER_EASYLIST subscription.
  auto buffer = utils::MakeFlatbufferDataFromResourceBundle(
      IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  base::FilePath path(FILE_PATH_LITERAL("x.fb"));
  validator->StoreTrustedSignature(*buffer, path);
  task_environment_.RunUntilIdle();

  // Recreate the validator with new schema version.
  validator = MakeValidator(CurrentSchemaVersion() + std::string("new"));
  // Same buffer, same path, but it's no longer valid because schema has
  // changed.
  EXPECT_FALSE(validator->IsSignatureValid(*buffer, path));
}

}  // namespace adblock

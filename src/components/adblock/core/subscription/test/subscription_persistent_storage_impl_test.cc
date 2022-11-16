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

#include "components/adblock/core/subscription/subscription_persistent_storage_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/containers/span.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/string_piece_forward.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/test/mock_subscription_persistent_metadata.h"
#include "components/grit/components_resources.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace adblock {
namespace {

class MockSubscriptionValidator : public SubscriptionValidator {
 public:
  MOCK_METHOD(bool,
              IsSignatureValid,
              (const FlatbufferData& data, const base::FilePath& path),
              (override, const));
  MOCK_METHOD(void,
              StoreTrustedSignature,
              (const FlatbufferData& data, const base::FilePath& path),
              (override));
  MOCK_METHOD(void,
              RemoveStoredSignature,
              (const base::FilePath& path),
              (override));

 protected:
  ~MockSubscriptionValidator() override = default;
};

MATCHER_P(BufferMatches, expected_span, "") {
  const auto arg_span = base::make_span(arg.data(), arg.size());
  return std::equal(arg_span.begin(), arg_span.end(), expected_span.begin(),
                    expected_span.end());
}

}  // namespace

class AdblockSubscriptionPersistentStorageImplTest : public ::testing::Test {
 public:
  void SetUp() final {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    validator_ = base::MakeRefCounted<MockSubscriptionValidator>();
    storage_ = std::make_unique<SubscriptionPersistentStorageImpl>(
        temp_dir_.GetPath(), validator_, &metadata_);
  }

  base::FilePath PathRelativeToTemp(base::StringPiece file_name) const {
    return temp_dir_.GetPath().AppendASCII(file_name);
  }

  const std::unique_ptr<FlatbufferData> kEasylistFlatbuffer =
      utils::MakeFlatbufferDataFromResourceBundle(
          IDR_ADBLOCK_FLATBUFFER_EASYLIST);
  const base::span<const uint8_t> kEasylistData =
      base::make_span(kEasylistFlatbuffer->data(), kEasylistFlatbuffer->size());
  const std::unique_ptr<FlatbufferData> kExceptionrulesFlatbuffer =
      utils::MakeFlatbufferDataFromResourceBundle(
          IDR_ADBLOCK_FLATBUFFER_EXCEPTIONRULES);
  const base::span<const uint8_t> kExceptionrulesData =
      base::make_span(kExceptionrulesFlatbuffer->data(),
                      kExceptionrulesFlatbuffer->size());

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  MockSubscriptionPersistentMetadata metadata_;
  scoped_refptr<MockSubscriptionValidator> validator_;
  std::unique_ptr<SubscriptionPersistentStorageImpl> storage_;
};

TEST_F(AdblockSubscriptionPersistentStorageImplTest,
       ReadsValidSubscriptionsAndDeletesInvalid) {
  // Populate temp_dir_ with two valid flatbuffer files and two invalid ones.
  ASSERT_TRUE(
      base::WriteFile(PathRelativeToTemp("easylist.fb"), kEasylistData));
  ASSERT_TRUE(base::WriteFile(PathRelativeToTemp("exceptionrules.fb"),
                              kExceptionrulesData));
  ASSERT_TRUE(base::WriteFile(PathRelativeToTemp("invalid1.fb"), "some_data"));
  ASSERT_TRUE(base::WriteFile(PathRelativeToTemp("invalid2.fb"), "bogus"));

  // Initialize the storage, it should attempt to read contents of temp_dir_.
  base::MockCallback<SubscriptionPersistentStorage::LoadCallback> callback;

  // Save the argument passed to callback.
  std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions;
  EXPECT_CALL(callback, Run(testing::_))
      .WillOnce(testing::SaveArg<0>(&loaded_subscriptions));

  // Storage will query metadata for the subscriptions in managed to read from
  // disk.
  const auto installation_time_easylist =
      base::Time::FromDeltaSinceWindowsEpoch(base::Seconds(30));
  const auto installation_time_exceptionrules =
      base::Time::FromDeltaSinceWindowsEpoch(base::Seconds(60));
  EXPECT_CALL(metadata_, GetLastInstallationTime(DefaultSubscriptionUrl()))
      .WillOnce(testing::Return(installation_time_easylist));
  EXPECT_CALL(metadata_, GetLastInstallationTime(AcceptableAdsUrl()))
      .WillOnce(testing::Return(installation_time_exceptionrules));

  // Subscriptions found on disk will be validated.
  // First two files are OK.
  EXPECT_CALL(*validator_, IsSignatureValid(BufferMatches(kEasylistData),
                                            PathRelativeToTemp("easylist.fb")))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*validator_,
              IsSignatureValid(BufferMatches(kExceptionrulesData),
                               PathRelativeToTemp("exceptionrules.fb")))
      .WillOnce(testing::Return(true));
  // The other two are invalid.
  EXPECT_CALL(*validator_,
              IsSignatureValid(BufferMatches(std::string("some_data")),
                               PathRelativeToTemp("invalid1.fb")))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*validator_, IsSignatureValid(BufferMatches(std::string("bogus")),
                                            PathRelativeToTemp("invalid2.fb")))
      .WillOnce(testing::Return(false));

  storage_->LoadSubscriptions(callback.Get());
  task_environment_.RunUntilIdle();

  // The two valid subscriptions were loaded.
  ASSERT_EQ(loaded_subscriptions.size(), 2u);
  base::ranges::sort(loaded_subscriptions, {}, &Subscription::GetSourceUrl);

  EXPECT_EQ(loaded_subscriptions[0]->GetSourceUrl(), DefaultSubscriptionUrl());
  EXPECT_EQ(loaded_subscriptions[0]->GetInstallationState(),
            Subscription::InstallationState::Installed);
  EXPECT_EQ(loaded_subscriptions[0]->GetInstallationTime(),
            installation_time_easylist);

  EXPECT_EQ(loaded_subscriptions[1]->GetSourceUrl(), AcceptableAdsUrl());
  EXPECT_EQ(loaded_subscriptions[1]->GetInstallationState(),
            Subscription::InstallationState::Installed);
  EXPECT_EQ(loaded_subscriptions[1]->GetInstallationTime(),
            installation_time_exceptionrules);

  // The storage directory no longer contains the invalid files.
  EXPECT_FALSE(base::PathExists(PathRelativeToTemp("invalid1.fb")));
  EXPECT_FALSE(base::PathExists(PathRelativeToTemp("invalid2.fb")));
}

TEST_F(AdblockSubscriptionPersistentStorageImplTest, StoreValidSubscription) {
  storage_->LoadSubscriptions(base::DoNothing());
  task_environment_.RunUntilIdle();

  // Attempt to store a valid subscription.
  base::MockCallback<SubscriptionPersistentStorage::StoreCallback> callback;
  // The callback will be called with parsed Subscription. Save the argument.
  scoped_refptr<Subscription> loaded_subscription;
  EXPECT_CALL(callback, Run(testing::_))
      .WillOnce(testing::SaveArg<0>(&loaded_subscription));
  // The subscription will get its signature stored.
  base::FilePath signature_path;
  EXPECT_CALL(*validator_,
              StoreTrustedSignature(BufferMatches(kEasylistData), testing::_))
      .WillOnce(testing::SaveArg<1>(&signature_path));

  storage_->StoreSubscription(utils::MakeFlatbufferDataFromResourceBundle(
                                  IDR_ADBLOCK_FLATBUFFER_EASYLIST),
                              callback.Get());
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(loaded_subscription);
  EXPECT_EQ(loaded_subscription->GetSourceUrl(), DefaultSubscriptionUrl());

  // The storage directory is not empty, contains the subscription file with
  // unspecified filename.
  EXPECT_FALSE(base::IsDirectoryEmpty(temp_dir_.GetPath()));
  base::FileEnumerator enumerator(temp_dir_.GetPath(), false,
                                  base::FileEnumerator::FILES);
  base::FilePath subscription_path = enumerator.Next();
  EXPECT_FALSE(subscription_path.empty());
  // The base file API operates on chars and flatbuffer data is stored in
  // unsigned chars. To compare the content of a file with flatbuffer data, we
  // need to reinterpret cast.
  std::string file_content;
  ASSERT_TRUE(base::ReadFileToString(subscription_path, &file_content));
  const base::span<const uint8_t> reinterpreted_content(
      reinterpret_cast<const uint8_t*>(file_content.data()),
      file_content.size());
  EXPECT_TRUE(base::ranges::equal(reinterpreted_content, kEasylistData));
  // SignatureValidator was given the same path as the one used for storage.
  EXPECT_EQ(subscription_path, signature_path);
}

TEST_F(AdblockSubscriptionPersistentStorageImplTest,
       StoreAndRemoveSubscription) {
  storage_->LoadSubscriptions(base::DoNothing());
  task_environment_.RunUntilIdle();

  // Store a valid subscription.
  base::MockCallback<SubscriptionPersistentStorage::StoreCallback> callback;
  scoped_refptr<InstalledSubscription> loaded_subscription;
  EXPECT_CALL(callback, Run(testing::_))
      .WillOnce(testing::SaveArg<0>(&loaded_subscription));

  // The subscription will get its signature stored.
  base::FilePath signature_path;
  EXPECT_CALL(*validator_,
              StoreTrustedSignature(BufferMatches(kEasylistData), testing::_))
      .WillOnce(testing::SaveArg<1>(&signature_path));

  storage_->StoreSubscription(utils::MakeFlatbufferDataFromResourceBundle(
                                  IDR_ADBLOCK_FLATBUFFER_EASYLIST),
                              callback.Get());
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(loaded_subscription);

  // Directory is not empty, contains the subscription.
  EXPECT_FALSE(base::IsDirectoryEmpty(temp_dir_.GetPath()));

  // The subscription will get its signature removed.
  EXPECT_CALL(*validator_, RemoveStoredSignature(signature_path));

  // Remove the subscription.
  storage_->RemoveSubscription(loaded_subscription);
  task_environment_.RunUntilIdle();

  // Directory is now empty again.
  ASSERT_TRUE(loaded_subscription);
}

TEST_F(AdblockSubscriptionPersistentStorageImplTest, StorageIsPersistent) {
  // Initially, no loaded subscriptions.
  base::MockCallback<SubscriptionPersistentStorage::LoadCallback> callback;
  std::vector<scoped_refptr<InstalledSubscription>> loaded_subscriptions;
  EXPECT_CALL(callback, Run(testing::_))
      .WillOnce(testing::SaveArg<0>(&loaded_subscriptions));
  storage_->LoadSubscriptions(callback.Get());
  task_environment_.RunUntilIdle();

  ASSERT_TRUE(loaded_subscriptions.empty());

  // Validator will be asked to store the signature.
  base::FilePath signature_path;
  EXPECT_CALL(*validator_,
              StoreTrustedSignature(BufferMatches(kEasylistData), testing::_))
      .WillOnce(testing::SaveArg<1>(&signature_path));

  // Metadata will be updated.
  EXPECT_CALL(metadata_,
              SetExpirationInterval(DefaultSubscriptionUrl(), base::Days(1)));
  EXPECT_CALL(metadata_, SetVersion(DefaultSubscriptionUrl(), testing::_));
  EXPECT_CALL(metadata_,
              IncrementDownloadSuccessCount(DefaultSubscriptionUrl()));

  // Store a valid subscription.
  storage_->StoreSubscription(utils::MakeFlatbufferDataFromResourceBundle(
                                  IDR_ADBLOCK_FLATBUFFER_EASYLIST),
                              base::DoNothing());
  task_environment_.RunUntilIdle();

  // Destroy and re-create storage.
  storage_ = std::make_unique<SubscriptionPersistentStorageImpl>(
      temp_dir_.GetPath(), validator_, &metadata_);

  // Validator will be asked to check the signature of subscription on disk.
  // Will query the path passed to |StoreTrustedSignature| before.
  EXPECT_CALL(*validator_,
              IsSignatureValid(BufferMatches(kEasylistData), signature_path))
      .WillOnce(testing::Return(true));

  // Load subscriptions.
  EXPECT_CALL(callback, Run(testing::_))
      .WillOnce(testing::SaveArg<0>(&loaded_subscriptions));
  storage_->LoadSubscriptions(callback.Get());
  task_environment_.RunUntilIdle();

  // This time, we see the subscription added in storage's "previous life".
  ASSERT_EQ(loaded_subscriptions.size(), 1u);
  EXPECT_EQ(loaded_subscriptions[0]->GetSourceUrl(), DefaultSubscriptionUrl());
}

}  // namespace adblock

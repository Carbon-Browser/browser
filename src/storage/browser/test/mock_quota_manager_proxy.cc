// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/test/mock_quota_manager_proxy.h"

#include <utility>

#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "components/services/storage/public/mojom/quota_client.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"

namespace storage {

MockQuotaManagerProxy::MockQuotaManagerProxy(
    MockQuotaManager* quota_manager,
    scoped_refptr<base::SequencedTaskRunner> quota_manager_task_runner)
    : QuotaManagerProxy(
          quota_manager,
          std::move(quota_manager_task_runner),
          quota_manager ? quota_manager->profile_path() : base::FilePath()),
      mock_quota_manager_(quota_manager) {}

void MockQuotaManagerProxy::RegisterClient(
    mojo::PendingRemote<storage::mojom::QuotaClient> client,
    QuotaClientType client_type,
    const std::vector<blink::mojom::StorageType>& storage_types) {
  DCHECK(!registered_client_);
  registered_client_.Bind(std::move(client));
}

void MockQuotaManagerProxy::UpdateOrCreateBucket(
    const BucketInitParams& params,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceCallback<void(QuotaErrorOr<BucketInfo>)> callback) {
  if (mock_quota_manager_)
    mock_quota_manager_->UpdateOrCreateBucket(params, std::move(callback));
}

QuotaErrorOr<BucketInfo> MockQuotaManagerProxy::GetOrCreateBucketSync(
    const BucketInitParams& params) {
  return (mock_quota_manager_)
             ? mock_quota_manager_->GetOrCreateBucketSync(params)
             : QuotaError::kUnknownError;
}

void MockQuotaManagerProxy::CreateBucketForTesting(
    const blink::StorageKey& storage_key,
    const std::string& bucket_name,
    blink::mojom::StorageType storage_type,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceCallback<void(QuotaErrorOr<BucketInfo>)> callback) {
  if (mock_quota_manager_) {
    mock_quota_manager_->CreateBucketForTesting(
        storage_key, bucket_name, storage_type, std::move(callback));
  }
}

void MockQuotaManagerProxy::GetBucket(
    const blink::StorageKey& storage_key,
    const std::string& bucket_name,
    blink::mojom::StorageType type,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceCallback<void(QuotaErrorOr<BucketInfo>)> callback) {
  if (mock_quota_manager_) {
    mock_quota_manager_->GetBucket(storage_key, bucket_name, type,
                                   std::move(callback));
  }
}

void MockQuotaManagerProxy::GetBucketById(
    const BucketId& bucket_id,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceCallback<void(QuotaErrorOr<BucketInfo>)> callback) {
  if (mock_quota_manager_) {
    mock_quota_manager_->GetBucketById(bucket_id, std::move(callback));
  }
}

void MockQuotaManagerProxy::GetBucketsForStorageKey(
    const blink::StorageKey& storage_key,
    blink::mojom::StorageType type,
    bool delete_expired,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceCallback<void(QuotaErrorOr<std::set<BucketInfo>>)> callback) {
  if (mock_quota_manager_) {
    mock_quota_manager_->GetBucketsForStorageKey(
        storage_key, type, std::move(callback), delete_expired);
  } else {
    std::move(callback).Run(std::set<BucketInfo>());
  }
}

void MockQuotaManagerProxy::GetUsageAndQuota(
    const blink::StorageKey& storage_key,
    blink::mojom::StorageType type,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    QuotaManager::UsageAndQuotaCallback callback) {
  if (mock_quota_manager_) {
    mock_quota_manager_->GetUsageAndQuota(storage_key, type,
                                          std::move(callback));
  }
}

void MockQuotaManagerProxy::NotifyStorageAccessed(
    const blink::StorageKey& storage_key,
    blink::mojom::StorageType type,
    base::Time access_time) {
  ++storage_accessed_count_;
  last_notified_storage_key_ = storage_key;
  last_notified_type_ = type;
}

void MockQuotaManagerProxy::NotifyBucketAccessed(storage::BucketId bucket_id,
                                                 base::Time access_time) {
  ++bucket_accessed_count_;
  last_notified_bucket_id_ = bucket_id;
}

void MockQuotaManagerProxy::NotifyStorageModified(
    storage::QuotaClientType client_id,
    const blink::StorageKey& storage_key,
    blink::mojom::StorageType type,
    int64_t delta,
    base::Time modification_time,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceClosure callback) {
  ++storage_modified_count_;
  last_notified_storage_key_ = storage_key;
  last_notified_type_ = type;
  last_notified_delta_ = delta;
  if (mock_quota_manager_) {
    mock_quota_manager_->GetOrCreateBucketDeprecated(
        BucketInitParams::ForDefaultBucket(storage_key), type,
        base::BindLambdaForTesting(
            [this, delta, callback_task_runner,
             &callback](QuotaErrorOr<BucketInfo> result) {
              if (result.ok()) {
                mock_quota_manager_->UpdateUsage(result->ToBucketLocator().id,
                                                 delta);
              }
              callback_task_runner->PostTask(FROM_HERE, std::move(callback));
            }));
  } else if (callback)
    callback_task_runner->PostTask(FROM_HERE, std::move(callback));
}

void MockQuotaManagerProxy::NotifyBucketModified(
    storage::QuotaClientType client_id,
    storage::BucketId bucket_id,
    int64_t delta,
    base::Time modification_time,
    scoped_refptr<base::SequencedTaskRunner> callback_task_runner,
    base::OnceClosure callback) {
  ++bucket_modified_count_;
  last_notified_bucket_id_ = bucket_id;
  last_notified_bucket_delta_ = delta;
  if (mock_quota_manager_) {
    mock_quota_manager_->UpdateUsage(bucket_id, delta);
  }
  if (callback)
    callback_task_runner->PostTask(FROM_HERE, std::move(callback));
}

MockQuotaManagerProxy::~MockQuotaManagerProxy() = default;

}  // namespace storage

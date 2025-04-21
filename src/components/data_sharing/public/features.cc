// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_sharing/public/features.h"

#include "base/feature_list.h"
#include "base/time/time.h"

namespace data_sharing::features {
namespace {
const char kDataSharingDefaultUrl[] =
    "https://shared-tabs-v3-dot-googwebreview.appspot.com/chrome/tabshare/";
}

BASE_FEATURE(kDataSharingFeature,
             "DataSharing",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kDataSharingJoinOnly,
             "DataSharingJoinOnly",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<std::string> kDataSharingURL(
    &kDataSharingFeature,
    "data_sharing_url",
    kDataSharingDefaultUrl);

constexpr base::FeatureParam<base::TimeDelta>
    kDataSharingGroupDataPeriodicPollingInterval(
        &kDataSharingFeature,
        "data_sharing_group_data_periodic_polling_interval",
        base::Days(1));

}  // namespace data_sharing::features

// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_FEATURES_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace certificate_transparency {
namespace features {

COMPONENT_EXPORT(CERTIFICATE_TRANSPARENCY)
extern const base::Feature kCertificateTransparencyComponentUpdater;

}  // namespace features
}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_CT_FEATURES_H_

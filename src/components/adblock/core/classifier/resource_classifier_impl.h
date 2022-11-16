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

#ifndef COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_

#include <set>

#include "components/adblock/core/classifier/resource_classifier.h"

namespace adblock {

class ResourceClassifierImpl final : public ResourceClassifier {
 public:
  ResourceClassifierImpl() = default;

  ClassificationResult ClassifyRequest(
      const SubscriptionCollection& subscription_collection,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const SiteKey& sitekey) const final;

  ClassificationResult ClassifyPopup(
      const SubscriptionCollection& subscription_collection,
      const GURL& popup_url,
      const GURL& opener_url,
      const SiteKey& sitekey) const final;

  ClassificationResult ClassifyResponse(
      const SubscriptionCollection& subscription_collection,
      const GURL& request_url,
      const std::vector<GURL>& frame_hierarchy,
      ContentType content_type,
      const scoped_refptr<net::HttpResponseHeaders>& response_headers)
      const final;

 protected:
  friend class base::RefCountedThreadSafe<ResourceClassifierImpl>;
  ~ResourceClassifierImpl() override;

 private:
  ClassificationResult CheckHeaderFiltersMatchResponseHeaders(
      const GURL request_url,
      const std::vector<GURL> frame_hierarchy,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      std::set<HeaderFilterData> blocking_filters,
      std::set<HeaderFilterData> allowing_filters) const;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CLASSIFIER_RESOURCE_CLASSIFIER_IMPL_H_

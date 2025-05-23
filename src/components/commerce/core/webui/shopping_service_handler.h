// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_COMMERCE_CORE_WEBUI_SHOPPING_SERVICE_HANDLER_H_
#define COMPONENTS_COMMERCE_CORE_WEBUI_SHOPPING_SERVICE_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/commerce/core/mojom/shopping_service.mojom.h"
#include "components/commerce/core/product_specifications/product_specifications_set.h"
#include "components/commerce/core/subscriptions/subscriptions_manager.h"
#include "components/commerce/core/subscriptions/subscriptions_observer.h"
#include "components/optimization_guide/core/model_quality/model_quality_log_entry.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

class PrefService;

namespace optimization_guide {
class ModelQualityLogsUploaderService;
}  // namespace optimization_guide

namespace base {
class Uuid;
}

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace feature_engagement {
class Tracker;
}  // namespace feature_engagement
namespace commerce {

class ShoppingService;
struct PriceInsightsInfo;
struct ProductSpecifications;

class ShoppingServiceHandler
    : public shopping_service::mojom::ShoppingServiceHandler {
 public:
  // Handles platform specific tasks.
  class Delegate {
   public:
    Delegate() = default;
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;

    virtual ~Delegate() = default;

    virtual std::optional<GURL> GetCurrentTabUrl() = 0;

    virtual const bookmarks::BookmarkNode* GetOrAddBookmarkForCurrentUrl() = 0;

    virtual void OpenUrlInNewTab(const GURL& url) = 0;

    virtual void SwitchToOrOpenTab(const GURL& url) = 0;

    virtual void ShowFeedbackForProductSpecifications(
        const std::string& log_id) = 0;

    virtual ukm::SourceId GetCurrentTabUkmSourceId() = 0;
  };

  ShoppingServiceHandler(
      mojo::PendingReceiver<shopping_service::mojom::ShoppingServiceHandler>
          receiver,
      bookmarks::BookmarkModel* bookmark_model,
      ShoppingService* shopping_service,
      PrefService* prefs,
      feature_engagement::Tracker* tracker,
      std::unique_ptr<Delegate> delegate,
      optimization_guide::ModelQualityLogsUploaderService*
          model_quality_logs_uploader_service);
  ShoppingServiceHandler(const ShoppingServiceHandler&) = delete;
  ShoppingServiceHandler& operator=(const ShoppingServiceHandler&) = delete;
  ~ShoppingServiceHandler() override;

  // shopping_service::mojom::ShoppingServiceHandler:
  void GetProductInfoForCurrentUrl(
      GetProductInfoForCurrentUrlCallback callback) override;
  void GetProductInfoForUrl(const GURL& url,
                            GetProductInfoForUrlCallback callback) override;
  void GetPriceInsightsInfoForCurrentUrl(
      GetPriceInsightsInfoForCurrentUrlCallback callback) override;
  void GetPriceInsightsInfoForUrl(
      const GURL& url,
      GetPriceInsightsInfoForUrlCallback callback) override;
  void GetProductSpecificationsForUrls(
      const std::vector<::GURL>& urls,
      GetProductSpecificationsForUrlsCallback callback) override;
  void GetUrlInfosForProductTabs(
      GetUrlInfosForProductTabsCallback callback) override;
  void GetUrlInfosForRecentlyViewedTabs(
      GetUrlInfosForRecentlyViewedTabsCallback callback) override;
  void IsShoppingListEligible(IsShoppingListEligibleCallback callback) override;
  void GetPriceTrackingStatusForCurrentUrl(
      GetPriceTrackingStatusForCurrentUrlCallback callback) override;
  void SwitchToOrOpenTab(const GURL& url) override;
  void OpenUrlInNewTab(const GURL& url) override;
  void GetAllProductSpecificationsSets(
      GetAllProductSpecificationsSetsCallback callback) override;
  void GetProductSpecificationsSetByUuid(
      const base::Uuid& uuid,
      GetProductSpecificationsSetByUuidCallback callback) override;
  void AddProductSpecificationsSet(
      const std::string& name,
      const std::vector<GURL>& urls,
      AddProductSpecificationsSetCallback callback) override;
  void DeleteProductSpecificationsSet(const base::Uuid& uuid) override;
  void SetNameForProductSpecificationsSet(
      const base::Uuid& uuid,
      const std::string& name,
      SetNameForProductSpecificationsSetCallback callback) override;
  void SetUrlsForProductSpecificationsSet(
      const base::Uuid& uuid,
      const std::vector<GURL>& urls,
      SetUrlsForProductSpecificationsSetCallback callback) override;
  void SetProductSpecificationsUserFeedback(
      shopping_service::mojom::UserFeedback feedback) override;
  void GetProductSpecificationsFeatureState(
      GetProductSpecificationsFeatureStateCallback callback) override;

  optimization_guide::ModelQualityLogEntry*
  current_log_quality_entry_for_testing() {
    return current_log_quality_entry_.get();
  }

 private:
  void OnFetchPriceInsightsInfoForCurrentUrl(
      GetPriceInsightsInfoForCurrentUrlCallback callback,
      const GURL& url,
      const std::optional<PriceInsightsInfo>& info);
  void OnGetPriceTrackingStatusForCurrentUrl(
      GetPriceTrackingStatusForCurrentUrlCallback callback,
      bool tracked);
  void OnGetProductSpecificationsForUrls(
      std::vector<GURL> input_urls,
      GetProductSpecificationsForUrlsCallback callback,
      std::vector<uint64_t> ids,
      std::optional<ProductSpecifications> specs);

  mojo::Receiver<shopping_service::mojom::ShoppingServiceHandler> receiver_;
  // The bookmark model, shopping service and tracker will outlive this
  // implementation since it is a keyed service bound to the browser context
  // (which in turn has the same lifecycle as the browser). The web UI that
  // hosts this will be shut down prior to the rest of the browser.
  raw_ptr<bookmarks::BookmarkModel> bookmark_model_;
  raw_ptr<ShoppingService> shopping_service_;
  raw_ptr<PrefService, DanglingUntriaged> pref_service_;
  raw_ptr<feature_engagement::Tracker> tracker_;
  std::string locale_;
  std::unique_ptr<Delegate> delegate_;
  raw_ptr<optimization_guide::ModelQualityLogsUploaderService>
      model_quality_logs_uploader_service_;
  std::unique_ptr<optimization_guide::ModelQualityLogEntry>
      current_log_quality_entry_;

  base::WeakPtrFactory<ShoppingServiceHandler> weak_ptr_factory_{this};
};

}  // namespace commerce

#endif  // COMPONENTS_COMMERCE_CORE_WEBUI_SHOPPING_SERVICE_HANDLER_H_

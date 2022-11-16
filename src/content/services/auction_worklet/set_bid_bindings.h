// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SERVICES_AUCTION_WORKLET_SET_BID_BINDINGS_H_
#define CONTENT_SERVICES_AUCTION_WORKLET_SET_BID_BINDINGS_H_

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "content/services/auction_worklet/auction_v8_helper.h"
#include "content/services/auction_worklet/context_recycler.h"
#include "content/services/auction_worklet/public/mojom/bidder_worklet.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/interest_group/interest_group.h"
#include "url/gurl.h"
#include "v8/include/v8-forward.h"

namespace auction_worklet {

// Class to manage bindings for setting a bidding result. Expected to be used
// for a context managed by ContextRecycler.
class SetBidBindings : public Bindings {
 public:
  explicit SetBidBindings(AuctionV8Helper* v8_helper);
  SetBidBindings(const SetBidBindings&) = delete;
  SetBidBindings& operator=(const SetBidBindings&) = delete;
  ~SetBidBindings() override;

  // This must be called before every time this is used.
  void ReInitialize(
      base::TimeTicks start,
      bool has_top_level_seller_origin,
      const absl::optional<std::vector<blink::InterestGroup::Ad>>* ads,
      const absl::optional<std::vector<blink::InterestGroup::Ad>>*
          ad_components);

  void FillInGlobalTemplate(
      v8::Local<v8::ObjectTemplate> global_template) override;
  void Reset() override;

  bool has_bid() const { return !bid_.is_null(); }
  mojom::BidderWorkletBidPtr TakeBid();

  bool SetBid(v8::Local<v8::Value> generate_bid_result,
              std::string error_prefix,
              std::vector<std::string>& errors_out);

 private:
  static void SetBid(const v8::FunctionCallbackInfo<v8::Value>& args);

  const raw_ptr<AuctionV8Helper> v8_helper_;

  base::TimeTicks start_;
  bool has_top_level_seller_origin_ = false;
  raw_ptr<const absl::optional<std::vector<blink::InterestGroup::Ad>>> ads_ =
      nullptr;
  raw_ptr<const absl::optional<std::vector<blink::InterestGroup::Ad>>>
      ad_components_ = nullptr;

  mojom::BidderWorkletBidPtr bid_;
};

}  // namespace auction_worklet

#endif  // CONTENT_SERVICES_AUCTION_WORKLET_SET_BID_BINDINGS_H_

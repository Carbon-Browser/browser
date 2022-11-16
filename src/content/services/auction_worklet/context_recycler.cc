// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/services/auction_worklet/context_recycler.h"

#include "base/check.h"
#include "content/services/auction_worklet/for_debugging_only_bindings.h"
#include "content/services/auction_worklet/register_ad_beacon_bindings.h"
#include "content/services/auction_worklet/report_bindings.h"
#include "content/services/auction_worklet/set_bid_bindings.h"
#include "content/services/auction_worklet/set_priority_bindings.h"
#include "v8/include/v8-template.h"

namespace auction_worklet {

Bindings::Bindings() = default;
Bindings::~Bindings() = default;

ContextRecycler::ContextRecycler(AuctionV8Helper* v8_helper)
    : v8_helper_(v8_helper) {}

ContextRecycler::~ContextRecycler() = default;

void ContextRecycler::AddForDebuggingOnlyBindings() {
  DCHECK(!for_debugging_only_bindings_);
  for_debugging_only_bindings_ =
      std::make_unique<ForDebuggingOnlyBindings>(v8_helper_);
  AddBindings(for_debugging_only_bindings_.get());
}

void ContextRecycler::AddRegisterAdBeaconBindings() {
  DCHECK(!register_ad_beacon_bindings_);
  register_ad_beacon_bindings_ =
      std::make_unique<RegisterAdBeaconBindings>(v8_helper_);
  AddBindings(register_ad_beacon_bindings_.get());
}

void ContextRecycler::AddReportBindings() {
  DCHECK(!report_bindings_);
  report_bindings_ = std::make_unique<ReportBindings>(v8_helper_);
  AddBindings(report_bindings_.get());
}

void ContextRecycler::AddSetBidBindings() {
  DCHECK(!set_bid_bindings_);
  set_bid_bindings_ = std::make_unique<SetBidBindings>(v8_helper_);
  AddBindings(set_bid_bindings_.get());
}

void ContextRecycler::AddSetPriorityBindings() {
  DCHECK(!set_priority_bindings_);
  set_priority_bindings_ = std::make_unique<SetPriorityBindings>(v8_helper_);
  AddBindings(set_priority_bindings_.get());
}

void ContextRecycler::AddBindings(Bindings* bindings) {
  DCHECK(context_.IsEmpty());  // should be called before GetContext()
  bindings_list_.push_back(bindings);
}

v8::Local<v8::Context> ContextRecycler::GetContext() {
  v8::Isolate* isolate = v8_helper_->isolate();
  if (context_.IsEmpty()) {
    v8::Local<v8::ObjectTemplate> global_template =
        v8::ObjectTemplate::New(isolate);
    for (Bindings* bindings : bindings_list_)
      bindings->FillInGlobalTemplate(global_template);
    context_.Reset(isolate, v8_helper_->CreateContext(global_template));
  }

  return v8::Local<v8::Context>::New(isolate, context_);
}

void ContextRecycler::ResetForReuse() {
  for (Bindings* bindings : bindings_list_)
    bindings->Reset();
}

ContextRecyclerScope::ContextRecyclerScope(ContextRecycler& context_recycler)
    : context_recycler_(context_recycler),
      context_(context_recycler_.GetContext()),
      context_scope_(context_) {}

ContextRecyclerScope::~ContextRecyclerScope() {
  context_recycler_.ResetForReuse();
}

v8::Local<v8::Context> ContextRecyclerScope::GetContext() {
  return context_;
}

}  // namespace auction_worklet

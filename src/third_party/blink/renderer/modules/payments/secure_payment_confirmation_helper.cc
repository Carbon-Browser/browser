// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/secure_payment_confirmation_helper.h"

#include <stdint.h>

#include "base/logging.h"
#include "third_party/blink/public/mojom/payments/payment_request.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_arraybuffer_arraybufferview.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_payment_credential_instrument.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_secure_payment_confirmation_request.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/payments/secure_payment_confirmation_type_converter.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

namespace {
bool IsEmpty(const V8UnionArrayBufferOrArrayBufferView* buffer) {
  DCHECK(buffer);
  switch (buffer->GetContentType()) {
    case V8BufferSource::ContentType::kArrayBuffer:
      return buffer->GetAsArrayBuffer()->ByteLength() == 0;
    case V8BufferSource::ContentType::kArrayBufferView:
      return buffer->GetAsArrayBufferView()->byteLength() == 0;
  }
}
}  // namespace

// static
::payments::mojom::blink::SecurePaymentConfirmationRequestPtr
SecurePaymentConfirmationHelper::ParseSecurePaymentConfirmationData(
    const ScriptValue& input,
    ExecutionContext& execution_context,
    ExceptionState& exception_state) {
  DCHECK(!input.IsEmpty());
  SecurePaymentConfirmationRequest* request =
      NativeValueTraits<SecurePaymentConfirmationRequest>::NativeValue(
          input.GetIsolate(), input.V8Value(), exception_state);
  if (exception_state.HadException())
    return nullptr;

  if (request->credentialIds().IsEmpty()) {
    exception_state.ThrowRangeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"credentialIds\" field.");
    return nullptr;
  }
  for (const V8UnionArrayBufferOrArrayBufferView* id :
       request->credentialIds()) {
    if (IsEmpty(id)) {
      exception_state.ThrowRangeError(
          "The \"secure-payment-confirmation\" method requires that elements "
          "in the \"credentialIds\" field are non-empty.");
      return nullptr;
    }
  }
  if (IsEmpty(request->challenge())) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"challenge\" field.");
    return nullptr;
  }

  if (request->instrument()->displayName().IsEmpty()) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"instrument.displayName\" field.");
    return nullptr;
  }
  if (request->instrument()->icon().IsEmpty()) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"instrument.icon\" field.");
    return nullptr;
  }
  if (!KURL(request->instrument()->icon()).IsValid()) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a valid URL in "
        "the \"instrument.icon\" field.");
    return nullptr;
  }
  // TODO(https://crbug.com/1342686): Check that rpId is a valid domain.
  if (request->rpId().IsEmpty()) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"rpId\" field.");
    return nullptr;
  }
  if ((!request->hasPayeeOrigin() && !request->hasPayeeName()) ||
      (request->hasPayeeOrigin() && request->payeeOrigin().IsEmpty()) ||
      (request->hasPayeeName() && request->payeeName().IsEmpty())) {
    exception_state.ThrowTypeError(
        "The \"secure-payment-confirmation\" method requires a non-empty "
        "\"payeeOrigin\" or \"payeeName\" field.");
    return nullptr;
  }
  if (request->hasPayeeOrigin()) {
    KURL payee_url(request->payeeOrigin());
    if (!payee_url.IsValid() || !payee_url.ProtocolIs("https")) {
      exception_state.ThrowTypeError(
          "The \"secure-payment-confirmation\" method requires a valid HTTPS "
          "URL in the \"payeeOrigin\" field.");
      return nullptr;
    }
  }

  // Opt Out should only be carried through if the flag is enabled.
  if (request->hasShowOptOut() &&
      !blink::RuntimeEnabledFeatures::SecurePaymentConfirmationOptOutEnabled(
          &execution_context)) {
    request->setShowOptOut(false);
  }

  return mojo::ConvertTo<
      payments::mojom::blink::SecurePaymentConfirmationRequestPtr>(request);
}

}  // namespace blink

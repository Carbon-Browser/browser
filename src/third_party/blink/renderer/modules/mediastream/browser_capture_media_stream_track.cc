// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediastream/browser_capture_media_stream_track.h"

#include "base/guid.h"
#include "base/metrics/histogram_functions.h"
#include "base/token.h"
#include "build/build_config.h"
#include "media/capture/mojom/video_capture_types.mojom-blink.h"
#include "media/capture/video_capture_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/web/modules/mediastream/media_stream_video_source.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/region_capture_crop_id.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class CropToResult {
  kResolved = 0,
  kUnsupportedPlatform = 1,
  kInvalidCropTargetFormat = 2,
  kRejectedWithErrorGeneric = 3,
  kRejectedWithUnsupportedCaptureDevice = 4,
  kRejectedWithErrorUnknownDeviceId_DEPRECATED = 5,
  kRejectedWithNotImplemented = 6,
  kNonIncreasingCropVersion = 7,
  kInvalidCropTarget = 8,
  kMaxValue = kInvalidCropTarget
};

void RecordUma(CropToResult result) {
  base::UmaHistogramEnumeration("Media.RegionCapture.CropTo.Result", result);
}

#if !BUILDFLAG(IS_ANDROID)

// TODO(crbug.com/1332628): Remove this flag once it's clear it's not necessary.
const base::Feature kCropTopPromiseWaitsForFirstFrame{
    "CropTopPromiseWaitsForFirstFrame", base::FEATURE_ENABLED_BY_DEFAULT};

// If crop_id is the empty string, returns an empty base::Token.
// If crop_id is a valid UUID, returns a base::Token representing the ID.
// Otherwise, returns nullopt.
absl::optional<base::Token> CropIdStringToToken(const String& crop_id) {
  if (crop_id.IsEmpty()) {
    return base::Token();
  }
  if (!crop_id.ContainsOnlyASCIIOrEmpty()) {
    return absl::nullopt;
  }
  const base::GUID guid = base::GUID::ParseCaseInsensitive(crop_id.Ascii());
  return guid.is_valid() ? absl::make_optional<base::Token>(GUIDToToken(guid))
                         : absl::nullopt;
}

void RaiseCropException(ScriptPromiseResolver* resolver,
                        DOMExceptionCode exception_code,
                        const WTF::String& exception_text) {
  resolver->Reject(
      MakeGarbageCollected<DOMException>(exception_code, exception_text));
}

void ResolveCropPromiseHelper(ScriptPromiseResolver* resolver,
                              media::mojom::CropRequestResult result) {
  DCHECK(IsMainThread());

  if (!resolver) {
    return;
  }

  switch (result) {
    case media::mojom::CropRequestResult::kSuccess:
      RecordUma(CropToResult::kResolved);
      // TODO(crbug.com/1247761): Delay reporting success to the Web-application
      // until "seeing" the last frame cropped to the previous crop-target.
      resolver->Resolve();
      return;
    case media::mojom::CropRequestResult::kErrorGeneric:
      RecordUma(CropToResult::kRejectedWithErrorGeneric);
      RaiseCropException(resolver, DOMExceptionCode::kAbortError,
                         "Unknown error.");
      return;
    case media::mojom::CropRequestResult::kUnsupportedCaptureDevice:
      // Note that this is an unsupported device; not an unsupported Element.
      // This should essentially not happen. If it happens, it indicates
      // something in the capture pipeline has been changed.
      RecordUma(CropToResult::kRejectedWithUnsupportedCaptureDevice);
      RaiseCropException(resolver, DOMExceptionCode::kAbortError,
                         "Unsupported device.");
      return;
    case media::mojom::CropRequestResult::kNotImplemented:
      // Unimplemented codepath reached, OTHER than lacking support for
      // a specific Element subtype.
      RecordUma(CropToResult::kRejectedWithNotImplemented);
      RaiseCropException(resolver, DOMExceptionCode::kOperationError,
                         "Not implemented.");
      return;
    case media::mojom::CropRequestResult::kNonIncreasingCropVersion:
      // This should rarely happen, as the browser process would issue
      // a BadMessage in this case. But if that message has to hop from
      // the IO thread to the UI thread, it could theoretically happen
      // that Blink receives this callback before being killed, so we
      // can't quite DCHECK this.
      RecordUma(CropToResult::kNonIncreasingCropVersion);
      RaiseCropException(resolver, DOMExceptionCode::kAbortError,
                         "Non-increasing crop version.");
      return;
    case media::mojom::CropRequestResult::kInvalidCropTarget:
      RecordUma(CropToResult::kNonIncreasingCropVersion);
      RaiseCropException(resolver, DOMExceptionCode::kNotAllowedError,
                         "Invalid CropTarget.");
      return;
  }

  NOTREACHED();
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

BrowserCaptureMediaStreamTrack::BrowserCaptureMediaStreamTrack(
    ExecutionContext* execution_context,
    MediaStreamComponent* component,
    base::OnceClosure callback,
    const String& descriptor_id,
    bool is_clone)
    : BrowserCaptureMediaStreamTrack(execution_context,
                                     component,
                                     component->Source()->GetReadyState(),
                                     std::move(callback),
                                     descriptor_id,
                                     is_clone) {}

BrowserCaptureMediaStreamTrack::BrowserCaptureMediaStreamTrack(
    ExecutionContext* execution_context,
    MediaStreamComponent* component,
    MediaStreamSource::ReadyState ready_state,
    base::OnceClosure callback,
    const String& descriptor_id,
    bool is_clone)
    : FocusableMediaStreamTrack(execution_context,
                                component,
                                ready_state,
                                std::move(callback),
                                descriptor_id,
                                is_clone) {}

#if !BUILDFLAG(IS_ANDROID)
void BrowserCaptureMediaStreamTrack::Trace(Visitor* visitor) const {
  visitor->Trace(pending_promises_);
  FocusableMediaStreamTrack::Trace(visitor);
}
#endif  // !BUILDFLAG(IS_ANDROID)

ScriptPromise BrowserCaptureMediaStreamTrack::cropTo(
    ScriptState* script_state,
    CropTarget* crop_target,
    ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  const String crop_id(crop_target ? crop_target->GetCropId() : String());

  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

#if BUILDFLAG(IS_ANDROID)
  RecordUma(CropToResult::kUnsupportedPlatform);
  resolver->Reject(MakeGarbageCollected<DOMException>(
      DOMExceptionCode::kUnknownError, "Not supported on Android."));
  return promise;
#else

  const absl::optional<base::Token> crop_id_token =
      CropIdStringToToken(crop_id);
  if (!crop_id_token.has_value()) {
    RecordUma(CropToResult::kInvalidCropTargetFormat);
    resolver->Reject(MakeGarbageCollected<DOMException>(
        DOMExceptionCode::kUnknownError, "Invalid crop-ID."));
    return promise;
  }

  MediaStreamComponent* const component = Component();
  DCHECK(component);

  MediaStreamSource* const source = component->Source();
  DCHECK(component->Source());
  // We don't currently instantiate BrowserCaptureMediaStreamTrack for audio
  // tracks. If we do in the future, we'll have to raise an exception if
  // cropTo() is called on a non-video track.
  DCHECK_EQ(source->GetType(), MediaStreamSource::kTypeVideo);

  MediaStreamVideoSource* const native_source =
      MediaStreamVideoSource::GetVideoSource(source);
  MediaStreamTrackPlatform* const native_track =
      MediaStreamTrackPlatform::GetTrack(WebMediaStreamTrack(component));
  if (!native_source || !native_track) {
    // TODO(crbug.com/1266378): Use dedicate UMA values.
    RecordUma(CropToResult::kRejectedWithErrorGeneric);
    resolver->Reject(MakeGarbageCollected<DOMException>(
        DOMExceptionCode::kUnknownError, "Native/platform track missing."));
    return promise;
  }

  // TODO(crbug.com/1332628): Instead of using GetNextCropVersion(), move the
  // ownership of the Promises from this->pending_promises_ into native_source.
  const absl::optional<uint32_t> optional_crop_version =
      native_source->GetNextCropVersion();
  if (!optional_crop_version.has_value()) {
    resolver->Reject(MakeGarbageCollected<DOMException>(
        DOMExceptionCode::kOperationError,
        "Can't change crop-target while clones exist."));
    return promise;
  }
  const uint32_t crop_version = optional_crop_version.value();

  pending_promises_.Set(crop_version,
                        MakeGarbageCollected<CropPromiseInfo>(resolver));

  // Register for a one-off notification when the first frame cropped
  // to the new crop-target is observed.
  native_track->AddCropVersionCallback(
      crop_version,
      WTF::Bind(&BrowserCaptureMediaStreamTrack::OnCropVersionObserved,
                WrapWeakPersistent(this), crop_version));

  native_source->Crop(
      crop_id_token.value(), crop_version,
      WTF::Bind(&BrowserCaptureMediaStreamTrack::OnResultFromBrowserProcess,
                WrapWeakPersistent(this), crop_version));

  return promise;
#endif
}

BrowserCaptureMediaStreamTrack* BrowserCaptureMediaStreamTrack::clone(
    ScriptState* script_state) {
  // Instantiate the clone.
  BrowserCaptureMediaStreamTrack* cloned_track =
      MakeGarbageCollected<BrowserCaptureMediaStreamTrack>(
          ExecutionContext::From(script_state), Component()->Clone(),
          GetReadyState(), base::DoNothing(), descriptor_id(),
          /*is_clone=*/true);

  // Copy state. (Note: Invokes FocusableMediaStreamTrack::CloneInternal().)
  CloneInternal(cloned_track);

  return cloned_track;
}

#if !BUILDFLAG(IS_ANDROID)
void BrowserCaptureMediaStreamTrack::OnResultFromBrowserProcess(
    uint32_t crop_version,
    media::mojom::CropRequestResult result) {
  DCHECK(IsMainThread());
  DCHECK_GT(crop_version, 0u);

  const auto iter = pending_promises_.find(crop_version);
  if (iter == pending_promises_.end()) {
    return;
  }
  CropPromiseInfo* const info = iter->value;

  DCHECK(!info->crop_result.has_value()) << "Invoked twice.";
  info->crop_result = result;

  MaybeFinalizeCropPromise(iter);
}

void BrowserCaptureMediaStreamTrack::OnCropVersionObserved(
    uint32_t crop_version) {
  DCHECK(IsMainThread());
  DCHECK_GT(crop_version, 0u);

  if (!base::FeatureList::IsEnabled(kCropTopPromiseWaitsForFirstFrame)) {
    return;
  }

  const auto iter = pending_promises_.find(crop_version);
  if (iter == pending_promises_.end()) {
    return;
  }
  CropPromiseInfo* const info = iter->value;

  DCHECK(!info->crop_version_observed) << "Invoked twice.";
  info->crop_version_observed = true;

  MaybeFinalizeCropPromise(iter);
}

void BrowserCaptureMediaStreamTrack::MaybeFinalizeCropPromise(
    BrowserCaptureMediaStreamTrack::PromiseMapIterator iter) {
  DCHECK(IsMainThread());
  DCHECK_NE(iter, pending_promises_.end());

  CropPromiseInfo* const info = iter->value;

  if (!info->crop_result.has_value()) {
    return;
  }

  const media::mojom::CropRequestResult result = info->crop_result.value();

  // Failure can be reported immediately, but success is only reported once
  // the new crop-version is observed.
  if (result == media::mojom::CropRequestResult::kSuccess &&
      base::FeatureList::IsEnabled(kCropTopPromiseWaitsForFirstFrame) &&
      !info->crop_version_observed) {
    return;
  }

  // When `result == kSuccess`, the callback will be removed by the track
  // itself as it invokes it. For failure, we remove the callback immediately,
  // since there's no need to wait.
  if (result != media::mojom::CropRequestResult::kSuccess) {
    MediaStreamTrackPlatform* const native_track =
        MediaStreamTrackPlatform::GetTrack(WebMediaStreamTrack(Component()));
    if (native_track) {
      native_track->RemoveCropVersionCallback(iter->key);
    }
  }

  ScriptPromiseResolver* const resolver = info->promise_resolver;
  pending_promises_.erase(iter);
  ResolveCropPromiseHelper(resolver, result);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace blink

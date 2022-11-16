// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/webgpu_swap_buffer_provider.h"

#include "build/build_config.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "gpu/command_buffer/client/shared_image_interface.h"
#include "gpu/command_buffer/client/webgpu_interface.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/command_buffer/common/shared_image_usage.h"

namespace blink {

namespace {
viz::ResourceFormat WGPUFormatToViz(WGPUTextureFormat format) {
  switch (format) {
    case WGPUTextureFormat_BGRA8Unorm:
      return viz::BGRA_8888;
    case WGPUTextureFormat_RGBA8Unorm:
      return viz::RGBA_8888;
    case WGPUTextureFormat_RGBA16Float:
      return viz::RGBA_F16;
    default:
      NOTREACHED();
      return viz::RGBA_8888;
  }
}

}  // namespace

WebGPUSwapBufferProvider::WebGPUSwapBufferProvider(
    Client* client,
    scoped_refptr<DawnControlClientHolder> dawn_control_client,
    WGPUDevice device,
    WGPUTextureUsage usage,
    WGPUTextureFormat format)
    : dawn_control_client_(dawn_control_client),
      client_(client),
      device_(device) {
  // Create a layer that will be used by the canvas and will ask for a
  // SharedImage each frame.
  layer_ = cc::TextureLayer::CreateForMailbox(this);

  layer_->SetIsDrawable(true);
  layer_->SetBlendBackgroundColor(false);
  layer_->SetNearestNeighbor(false);
  layer_->SetFlipped(false);
  // TODO(cwallez@chromium.org): These flags aren't taken into account when the
  // layer is promoted to an overlay. Make sure we have fallback / emulation
  // paths to keep the rendering correct in that cases.
  layer_->SetContentsOpaque(true);
  layer_->SetPremultipliedAlpha(true);

  dawn_control_client_->GetProcs().deviceReference(device_);

  // Initialize the texture descriptor. Only the size will change after this.
  texture_desc_ = {};
  texture_desc_.dimension = WGPUTextureDimension_2D;
  texture_desc_.mipLevelCount = 1;
  texture_desc_.sampleCount = 1;
  texture_desc_.usage = usage;
  texture_desc_.format = format;
}

WebGPUSwapBufferProvider::~WebGPUSwapBufferProvider() {
  Neuter();
  dawn_control_client_->GetProcs().deviceRelease(device_);
  device_ = nullptr;
}

viz::ResourceFormat WebGPUSwapBufferProvider::Format() const {
  return WGPUFormatToViz(texture_desc_.format);
}

const gfx::Size& WebGPUSwapBufferProvider::Size() const {
  if (current_swap_buffer_)
    return current_swap_buffer_->size;

  static constexpr gfx::Size kEmpty;
  return kEmpty;
}

cc::Layer* WebGPUSwapBufferProvider::CcLayer() {
  DCHECK(!neutered_);
  return layer_.get();
}

void WebGPUSwapBufferProvider::SetFilterQuality(
    cc::PaintFlags::FilterQuality filter_quality) {
  if (layer_) {
    layer_->SetNearestNeighbor(filter_quality ==
                               cc::PaintFlags::FilterQuality::kNone);
  }
}

std::tuple<uint32_t, bool>
WebGPUSwapBufferProvider::GetTextureTargetAndOverlayCandidacy() const {
// On macOS, shared images are backed by IOSurfaces that can only be used with
// OpenGL via the rectangle texture target and are overlay candidates. Every
// other shared image implementation is implemented on OpenGL via some form of
// eglSurface and eglBindTexImage (on ANGLE or system drivers) so they use the
// 2D texture target and cannot always be overlay candidates.
#if BUILDFLAG(IS_MAC)
  const uint32_t texture_target = gpu::GetPlatformSpecificTextureTarget();
  const bool is_overlay_candidate = true;
#else
  const uint32_t texture_target = GL_TEXTURE_2D;
  const bool is_overlay_candidate = false;
#endif

  return std::make_tuple(texture_target, is_overlay_candidate);
}

uint32_t WebGPUSwapBufferProvider::GetTextureTarget() const {
  return std::get<0>(GetTextureTargetAndOverlayCandidacy());
}
bool WebGPUSwapBufferProvider::IsOverlayCandidate() const {
  return std::get<1>(GetTextureTargetAndOverlayCandidacy());
}

void WebGPUSwapBufferProvider::ReleaseWGPUTextureAccessIfNeeded(
    bool for_transfer) {
  if (current_swap_buffer_) {
    if (auto context_provider = GetContextProviderWeakPtr()) {
      gpu::webgpu::WebGPUInterface* webgpu =
          context_provider->ContextProvider()->WebGPUInterface();

      // Dissociate mailbox to avoid memory leaks.
      if (wire_device_id_ && wire_texture_id_) {
        if (for_transfer) {
          // Give client a chance to do something before the dissociation.
          DCHECK(client_);
          client_->OnTextureTransferred();

          webgpu->DissociateMailboxForPresent(
              wire_device_id_, wire_device_generation_, wire_texture_id_,
              wire_texture_generation_);
        } else {
          webgpu->DissociateMailbox(wire_texture_id_, wire_texture_generation_);
        }
        wire_device_id_ = 0;
        wire_device_generation_ = 0;
        wire_texture_id_ = 0;
        wire_texture_generation_ = 0;
      }

      // Ensure we wait for previous WebGPU commands before destroying the
      // shared image.
      webgpu->GenUnverifiedSyncTokenCHROMIUM(
          current_swap_buffer_->access_finished_token.GetData());
    }
  }
}

void WebGPUSwapBufferProvider::DiscardCurrentSwapBuffer() {
  ReleaseWGPUTextureAccessIfNeeded(/*for_transfer=*/false);
  current_swap_buffer_ = nullptr;
}

void WebGPUSwapBufferProvider::Neuter() {
  if (neutered_) {
    return;
  }

  if (layer_) {
    layer_->ClearClient();
    layer_ = nullptr;
  }

  DiscardCurrentSwapBuffer();

  client_ = nullptr;
  neutered_ = true;
}

std::unique_ptr<WebGPUSwapBufferProvider::SwapBuffer>
WebGPUSwapBufferProvider::NewOrRecycledSwapBuffer(
    gpu::SharedImageInterface* sii,
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider,
    const gfx::Size& size,
    SkAlphaType alpha_mode) {
  // Recycled SwapBuffers must be the same size.
  if (!unused_swap_buffers_.IsEmpty() &&
      unused_swap_buffers_.back()->size != size) {
    unused_swap_buffers_.clear();
  }

  if (unused_swap_buffers_.IsEmpty()) {
    gpu::Mailbox mailbox = sii->CreateSharedImage(
        Format(), size, gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
        alpha_mode,
        gpu::SHARED_IMAGE_USAGE_WEBGPU |
            gpu::SHARED_IMAGE_USAGE_WEBGPU_SWAP_CHAIN_TEXTURE |
            gpu::SHARED_IMAGE_USAGE_DISPLAY,
        gpu::kNullSurfaceHandle);
    gpu::SyncToken creation_token = sii->GenUnverifiedSyncToken();

    unused_swap_buffers_.push_back(std::make_unique<SwapBuffer>(
        std::move(context_provider), mailbox, creation_token, size));
    DCHECK_EQ(unused_swap_buffers_.back()->size, size);
  }

  std::unique_ptr<SwapBuffer> swap_buffer =
      std::move(unused_swap_buffers_.back());
  unused_swap_buffers_.pop_back();

  DCHECK_EQ(swap_buffer->size, size);
  return swap_buffer;
}

void WebGPUSwapBufferProvider::RecycleSwapBuffer(
    std::unique_ptr<SwapBuffer> swap_buffer) {
  // We don't want to keep an arbitrary large number of swap buffers.
  if (unused_swap_buffers_.size() >
      static_cast<unsigned int>(kMaxRecycledSwapBuffers))
    return;

  unused_swap_buffers_.push_back(std::move(swap_buffer));
}

WGPUTexture WebGPUSwapBufferProvider::GetNewTexture(const gfx::Size& size,
                                                    SkAlphaType alpha_mode) {
  DCHECK(!wire_texture_id_);
  auto context_provider = GetContextProviderWeakPtr();
  if (!context_provider) {
    return nullptr;
  }

  gpu::webgpu::WebGPUInterface* webgpu =
      context_provider->ContextProvider()->WebGPUInterface();

  // Create a new swap buffer.
  current_swap_buffer_ = NewOrRecycledSwapBuffer(
      context_provider->ContextProvider()->SharedImageInterface(),
      context_provider, size, alpha_mode);

  // Ensure the shared image is allocated and not in use service-side before
  // working with it
  webgpu->WaitSyncTokenCHROMIUM(
      current_swap_buffer_->access_finished_token.GetConstData());

  // Associate the mailbox to a dawn_wire client DawnTexture object. Pass in a
  // complete descriptor of the texture so that reflection on GPUTexture from
  // canvases gives the correct result.
  texture_desc_.size = {static_cast<uint32_t>(size.width()),
                        static_cast<uint32_t>(size.height()), 1};

  gpu::webgpu::ReservedTexture reservation =
      webgpu->ReserveTexture(device_, &texture_desc_);
  DCHECK(reservation.texture);
  wire_device_id_ = reservation.deviceId;
  wire_device_generation_ = reservation.deviceGeneration;
  wire_texture_id_ = reservation.id;
  wire_texture_generation_ = reservation.generation;

  webgpu->AssociateMailbox(
      wire_device_id_, wire_device_generation_, wire_texture_id_,
      wire_texture_generation_, texture_desc_.usage,
      gpu::webgpu::WEBGPU_MAILBOX_DISCARD,
      reinterpret_cast<GLbyte*>(&current_swap_buffer_->mailbox));

  // When the page request a texture it means we'll need to present it on the
  // next animation frame.
  layer_->SetNeedsDisplay();

  return reservation.texture;
}

WebGPUSwapBufferProvider::WebGPUMailboxTextureAndSize
WebGPUSwapBufferProvider::GetLastWebGPUMailboxTextureAndSize() const {
  auto context_provider = GetContextProviderWeakPtr();
  if (!last_swap_buffer_ || !context_provider)
    return WebGPUMailboxTextureAndSize(nullptr, gfx::Size());

  return WebGPUMailboxTextureAndSize(
      WebGPUMailboxTexture::FromExistingMailbox(
          dawn_control_client_, device_,
          static_cast<WGPUTextureUsage>(texture_desc_.usage),
          last_swap_buffer_->mailbox, last_swap_buffer_->access_finished_token,
          gpu::webgpu::WEBGPU_MAILBOX_NONE),
      last_swap_buffer_->size);
}

base::WeakPtr<WebGraphicsContext3DProviderWrapper>
WebGPUSwapBufferProvider::GetContextProviderWeakPtr() const {
  return dawn_control_client_->GetContextProviderWeakPtr();
}

bool WebGPUSwapBufferProvider::PrepareTransferableResource(
    cc::SharedBitmapIdRegistrar* bitmap_registrar,
    viz::TransferableResource* out_resource,
    viz::ReleaseCallback* out_release_callback) {
  DCHECK(!neutered_);
  if (!current_swap_buffer_ || neutered_ || !GetContextProviderWeakPtr()) {
    return false;
  }

  ReleaseWGPUTextureAccessIfNeeded(/*for_transfer=*/true);

  // Populate the output resource
  *out_resource = viz::TransferableResource::MakeGL(
      current_swap_buffer_->mailbox, GL_LINEAR, GetTextureTarget(),
      current_swap_buffer_->access_finished_token, current_swap_buffer_->size,
      IsOverlayCandidate());
  out_resource->color_space = gfx::ColorSpace::CreateSRGB();
  out_resource->format = Format();

  // This holds a ref on the SwapBuffers that will keep it alive until the
  // mailbox is released (and while the release callback is running).
  *out_release_callback =
      WTF::Bind(&WebGPUSwapBufferProvider::MailboxReleased,
                scoped_refptr<WebGPUSwapBufferProvider>(this),
                std::move(current_swap_buffer_));

  return true;
}

bool WebGPUSwapBufferProvider::CopyToVideoFrame(
    WebGraphicsContext3DVideoFramePool* frame_pool,
    SourceDrawingBuffer src_buffer,
    const gfx::ColorSpace& dst_color_space,
    WebGraphicsContext3DVideoFramePool::FrameReadyCallback callback) {
  DCHECK(!neutered_);
  if (neutered_ || !GetContextProviderWeakPtr()) {
    return false;
  }

  DCHECK(frame_pool);

  auto* frame_pool_ri = frame_pool->GetRasterInterface();
  DCHECK(frame_pool_ri);

  // Copy kFrontBuffer to a video frame is not supported
  DCHECK_EQ(src_buffer, kBackBuffer);

  // For a conversion from swap buffer's texture to video frame, we do it
  // using WebGraphicsContext3DVideoFramePool's graphics context. Thus, we
  // need to release WebGPU/Dawn's context's access to the texture.
  ReleaseWGPUTextureAccessIfNeeded(/*for_transfer=*/true);

  gpu::MailboxHolder mailbox_holder(current_swap_buffer_->mailbox,
                                    current_swap_buffer_->access_finished_token,
                                    GetTextureTarget());

  auto success = frame_pool->CopyRGBATextureToVideoFrame(
      Format(), current_swap_buffer_->size, gfx::ColorSpace::CreateSRGB(),
      kTopLeft_GrSurfaceOrigin, mailbox_holder, dst_color_space,
      std::move(callback));

  // Subsequent access to this swap buffer (either webgpu or compositor) must
  // wait for the copy operation to finish.
  frame_pool_ri->GenUnverifiedSyncTokenCHROMIUM(
      current_swap_buffer_->access_finished_token.GetData());

  return success;
}

void WebGPUSwapBufferProvider::MailboxReleased(
    std::unique_ptr<SwapBuffer> swap_buffer,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  // Update the SyncToken to ensure that we will wait for it even if we
  // immediately destroy this buffer.
  swap_buffer->access_finished_token = sync_token;

  if (lost_resource)
    return;

  if (last_swap_buffer_) {
    RecycleSwapBuffer(std::move(last_swap_buffer_));
  }

  last_swap_buffer_ = std::move(swap_buffer);
}

WebGPUSwapBufferProvider::SwapBuffer::SwapBuffer(
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider,
    gpu::Mailbox mailbox,
    gpu::SyncToken creation_token,
    gfx::Size size)
    : size(size),
      mailbox(mailbox),
      context_provider(context_provider),
      access_finished_token(creation_token) {}

WebGPUSwapBufferProvider::SwapBuffer::~SwapBuffer() {
  if (context_provider) {
    gpu::SharedImageInterface* sii =
        context_provider->ContextProvider()->SharedImageInterface();
    sii->DestroySharedImage(access_finished_token, mailbox);
  }
}

gpu::Mailbox WebGPUSwapBufferProvider::GetCurrentMailboxForTesting() const {
  DCHECK(current_swap_buffer_);
  return current_swap_buffer_->mailbox;
}
}  // namespace blink

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/test_image_backing.h"
#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/skia/include/core/SkPromiseImageTexture.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/mock/GrMockTypes.h"

namespace gpu {
namespace {
class TestGLTextureImageRepresentation : public GLTextureImageRepresentation {
 public:
  TestGLTextureImageRepresentation(SharedImageManager* manager,
                                   SharedImageBacking* backing,
                                   MemoryTypeTracker* tracker,
                                   gles2::Texture* texture)
      : GLTextureImageRepresentation(manager, backing, tracker),
        texture_(texture) {}

  gles2::Texture* GetTexture() override { return texture_; }
  bool BeginAccess(GLenum mode) override {
    return static_cast<TestImageBacking*>(backing())->can_access();
  }

 private:
  const raw_ptr<gles2::Texture> texture_;
};

class TestGLTexturePassthroughImageRepresentation
    : public GLTexturePassthroughImageRepresentation {
 public:
  TestGLTexturePassthroughImageRepresentation(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker,
      scoped_refptr<gles2::TexturePassthrough> texture)
      : GLTexturePassthroughImageRepresentation(manager, backing, tracker),
        texture_(std::move(texture)) {}

  const scoped_refptr<gles2::TexturePassthrough>& GetTexturePassthrough()
      override {
    return texture_;
  }
  bool BeginAccess(GLenum mode) override {
    return static_cast<TestImageBacking*>(backing())->can_access();
  }

 private:
  const scoped_refptr<gles2::TexturePassthrough> texture_;
};

class TestSkiaImageRepresentation : public SkiaImageRepresentation {
 public:
  TestSkiaImageRepresentation(SharedImageManager* manager,
                              SharedImageBacking* backing,
                              MemoryTypeTracker* tracker)
      : SkiaImageRepresentation(manager, backing, tracker) {}

 protected:
  sk_sp<SkSurface> BeginWriteAccess(
      int final_msaa_count,
      const SkSurfaceProps& surface_props,
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores) override {
    if (!static_cast<TestImageBacking*>(backing())->can_access()) {
      return nullptr;
    }
    SkSurfaceProps props = skia::LegacyDisplayGlobals::GetSkSurfaceProps();
    return SkSurface::MakeRasterN32Premul(size().width(), size().height(),
                                          &props);
  }
  sk_sp<SkPromiseImageTexture> BeginWriteAccess(
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores,
      std::unique_ptr<GrBackendSurfaceMutableState>* end_state) override {
    if (!static_cast<TestImageBacking*>(backing())->can_access()) {
      return nullptr;
    }
    GrBackendTexture backend_tex(size().width(), size().height(),
                                 GrMipMapped::kNo, GrMockTextureInfo());
    return SkPromiseImageTexture::Make(backend_tex);
  }
  void EndWriteAccess(sk_sp<SkSurface> surface) override {}
  sk_sp<SkPromiseImageTexture> BeginReadAccess(
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores) override {
    if (!static_cast<TestImageBacking*>(backing())->can_access()) {
      return nullptr;
    }
    GrBackendTexture backend_tex(size().width(), size().height(),
                                 GrMipMapped::kNo, GrMockTextureInfo());
    return SkPromiseImageTexture::Make(backend_tex);
  }
  void EndReadAccess() override {}
};

class TestDawnImageRepresentation : public DawnImageRepresentation {
 public:
  TestDawnImageRepresentation(SharedImageManager* manager,
                              SharedImageBacking* backing,
                              MemoryTypeTracker* tracker)
      : DawnImageRepresentation(manager, backing, tracker) {}

  WGPUTexture BeginAccess(WGPUTextureUsage usage) override {
    if (!static_cast<TestImageBacking*>(backing())->can_access()) {
      return nullptr;
    }

    // Return a dummy value.
    return reinterpret_cast<WGPUTexture>(203);
  }

  void EndAccess() override {}
};

class TestOverlayImageRepresentation : public OverlayImageRepresentation {
 public:
  TestOverlayImageRepresentation(SharedImageManager* manager,
                                 SharedImageBacking* backing,
                                 MemoryTypeTracker* tracker)
      : OverlayImageRepresentation(manager, backing, tracker) {}

  bool BeginReadAccess(gfx::GpuFenceHandle& acquire_fence) override {
    return true;
  }
  void EndReadAccess(gfx::GpuFenceHandle release_fence) override {}
  gl::GLImage* GetGLImage() override { return nullptr; }
};

}  // namespace

TestImageBacking::TestImageBacking(const Mailbox& mailbox,
                                   viz::ResourceFormat format,
                                   const gfx::Size& size,
                                   const gfx::ColorSpace& color_space,
                                   GrSurfaceOrigin surface_origin,
                                   SkAlphaType alpha_type,
                                   uint32_t usage,
                                   size_t estimated_size,
                                   GLuint texture_id)
    : SharedImageBacking(mailbox,
                         format,
                         size,
                         color_space,
                         surface_origin,
                         alpha_type,
                         usage,
                         estimated_size,
                         false /* is_thread_safe */),
      service_id_(texture_id) {
  texture_ = new gles2::Texture(service_id_);
  texture_->SetLightweightRef();
  texture_->SetTarget(GL_TEXTURE_2D, 1);
  texture_->set_min_filter(GL_LINEAR);
  texture_->set_mag_filter(GL_LINEAR);
  texture_->set_wrap_t(GL_CLAMP_TO_EDGE);
  texture_->set_wrap_s(GL_CLAMP_TO_EDGE);
  texture_->SetLevelInfo(GL_TEXTURE_2D, 0, GLInternalFormat(format),
                         size.width(), size.height(), 1, 0,
                         GLDataFormat(format), GLDataType(format), gfx::Rect());
  texture_->SetImmutable(true, true);
  texture_passthrough_ = base::MakeRefCounted<gles2::TexturePassthrough>(
      service_id_, GL_TEXTURE_2D);
}

TestImageBacking::TestImageBacking(const Mailbox& mailbox,
                                   viz::ResourceFormat format,
                                   const gfx::Size& size,
                                   const gfx::ColorSpace& color_space,
                                   GrSurfaceOrigin surface_origin,
                                   SkAlphaType alpha_type,
                                   uint32_t usage,
                                   size_t estimated_size)
    : TestImageBacking(mailbox,
                       format,
                       size,
                       color_space,
                       surface_origin,
                       alpha_type,
                       usage,
                       estimated_size,
                       203 /* texture_id */) {
  // Using a dummy |texture_id|, so lose our context so we don't do anything
  // real with it.
  OnContextLost();
}

TestImageBacking::~TestImageBacking() {
  // Pretend our context is lost to avoid actual cleanup in |texture_| or
  // |passthrough_texture_|.
  texture_->RemoveLightweightRef(false /* have_context */);
  texture_passthrough_->MarkContextLost();
  texture_passthrough_.reset();

  if (have_context())
    glDeleteTextures(1, &service_id_);
}

bool TestImageBacking::GetUploadFromMemoryCalledAndReset() {
  return std::exchange(upload_from_memory_called_, false);
}

SharedImageBackingType TestImageBacking::GetType() const {
  return SharedImageBackingType::kTest;
}

gfx::Rect TestImageBacking::ClearedRect() const {
  return texture_->GetLevelClearedRect(texture_->target(), 0);
}

void TestImageBacking::SetClearedRect(const gfx::Rect& cleared_rect) {
  texture_->SetLevelClearedRect(texture_->target(), 0, cleared_rect);
}

bool TestImageBacking::UploadFromMemory(const SkPixmap& pixmap) {
  upload_from_memory_called_ = true;
  return true;
}

bool TestImageBacking::ProduceLegacyMailbox(MailboxManager* mailbox_manager) {
  return false;
}

std::unique_ptr<GLTextureImageRepresentation>
TestImageBacking::ProduceGLTexture(SharedImageManager* manager,
                                   MemoryTypeTracker* tracker) {
  return std::make_unique<TestGLTextureImageRepresentation>(manager, this,
                                                            tracker, texture_);
}

std::unique_ptr<GLTexturePassthroughImageRepresentation>
TestImageBacking::ProduceGLTexturePassthrough(SharedImageManager* manager,
                                              MemoryTypeTracker* tracker) {
  return std::make_unique<TestGLTexturePassthroughImageRepresentation>(
      manager, this, tracker, texture_passthrough_);
}

std::unique_ptr<SkiaImageRepresentation> TestImageBacking::ProduceSkia(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    scoped_refptr<SharedContextState> context_state) {
  return std::make_unique<TestSkiaImageRepresentation>(manager, this, tracker);
}

std::unique_ptr<DawnImageRepresentation> TestImageBacking::ProduceDawn(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    WGPUDevice device,
    WGPUBackendType backend_type) {
  return std::make_unique<TestDawnImageRepresentation>(manager, this, tracker);
}

std::unique_ptr<OverlayImageRepresentation> TestImageBacking::ProduceOverlay(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker) {
  return std::make_unique<TestOverlayImageRepresentation>(manager, this,
                                                          tracker);
}

}  // namespace gpu

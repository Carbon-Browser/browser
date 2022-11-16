// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_IMAGE_BACKING_H_
#define GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_IMAGE_BACKING_H_

#include "gpu/command_buffer/service/shared_image/gl_texture_image_backing_helper.h"

namespace gl {
class GLImageEGL;
}

namespace gpu {

// Implementation of SharedImageBacking that creates a GL Texture that is not
// backed by a GLImage.
class GLTextureImageBacking : public ClearTrackingSharedImageBacking {
 public:
  GLTextureImageBacking(const Mailbox& mailbox,
                        viz::ResourceFormat format,
                        const gfx::Size& size,
                        const gfx::ColorSpace& color_space,
                        GrSurfaceOrigin surface_origin,
                        SkAlphaType alpha_type,
                        uint32_t usage,
                        bool is_passthrough);
  GLTextureImageBacking(const GLTextureImageBacking&) = delete;
  GLTextureImageBacking& operator=(const GLTextureImageBacking&) = delete;
  ~GLTextureImageBacking() override;

  void InitializeGLTexture(
      GLuint service_id,
      const GLTextureImageBackingHelper::InitializeGLTextureParams& params);
  void SetCompatibilitySwizzle(
      const gles2::Texture::CompatibilitySwizzle* swizzle);

  GLenum GetGLTarget() const;
  GLuint GetGLServiceId() const;
  void CreateEGLImage();

 private:
  // SharedImageBacking:
  void OnMemoryDump(const std::string& dump_name,
                    base::trace_event::MemoryAllocatorDump* dump,
                    base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t client_tracing_id) override;
  SharedImageBackingType GetType() const override;
  gfx::Rect ClearedRect() const final;
  void SetClearedRect(const gfx::Rect& cleared_rect) final;
  bool ProduceLegacyMailbox(MailboxManager* mailbox_manager) final;
  std::unique_ptr<GLTextureImageRepresentation> ProduceGLTexture(
      SharedImageManager* manager,
      MemoryTypeTracker* tracker) final;
  std::unique_ptr<GLTexturePassthroughImageRepresentation>
  ProduceGLTexturePassthrough(SharedImageManager* manager,
                              MemoryTypeTracker* tracker) final;
  std::unique_ptr<DawnImageRepresentation> ProduceDawn(
      SharedImageManager* manager,
      MemoryTypeTracker* tracker,
      WGPUDevice device,
      WGPUBackendType backend_type) final;
  std::unique_ptr<SkiaImageRepresentation> ProduceSkia(
      SharedImageManager* manager,
      MemoryTypeTracker* tracker,
      scoped_refptr<SharedContextState> context_state) override;
  void Update(std::unique_ptr<gfx::GpuFence> in_fence) override;

  bool IsPassthrough() const { return is_passthrough_; }

  const bool is_passthrough_;
  gles2::Texture* texture_ = nullptr;
  scoped_refptr<gles2::TexturePassthrough> passthrough_texture_;

  sk_sp<SkPromiseImageTexture> cached_promise_texture_;
  scoped_refptr<gl::GLImageEGL> image_egl_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_IMAGE_BACKING_H_

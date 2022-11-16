// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_PASSTHROUGH_ANDROID_IMAGE_REPRESENTATION_H_
#define GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_PASSTHROUGH_ANDROID_IMAGE_REPRESENTATION_H_

#include "gpu/command_buffer/service/shared_image/android_image_backing.h"
#include "gpu/command_buffer/service/shared_image/shared_image_representation.h"

namespace gpu {
class AndroidImageBacking;

class GLTexturePassthroughAndroidImageRepresentation
    : public GLTexturePassthroughImageRepresentation {
 public:
  GLTexturePassthroughAndroidImageRepresentation(
      SharedImageManager* manager,
      AndroidImageBacking* backing,
      MemoryTypeTracker* tracker,
      scoped_refptr<gles2::TexturePassthrough> texture);
  ~GLTexturePassthroughAndroidImageRepresentation() override;

  GLTexturePassthroughAndroidImageRepresentation(
      const GLTexturePassthroughAndroidImageRepresentation&) = delete;
  GLTexturePassthroughAndroidImageRepresentation& operator=(
      const GLTexturePassthroughAndroidImageRepresentation&) = delete;

  const scoped_refptr<gles2::TexturePassthrough>& GetTexturePassthrough()
      override;

  bool BeginAccess(GLenum mode) override;
  void EndAccess() override;

 private:
  AndroidImageBacking* android_backing() {
    return static_cast<AndroidImageBacking*>(backing());
  }

  scoped_refptr<gles2::TexturePassthrough> texture_;
  RepresentationAccessMode mode_ = RepresentationAccessMode::kNone;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_GL_TEXTURE_PASSTHROUGH_ANDROID_IMAGE_REPRESENTATION_H_

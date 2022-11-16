// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_DISPLAY_H_
#define UI_GL_GL_DISPLAY_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "ui/gl/gl_export.h"

#if defined(USE_EGL)
#include <EGL/egl.h>

#include "ui/gl/gpu_switching_manager.h"
#endif  // defined(USE_EGL)

namespace base {
class CommandLine;
}  // namespace base

namespace gl {
struct DisplayExtensionsEGL;
template <typename GLDisplayPlatform>
class GLDisplayManager;

class EGLDisplayPlatform {
 public:
  constexpr EGLDisplayPlatform()
      : display_(EGL_DEFAULT_DISPLAY), platform_(0), valid_(false) {}
  explicit constexpr EGLDisplayPlatform(EGLNativeDisplayType display,
                                        int platform = 0)
      : display_(display), platform_(platform), valid_(true) {}

  bool Valid() const { return valid_; }
  int GetPlatform() const { return platform_; }
  EGLNativeDisplayType GetDisplay() const { return display_; }

 private:
  EGLNativeDisplayType display_;
  // 0 for default, or EGL_PLATFORM_* enum.
  int platform_;
  bool valid_;
};

// If adding a new type, also add it to EGLDisplayType in
// tools/metrics/histograms/enums.xml. Don't remove or reorder entries.
enum DisplayType {
  DEFAULT = 0,
  SWIFT_SHADER = 1,
  ANGLE_WARP = 2,
  ANGLE_D3D9 = 3,
  ANGLE_D3D11 = 4,
  ANGLE_OPENGL = 5,
  ANGLE_OPENGLES = 6,
  ANGLE_NULL = 7,
  ANGLE_D3D11_NULL = 8,
  ANGLE_OPENGL_NULL = 9,
  ANGLE_OPENGLES_NULL = 10,
  ANGLE_VULKAN = 11,
  ANGLE_VULKAN_NULL = 12,
  ANGLE_D3D11on12 = 13,
  ANGLE_SWIFTSHADER = 14,
  ANGLE_OPENGL_EGL = 15,
  ANGLE_OPENGLES_EGL = 16,
  ANGLE_METAL = 17,
  ANGLE_METAL_NULL = 18,
  DISPLAY_TYPE_MAX = 19,
};

GL_EXPORT void GetEGLInitDisplaysForTesting(
    bool supports_angle_d3d,
    bool supports_angle_opengl,
    bool supports_angle_null,
    bool supports_angle_vulkan,
    bool supports_angle_swiftshader,
    bool supports_angle_egl,
    bool supports_angle_metal,
    const base::CommandLine* command_line,
    std::vector<DisplayType>* init_displays);

class GL_EXPORT GLDisplay {
 public:
  GLDisplay(const GLDisplay&) = delete;
  GLDisplay& operator=(const GLDisplay&) = delete;

  uint64_t system_device_id() const { return system_device_id_; }

  virtual ~GLDisplay();

  virtual void* GetDisplay() = 0;
  virtual void Shutdown() = 0;

 protected:
  explicit GLDisplay(uint64_t system_device_id);

  uint64_t system_device_id_ = 0;
};

#if defined(USE_EGL)
class GL_EXPORT GLDisplayEGL : public GLDisplay {
 public:
  GLDisplayEGL(const GLDisplayEGL&) = delete;
  GLDisplayEGL& operator=(const GLDisplayEGL&) = delete;

  ~GLDisplayEGL() override;

  static GLDisplayEGL* GetDisplayForCurrentContext();

  EGLDisplay GetDisplay() override;
  void Shutdown() override;

  void SetDisplay(EGLDisplay display);
  EGLDisplayPlatform GetNativeDisplay() const;
  DisplayType GetDisplayType() const;

  bool IsEGLSurfacelessContextSupported();
  bool IsEGLContextPrioritySupported();
  bool IsAndroidNativeFenceSyncSupported();
  bool IsANGLEExternalContextAndSurfaceSupported();

  bool Initialize(EGLDisplayPlatform native_display);
  void InitializeForTesting();
  bool InitializeExtensionSettings();

  std::unique_ptr<DisplayExtensionsEGL> ext;

 private:
  friend class GLDisplayManager<GLDisplayEGL>;
  friend class EGLApiTest;

  class EGLGpuSwitchingObserver final : public ui::GpuSwitchingObserver {
   public:
    explicit EGLGpuSwitchingObserver(EGLDisplay display);
    ~EGLGpuSwitchingObserver() override = default;
    void OnGpuSwitched(GpuPreference active_gpu_heuristic) override;

   private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
  };

  explicit GLDisplayEGL(uint64_t system_device_id);

  bool InitializeDisplay(EGLDisplayPlatform native_display);
  void InitializeCommon();

  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLDisplayPlatform native_display_ = EGLDisplayPlatform(EGL_DEFAULT_DISPLAY);
  DisplayType display_type_ = DisplayType::DEFAULT;

  bool egl_surfaceless_context_supported_ = false;
  bool egl_context_priority_supported_ = false;
  bool egl_android_native_fence_sync_supported_ = false;

  std::unique_ptr<EGLGpuSwitchingObserver> gpu_switching_observer_;
};
#endif  // defined(USE_EGL)

#if defined(USE_GLX)
class GL_EXPORT GLDisplayX11 : public GLDisplay {
 public:
  GLDisplayX11(const GLDisplayX11&) = delete;
  GLDisplayX11& operator=(const GLDisplayX11&) = delete;

  ~GLDisplayX11() override;

  void* GetDisplay() override;
  void Shutdown() override;

 private:
  friend class GLDisplayManager<GLDisplayX11>;

  explicit GLDisplayX11(uint64_t system_device_id);
};
#endif  // defined(USE_GLX)

}  // namespace gl

#endif  // UI_GL_GL_DISPLAY_H_

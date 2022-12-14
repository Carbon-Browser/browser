// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_CROSAPI_WALLPAPER_ASH_H_
#define CHROME_BROWSER_ASH_CROSAPI_WALLPAPER_ASH_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_decoder/image_decoder.h"
#include "chromeos/crosapi/mojom/wallpaper.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "ui/gfx/image/image_skia.h"

namespace wallpaper_api_util {

// A class to decode JPEG file.
class WallpaperDecoder : public ImageDecoder::ImageRequest {
 public:
  using DecodedCallback = base::OnceCallback<void(const gfx::ImageSkia&)>;
  using CanceledCallback = base::OnceCallback<void()>;
  using FailedCallback = base::OnceCallback<void(const std::string&)>;

  explicit WallpaperDecoder(DecodedCallback decoded_cb,
                            CanceledCallback canceled_cb,
                            FailedCallback failed_cb);
  ~WallpaperDecoder() override;

  WallpaperDecoder(const WallpaperDecoder&) = delete;
  WallpaperDecoder& operator=(const WallpaperDecoder&) = delete;

  void Start(const std::vector<uint8_t>& image_data);
  void Cancel();

  void OnImageDecoded(const SkBitmap& decoded_image) override;
  void OnDecodeImageFailed() override;

 private:
  DecodedCallback decoded_cb_;
  CanceledCallback canceled_cb_;
  FailedCallback failed_cb_;

  base::AtomicFlag cancel_flag_;
};

}  // namespace wallpaper_api_util

namespace crosapi {

class WallpaperAsh : public mojom::Wallpaper {
 public:
  WallpaperAsh();
  WallpaperAsh(const WallpaperAsh&) = delete;
  WallpaperAsh& operator=(const WallpaperAsh&) = delete;
  ~WallpaperAsh() override;

  void BindReceiver(mojo::PendingReceiver<mojom::Wallpaper> receiver);

  // mojom::Wallpaper:
  void SetWallpaper(mojom::WallpaperSettingsPtr wallpaper,
                    const std::string& extension_id,
                    const std::string& extension_name,
                    SetWallpaperCallback callback) override;

 private:
  // Starts to decode |data|. Must run on UI thread.
  void StartDecode(const std::vector<uint8_t>& data);

  // Handles cancel case. No error message should be set.
  void OnDecodingCanceled();

  // Handles failure case. Sets error message.
  void OnDecodingFailed(const std::string& error);

  void OnWallpaperDecoded(const gfx::ImageSkia& wallpaper);

  void CancelAndReset();

  mojo::ReceiverSet<mojom::Wallpaper> receivers_;

  // Holds an instance of WallpaperDecoder.
  wallpaper_api_util::WallpaperDecoder* wallpaper_decoder_ = nullptr;

  SetWallpaperCallback pending_callback_;
  mojom::WallpaperSettingsPtr wallpaper_settings_;
  std::string extension_id_;
  std::string extension_name_;

  base::WeakPtrFactory<WallpaperAsh> weak_ptr_factory_{this};
};

}  // namespace crosapi

#endif

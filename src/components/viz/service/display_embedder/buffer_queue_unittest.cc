// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/buffer_queue.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <utility>

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "components/viz/test/test_context_provider.h"
#include "gpu/command_buffer/client/shared_image_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Expectation;
using ::testing::Ne;
using ::testing::Return;

namespace viz {

namespace {

constexpr gfx::BufferFormat kBufferQueueFormat = gfx::BufferFormat::RGBA_8888;
constexpr gfx::ColorSpace kBufferQueueColorSpace =
    gfx::ColorSpace::CreateSRGB();

}  // namespace

#if BUILDFLAG(IS_WIN)
const gpu::SurfaceHandle kFakeSurfaceHandle =
    reinterpret_cast<gpu::SurfaceHandle>(1);
#else
const gpu::SurfaceHandle kFakeSurfaceHandle = 1;
#endif

class BufferQueueTest : public ::testing::Test {
 public:
  BufferQueueTest() = default;

  void SetUp() override {
    context_provider_ = TestContextProvider::Create(
        std::make_unique<TestSharedImageInterface>());
    context_provider_->BindToCurrentThread();
    buffer_queue_ = std::make_unique<BufferQueue>(
        context_provider_->SharedImageInterface(), kFakeSurfaceHandle, 3);
  }

  void TearDown() override { buffer_queue_.reset(); }

  gpu::Mailbox current_buffer() {
    return buffer_queue_->current_buffer_
               ? buffer_queue_->current_buffer_->mailbox
               : gpu::Mailbox();
  }
  const base::circular_deque<std::unique_ptr<BufferQueue::AllocatedBuffer>>&
  available_buffers() {
    return buffer_queue_->available_buffers_;
  }
  base::circular_deque<std::unique_ptr<BufferQueue::AllocatedBuffer>>&
  in_flight_buffers() {
    return buffer_queue_->in_flight_buffers_;
  }

  const BufferQueue::AllocatedBuffer* displayed_frame() {
    return buffer_queue_->displayed_buffer_.get();
  }
  const BufferQueue::AllocatedBuffer* current_frame() {
    return buffer_queue_->current_buffer_.get();
  }
  const gfx::Size size() { return buffer_queue_->size_; }

  int CountBuffers() {
    int n = available_buffers().size() + in_flight_buffers().size() +
            (displayed_frame() ? 1 : 0);
    if (!current_buffer().IsZero())
      n++;
    return n;
  }

  // Check that each buffer is unique if present.
  bool CheckUnique() {
    std::set<gpu::Mailbox> buffers;
    if (!InsertUnique(&buffers, current_buffer())) {
      return false;
    }
    if (displayed_frame() &&
        !InsertUnique(&buffers, displayed_frame()->mailbox)) {
      return false;
    }
    for (auto& buffer : available_buffers()) {
      if (!InsertUnique(&buffers, buffer->mailbox)) {
        return false;
      }
    }
    for (auto& buffer : in_flight_buffers()) {
      if (buffer && !InsertUnique(&buffers, buffer->mailbox)) {
        return false;
      }
    }
    return true;
  }

  void SendDamagedFrame(const gfx::Rect& damage) {
    // We don't care about the GL-level implementation here, just how it uses
    // damage rects.
    buffer_queue_->GetCurrentBuffer();
    buffer_queue_->SwapBuffers(damage);
    buffer_queue_->SwapBuffersComplete();
  }

  void SendFullFrame() { SendDamagedFrame(gfx::Rect(buffer_queue_->size_)); }

 protected:
  bool InsertUnique(std::set<gpu::Mailbox>* set, gpu::Mailbox value) {
    if (value.IsZero())
      return true;
    if (set->find(value) != set->end())
      return false;
    set->insert(value);
    return true;
  }

  scoped_refptr<TestContextProvider> context_provider_;
  std::unique_ptr<BufferQueue> buffer_queue_;
};

const gfx::Size screen_size = gfx::Size(30, 30);
const gfx::Rect screen_rect = gfx::Rect(screen_size);
const gfx::Rect small_damage = gfx::Rect(gfx::Size(10, 10));
const gfx::Rect large_damage = gfx::Rect(gfx::Size(20, 20));
const gfx::Rect overlapping_damage = gfx::Rect(gfx::Size(5, 20));

class MockedSharedImageInterface : public TestSharedImageInterface {
 public:
  MockedSharedImageInterface() {
    ON_CALL(*this, CreateSharedImage(_, _, _, _, _, _, _))
        .WillByDefault(Return(gpu::Mailbox()));
    // this, &MockedSharedImageInterface::TestCreateSharedImage));
  }
  MOCK_METHOD7(CreateSharedImage,
               gpu::Mailbox(ResourceFormat format,
                            const gfx::Size& size,
                            const gfx::ColorSpace& color_space,
                            GrSurfaceOrigin surface_origin,
                            SkAlphaType alpha_type,
                            uint32_t usage,
                            gpu::SurfaceHandle surface_handle));
  MOCK_METHOD2(UpdateSharedImage,
               void(const gpu::SyncToken& sync_token,
                    const gpu::Mailbox& mailbox));
  MOCK_METHOD2(DestroySharedImage,
               void(const gpu::SyncToken& sync_token,
                    const gpu::Mailbox& mailbox));
  // Use this to call CreateSharedImage() defined in TestSharedImageInterface.
  gpu::Mailbox TestCreateSharedImage(ResourceFormat format,
                                     const gfx::Size& size,
                                     const gfx::ColorSpace& color_space,
                                     GrSurfaceOrigin surface_origin,
                                     SkAlphaType alpha_type,
                                     uint32_t usage,
                                     gpu::SurfaceHandle surface_handle) {
    return TestSharedImageInterface::CreateSharedImage(
        format, size, color_space, surface_origin, alpha_type, usage,
        surface_handle);
  }
};

scoped_refptr<TestContextProvider> CreateMockedSharedImageInterfaceProvider(
    MockedSharedImageInterface** sii) {
  std::unique_ptr<MockedSharedImageInterface> owned_sii(
      new MockedSharedImageInterface);
  *sii = owned_sii.get();
  scoped_refptr<TestContextProvider> context_provider =
      TestContextProvider::Create(std::move(owned_sii));
  context_provider->BindToCurrentThread();
  return context_provider;
}

TEST(BufferQueueStandaloneTest, BufferCreationAndDestruction) {
  MockedSharedImageInterface* sii;
  scoped_refptr<TestContextProvider> context_provider =
      CreateMockedSharedImageInterfaceProvider(&sii);
  std::unique_ptr<BufferQueue> buffer_queue = std::make_unique<BufferQueue>(
      context_provider->SharedImageInterface(), kFakeSurfaceHandle, 1);

  const gpu::Mailbox expected_mailbox = gpu::Mailbox::GenerateForSharedImage();
  {
    testing::InSequence dummy;
    EXPECT_CALL(*sii, CreateSharedImage(_, _, _, _, _,
                                        gpu::SHARED_IMAGE_USAGE_SCANOUT |
                                            gpu::SHARED_IMAGE_USAGE_DISPLAY,
                                        _))
        .WillOnce(Return(expected_mailbox));
    EXPECT_CALL(*sii, DestroySharedImage(_, expected_mailbox));
  }

  EXPECT_TRUE(buffer_queue->Reshape(screen_size, kBufferQueueColorSpace,
                                    kBufferQueueFormat));
  EXPECT_EQ(expected_mailbox, buffer_queue->GetCurrentBuffer());
}

TEST_F(BufferQueueTest, PartialSwapReuse) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);
  SendDamagedFrame(large_damage);
  // Verify that the damage has propagated.
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), large_damage);
}

TEST_F(BufferQueueTest, PartialSwapFullFrame) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendFullFrame();
  SendFullFrame();
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), screen_rect);
}

// Make sure that each time we swap buffers, the damage gets propagated to the
// previously swapped buffers.
TEST_F(BufferQueueTest, PartialSwapWithTripleBuffering) {
  EXPECT_EQ(0, CountBuffers());
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  EXPECT_EQ(3, CountBuffers());

  SendFullFrame();
  SendFullFrame();
  // Let's triple buffer.
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  buffer_queue_->SwapBuffers(small_damage);
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  // The whole buffer needs to be redrawn since it's a newly allocated buffer
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), screen_rect);

  SendDamagedFrame(overlapping_damage);
  // The next buffer should include damage from |overlapping_damage| and
  // |small_damage|.
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  const auto current_buffer_damage = buffer_queue_->CurrentBufferDamage();
  EXPECT_TRUE(current_buffer_damage.Contains(overlapping_damage));
  EXPECT_TRUE(current_buffer_damage.Contains(small_damage));

  // Let's make sure the damage is not trivially the whole screen.
  EXPECT_NE(current_buffer_damage, screen_rect);
}

TEST_F(BufferQueueTest, PartialSwapOverlapping) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));

  SendFullFrame();
  SendDamagedFrame(small_damage);
  SendDamagedFrame(overlapping_damage);
  // Expect small_damage UNION overlapping_damage
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), gfx::Rect(0, 0, 10, 20));
}

TEST_F(BufferQueueTest, MultipleGetCurrentBufferCalls) {
  // It is not valid to call GetCurrentBuffer without having set an initial
  // size via Reshape.
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  // Check that multiple bind calls do not create or change buffers.
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  gpu::Mailbox fb = buffer_queue_->GetCurrentBuffer();
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_EQ(fb, buffer_queue_->GetCurrentBuffer());
}

TEST_F(BufferQueueTest, CheckDoubleBuffering) {
  // Check buffer flow through double buffering path.

  // Create a BufferQueue with only 2 buffers.
  buffer_queue_ = std::make_unique<BufferQueue>(
      context_provider_->SharedImageInterface(), kFakeSurfaceHandle, 2);

  EXPECT_EQ(0, CountBuffers());
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  EXPECT_EQ(2, CountBuffers());

  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_FALSE(displayed_frame());

  buffer_queue_->SwapBuffers(screen_rect);

  EXPECT_EQ(1U, in_flight_buffers().size());
  buffer_queue_->SwapBuffersComplete();

  EXPECT_EQ(0U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_TRUE(CheckUnique());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_EQ(0U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  buffer_queue_->SwapBuffers(screen_rect);
  EXPECT_TRUE(CheckUnique());
  EXPECT_EQ(1U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  buffer_queue_->SwapBuffersComplete();
  EXPECT_TRUE(CheckUnique());
  EXPECT_EQ(0U, in_flight_buffers().size());
  EXPECT_EQ(1U, available_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_TRUE(CheckUnique());
  EXPECT_TRUE(available_buffers().empty());
}

TEST_F(BufferQueueTest, CheckTripleBuffering) {
  // Check buffer flow through triple buffering path.
  EXPECT_EQ(0, CountBuffers());
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  EXPECT_EQ(3, CountBuffers());

  // This bit is the same sequence tested in the doublebuffering case.
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_FALSE(displayed_frame());
  buffer_queue_->SwapBuffers(screen_rect);
  buffer_queue_->SwapBuffersComplete();
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  buffer_queue_->SwapBuffers(screen_rect);

  EXPECT_TRUE(CheckUnique());
  EXPECT_EQ(1U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_TRUE(CheckUnique());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_EQ(1U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  buffer_queue_->SwapBuffersComplete();
  EXPECT_TRUE(CheckUnique());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_EQ(0U, in_flight_buffers().size());
  EXPECT_FALSE(displayed_frame()->mailbox.IsZero());
  EXPECT_EQ(1U, available_buffers().size());
}

TEST_F(BufferQueueTest, CheckEmptySwap) {
  // It is not valid to call GetCurrentBuffer without having set an initial
  // size via Reshape.
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  // Check empty swap flow, in which the damage is empty.
  gpu::Mailbox mailbox = buffer_queue_->GetCurrentBuffer();
  EXPECT_FALSE(mailbox.IsZero());
  EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
  EXPECT_FALSE(displayed_frame());

  buffer_queue_->SwapBuffers(gfx::Rect());
  // Make sure we won't be drawing to the buffer we just sent for scanout.
  gpu::Mailbox new_mailbox = buffer_queue_->GetCurrentBuffer();
  EXPECT_FALSE(new_mailbox.IsZero());
  EXPECT_NE(mailbox, new_mailbox);

  EXPECT_EQ(1U, in_flight_buffers().size());
  buffer_queue_->SwapBuffersComplete();

  buffer_queue_->SwapBuffers(gfx::Rect());
  // Test SwapBuffers() without calling GetCurrentBuffer().
  buffer_queue_->SwapBuffers(gfx::Rect());
  EXPECT_EQ(2U, in_flight_buffers().size());

  buffer_queue_->SwapBuffersComplete();
  EXPECT_EQ(1U, in_flight_buffers().size());

  buffer_queue_->SwapBuffersComplete();
  EXPECT_EQ(0U, in_flight_buffers().size());
}

TEST_F(BufferQueueTest, CheckCorrectBufferOrdering) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
    buffer_queue_->SwapBuffers(screen_rect);
  }

  EXPECT_EQ(kSwapCount, in_flight_buffers().size());
  for (size_t i = 0; i < kSwapCount; ++i) {
    gpu::Mailbox next_mailbox = in_flight_buffers().front()->mailbox;
    buffer_queue_->SwapBuffersComplete();
    EXPECT_EQ(displayed_frame()->mailbox, next_mailbox);
  }
}

TEST_F(BufferQueueTest, ReshapeWithInFlightBuffers) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
    buffer_queue_->SwapBuffers(screen_rect);
  }

  EXPECT_TRUE(buffer_queue_->Reshape(gfx::Size(10, 20), kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  EXPECT_EQ(3u, in_flight_buffers().size());
  EXPECT_EQ(3u, available_buffers().size());
  // The inflight images are destroyed, but the buffers are still around for
  // now, in addition to the newly created buffers.
  EXPECT_EQ(6, CountBuffers());

  for (size_t i = 0; i < kSwapCount; ++i) {
    buffer_queue_->SwapBuffersComplete();
    EXPECT_FALSE(displayed_frame());
  }

  // The dummy buffers left should be discarded.
  EXPECT_EQ(3u, available_buffers().size());
}

TEST_F(BufferQueueTest, SwapAfterReshape) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  const size_t kSwapCount = 3;
  for (size_t i = 0; i < kSwapCount; ++i) {
    EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
    buffer_queue_->SwapBuffers(screen_rect);
  }

  EXPECT_TRUE(buffer_queue_->Reshape(gfx::Size(10, 20), kBufferQueueColorSpace,
                                     kBufferQueueFormat));

  for (size_t i = 0; i < kSwapCount; ++i) {
    EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
    buffer_queue_->SwapBuffers(screen_rect);
  }
  EXPECT_EQ(2 * kSwapCount, in_flight_buffers().size());

  for (size_t i = 0; i < kSwapCount; ++i) {
    buffer_queue_->SwapBuffersComplete();
    EXPECT_FALSE(displayed_frame());
  }

  EXPECT_TRUE(CheckUnique());

  for (size_t i = 0; i < kSwapCount; ++i) {
    gpu::Mailbox next_mailbox = in_flight_buffers().front()->mailbox;
    buffer_queue_->SwapBuffersComplete();
    EXPECT_EQ(displayed_frame()->mailbox, next_mailbox);
    EXPECT_TRUE(displayed_frame());
  }

  for (size_t i = 0; i < kSwapCount; ++i) {
    EXPECT_FALSE(buffer_queue_->GetCurrentBuffer().IsZero());
    buffer_queue_->SwapBuffers(screen_rect);
    buffer_queue_->SwapBuffersComplete();
  }
}

TEST_F(BufferQueueTest, SwapBuffersSkipped) {
  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);

  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);

  auto mailbox1 = buffer_queue_->GetCurrentBuffer();
  buffer_queue_->SwapBuffersSkipped(large_damage);
  auto mailbox2 = buffer_queue_->GetCurrentBuffer();

  // SwapBuffersSkipped() didn't advance the current buffer.
  EXPECT_EQ(mailbox1, mailbox2);
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);

  // Swap on the next frame with no additional damage.
  buffer_queue_->SwapBuffers(gfx::Rect());
  buffer_queue_->SwapBuffersComplete();

  // The next frame has the damage from the last SwapBuffersSkipped().
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), large_damage);
}

TEST_F(BufferQueueTest, EnsureMinNumberOfBuffers) {
  EXPECT_EQ(CountBuffers(), 0);

  buffer_queue_->EnsureMinNumberOfBuffers(4);

  // EnsureMinNumberOfBuffers does nothing before Reshape() is called.
  EXPECT_EQ(CountBuffers(), 0);

  EXPECT_TRUE(buffer_queue_->Reshape(screen_size, kBufferQueueColorSpace,
                                     kBufferQueueFormat));

  EXPECT_EQ(CountBuffers(), 4);

  buffer_queue_->EnsureMinNumberOfBuffers(2);

  // EnsureMinNumberOfBuffers will never decrease the number of buffers.
  EXPECT_EQ(CountBuffers(), 4);

  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);
  SendDamagedFrame(small_damage);

  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);

  buffer_queue_->EnsureMinNumberOfBuffers(5);

  EXPECT_EQ(CountBuffers(), 5);

  // 3 of the existing buffers will be in available_buffers_ already, with
  // existing small_damage.
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);
  SendDamagedFrame(small_damage);
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);
  SendDamagedFrame(small_damage);
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);
  SendDamagedFrame(small_damage);

  // The new buffer will come next with full damage.
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), screen_rect);
  SendDamagedFrame(small_damage);

  // Finally the buffer that was being displayed while we added the 5th buffer
  // will also have small damage.
  EXPECT_EQ(buffer_queue_->CurrentBufferDamage(), small_damage);
  SendDamagedFrame(small_damage);
}

}  // namespace viz

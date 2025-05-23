// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "remoting/host/touch_injector_win.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <utility>

#include "base/containers/contains.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "remoting/proto/event.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ExpectationSet;
using ::testing::InSequence;
using ::testing::Return;

namespace remoting {

using protocol::TouchEvent;
using protocol::TouchEventPoint;

namespace {

// Maps touch pointer ID to expected flags [start, move, end, cancel] listed
// below.
typedef std::map<uint32_t, uint32_t> IdFlagMap;

const uint32_t kStartFlag =
    POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;

const uint32_t kMoveFlag =
    POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE;

const uint32_t kEndFlag = POINTER_FLAG_UP;

const uint32_t kCancelFlag = POINTER_FLAG_UP | POINTER_FLAG_CANCELED;

MATCHER_P(EqualsSinglePointerTouchInfo, expected, "") {
  return arg->touchMask == expected.touchMask &&
         arg->rcContact.left == expected.rcContact.left &&
         arg->rcContact.top == expected.rcContact.top &&
         arg->rcContact.right == expected.rcContact.right &&
         arg->rcContact.bottom == expected.rcContact.bottom &&
         arg->orientation == expected.orientation &&
         arg->pressure == expected.pressure &&
         arg->pointerInfo.pointerType == expected.pointerInfo.pointerType &&
         arg->pointerInfo.pointerId == expected.pointerInfo.pointerId &&
         arg->pointerInfo.ptPixelLocation.x ==
             expected.pointerInfo.ptPixelLocation.x &&
         arg->pointerInfo.ptPixelLocation.y ==
             expected.pointerInfo.ptPixelLocation.y;
}

// Make sure that every touch point has the right flag (pointerFlags).
MATCHER_P(EqualsPointerTouchInfoFlag, id_to_flag_map, "") {
  for (size_t i = 0; i < id_to_flag_map.size(); ++i) {
    const POINTER_TOUCH_INFO* touch_info = arg + i;
    const uint32_t id = touch_info->pointerInfo.pointerId;
    if (!base::Contains(id_to_flag_map, id)) {
      return false;
    }

    if (id_to_flag_map.find(id)->second !=
        touch_info->pointerInfo.pointerFlags) {
      return false;
    }
  }
  return true;
}

class TouchInjectorWinDelegateMock : public TouchInjectorWinDelegate {
 public:
  TouchInjectorWinDelegateMock()
      : TouchInjectorWinDelegate(nullptr, nullptr, nullptr) {}
  ~TouchInjectorWinDelegateMock() override {}

  MOCK_METHOD2(InitializeTouchInjection, BOOL(UINT32 max_count, DWORD dw_mode));
  MOCK_METHOD2(InjectTouchInput,
               DWORD(UINT32 count, const POINTER_TOUCH_INFO* contacts));
};

class TouchInjectorWinTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TouchInjectorWin injector_;
};

// A test to make sure that the touch event is converted correctly to
// POINTER_TOUCH_INFO.
TEST_F(TouchInjectorWinTest, CheckConversionWithPressure) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(1234u);
  point->set_x(321.0f);
  point->set_y(123.0f);
  point->set_radius_x(10.0f);
  point->set_radius_y(20.0f);
  point->set_pressure(0.5f);
  point->set_angle(45.0f);

  POINTER_TOUCH_INFO expected_touch_info;
  expected_touch_info.touchMask =
      TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
  expected_touch_info.rcContact.left = 311;
  expected_touch_info.rcContact.top = 103;
  expected_touch_info.rcContact.right = 331;
  expected_touch_info.rcContact.bottom = 143;
  expected_touch_info.orientation = 0;
  expected_touch_info.pressure = 512;
  expected_touch_info.orientation = 45;

  expected_touch_info.pointerInfo.pointerType = PT_TOUCH;
  expected_touch_info.pointerInfo.pointerId = 1234u;
  expected_touch_info.pointerInfo.ptPixelLocation.x = 321;
  expected_touch_info.pointerInfo.ptPixelLocation.y = 123;

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));
  EXPECT_CALL(
      *delegate_mock,
      InjectTouchInput(1, EqualsSinglePointerTouchInfo(expected_touch_info)))
      .WillOnce(Return(1));

  // Check pressure clamping as well.
  expected_touch_info.pressure = 1024;  // Max
  EXPECT_CALL(
      *delegate_mock,
      InjectTouchInput(1, EqualsSinglePointerTouchInfo(expected_touch_info)))
      .WillOnce(Return(1));

  expected_touch_info.pressure = 0;  // Min
  EXPECT_CALL(
      *delegate_mock,
      InjectTouchInput(1, EqualsSinglePointerTouchInfo(expected_touch_info)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);

  // Change to MOVE so that there still only one point.
  event.set_event_type(TouchEvent::TOUCH_POINT_MOVE);
  point->set_pressure(2.0f);
  injector_.InjectTouchEvent(event);

  point->set_pressure(-3.0f);
  injector_.InjectTouchEvent(event);
}

// Some devices don't detect pressure. This test is a conversion check for
// such devices.
TEST_F(TouchInjectorWinTest, CheckConversionNoPressure) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(1234u);
  point->set_x(321.0f);
  point->set_y(123.0f);
  point->set_radius_x(10.0f);
  point->set_radius_y(20.0f);
  point->set_angle(45.0f);

  POINTER_TOUCH_INFO expected_touch_info;
  expected_touch_info.touchMask =
      TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION;
  expected_touch_info.rcContact.left = 311;
  expected_touch_info.rcContact.top = 103;
  expected_touch_info.rcContact.right = 331;
  expected_touch_info.rcContact.bottom = 143;
  expected_touch_info.orientation = 0;
  expected_touch_info.pressure = 0;
  expected_touch_info.orientation = 45;

  expected_touch_info.pointerInfo.pointerType = PT_TOUCH;
  expected_touch_info.pointerInfo.pointerId = 1234u;
  expected_touch_info.pointerInfo.ptPixelLocation.x = 321;
  expected_touch_info.pointerInfo.ptPixelLocation.y = 123;

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));
  EXPECT_CALL(
      *delegate_mock,
      InjectTouchInput(1, EqualsSinglePointerTouchInfo(expected_touch_info)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);
}

// If initialization fails, it should not call any touch injection functions.
TEST_F(TouchInjectorWinTest, InitFailed) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(0));
  EXPECT_CALL(*delegate_mock, InjectTouchInput(_, _)).Times(0);

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_FALSE(injector_.Init());
  injector_.InjectTouchEvent(event);
}

// Deinitialize and initialize should clean the state.
TEST_F(TouchInjectorWinTest, Reinitialize) {
  std::unique_ptr<TouchInjectorWinDelegateMock>
      delegate_mock_before_deinitialize(
          new ::testing::StrictMock<TouchInjectorWinDelegateMock>());
  std::unique_ptr<TouchInjectorWinDelegateMock>
      delegate_mock_after_deinitialize(
          new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent first_event;
  first_event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point0 = first_event.add_touch_points();
  point0->set_id(0u);

  TouchEvent second_event;
  second_event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point1 = second_event.add_touch_points();
  point1->set_id(1u);

  InSequence s;
  EXPECT_CALL(*delegate_mock_before_deinitialize,
              InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock_before_deinitialize,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  EXPECT_CALL(*delegate_mock_after_deinitialize, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  // After deinitializing and then initializing, previous touch points should be
  // gone.
  id_to_flags.clear();
  id_to_flags[1u] = kStartFlag;
  EXPECT_CALL(*delegate_mock_after_deinitialize,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(
      std::move(delegate_mock_before_deinitialize));

  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(first_event);
  injector_.Deinitialize();

  injector_.SetInjectorDelegateForTest(
      std::move(delegate_mock_after_deinitialize));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(second_event);
}

// Make sure that the flag is set to kStartFlag.
TEST_F(TouchInjectorWinTest, StartTouchPoint) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(0u);

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);
}

// Start a point and then move, make sure the flag is set to kMoveFlag.
TEST_F(TouchInjectorWinTest, MoveTouchPoint) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(0u);

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kMoveFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);
  event.set_event_type(TouchEvent::TOUCH_POINT_MOVE);
  injector_.InjectTouchEvent(event);
}

// Start a point and then move, make sure the flag is set to kEndFlag.
TEST_F(TouchInjectorWinTest, EndTouchPoint) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(0u);

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kEndFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);
  event.set_event_type(TouchEvent::TOUCH_POINT_END);
  injector_.InjectTouchEvent(event);
}

// Start a point and then move, make sure the flag is set to kCancelFlag.
TEST_F(TouchInjectorWinTest, CancelTouchPoint) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  TouchEvent event;
  event.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point = event.add_touch_points();
  point->set_id(0u);

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kCancelFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());
  injector_.InjectTouchEvent(event);
  event.set_event_type(TouchEvent::TOUCH_POINT_CANCEL);
  injector_.InjectTouchEvent(event);
}

// Note that points that haven't changed should be injected as MOVE.
// This tests:
// 1. Start first touch point.
// 2. Start second touch point.
// 3. Move both touch points.
// 4. Start third touch point.
// 5. End second touch point.
// 6. Cancel remaining (first and third) touch points.
TEST_F(TouchInjectorWinTest, MultiTouch) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(1, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kMoveFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kMoveFlag;
  id_to_flags[2u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(3, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kEndFlag;
  id_to_flags[2u] = kMoveFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(3, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  id_to_flags.erase(1u);
  id_to_flags[0u] = kCancelFlag;
  id_to_flags[2u] = kCancelFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(1));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());

  // Start first touch point.
  TouchEvent first_touch_start;
  first_touch_start.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point0 = first_touch_start.add_touch_points();
  point0->set_id(0u);
  injector_.InjectTouchEvent(first_touch_start);

  // Add second touch point.
  TouchEvent second_touch_start;
  second_touch_start.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point1 = second_touch_start.add_touch_points();
  point1->set_id(1u);
  injector_.InjectTouchEvent(second_touch_start);

  // Move both touch points.
  TouchEvent move_both;
  move_both.set_event_type(TouchEvent::TOUCH_POINT_MOVE);
  point0 = second_touch_start.add_touch_points();
  point1 = second_touch_start.add_touch_points();
  point0->set_id(0u);
  point1->set_id(1u);
  injector_.InjectTouchEvent(move_both);

  // Add another.
  TouchEvent third_touch_start;
  third_touch_start.set_event_type(TouchEvent::TOUCH_POINT_START);
  TouchEventPoint* point2 = third_touch_start.add_touch_points();
  point2->set_id(2u);
  injector_.InjectTouchEvent(third_touch_start);

  // Release second touch point.
  TouchEvent release_second;
  release_second.set_event_type(TouchEvent::TOUCH_POINT_END);
  point1 = release_second.add_touch_points();
  point1->set_id(1u);
  injector_.InjectTouchEvent(release_second);

  // Cancel the remaining two points.
  TouchEvent cancel_rest;
  cancel_rest.set_event_type(TouchEvent::TOUCH_POINT_CANCEL);
  point0 = cancel_rest.add_touch_points();
  point0->set_id(0u);
  point2 = cancel_rest.add_touch_points();
  point2->set_id(2u);
  injector_.InjectTouchEvent(cancel_rest);
}

TEST_F(TouchInjectorWinTest, InjectKeepAliveMoves) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  id_to_flags[1u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(2));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kMoveFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .Times(2)
      .WillRepeatedly(Return(2));

  id_to_flags[0u] = kEndFlag;
  id_to_flags[1u] = kEndFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(2));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());

  TouchEvent touch_start;
  touch_start.set_event_type(TouchEvent::TOUCH_POINT_START);
  touch_start.add_touch_points()->set_id(0u);
  touch_start.add_touch_points()->set_id(1u);
  injector_.InjectTouchEvent(touch_start);

  // Inject 2 keep-alive moves.
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval);
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval);

  TouchEvent touch_end;
  touch_end.set_event_type(TouchEvent::TOUCH_POINT_END);
  touch_end.add_touch_points()->set_id(0u);
  touch_end.add_touch_points()->set_id(1u);
  injector_.InjectTouchEvent(touch_end);

  // No longer injects keep-alive moves after all active touches have ended.
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval);
}

TEST_F(TouchInjectorWinTest,
       DoesNotInjectKeepAliveMovesIfTouchesAreUpdatedFrequently) {
  std::unique_ptr<TouchInjectorWinDelegateMock> delegate_mock(
      new ::testing::StrictMock<TouchInjectorWinDelegateMock>());

  InSequence s;
  EXPECT_CALL(*delegate_mock, InitializeTouchInjection(_, _))
      .WillOnce(Return(1));

  IdFlagMap id_to_flags;
  id_to_flags[0u] = kStartFlag;
  id_to_flags[1u] = kStartFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(2));

  id_to_flags[0u] = kMoveFlag;
  id_to_flags[1u] = kMoveFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .Times(2)
      .WillRepeatedly(Return(2));

  id_to_flags[0u] = kEndFlag;
  id_to_flags[1u] = kEndFlag;
  EXPECT_CALL(*delegate_mock,
              InjectTouchInput(2, EqualsPointerTouchInfoFlag(id_to_flags)))
      .WillOnce(Return(2));

  injector_.SetInjectorDelegateForTest(std::move(delegate_mock));
  EXPECT_TRUE(injector_.Init());

  TouchEvent touch_start;
  touch_start.set_event_type(TouchEvent::TOUCH_POINT_START);
  touch_start.add_touch_points()->set_id(0u);
  touch_start.add_touch_points()->set_id(1u);
  injector_.InjectTouchEvent(touch_start);
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval / 2);

  // The delegate should see exactly 2 touch move events, since events are
  // injected in an interval shorter than kKeepAliveInterval, so no keep-alive
  // events are synthesized.
  TouchEvent touch_move;
  touch_move.set_event_type(TouchEvent::TOUCH_POINT_MOVE);
  touch_move.add_touch_points()->set_id(0u);
  touch_move.add_touch_points()->set_id(1u);
  injector_.InjectTouchEvent(touch_move);
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval / 2);

  injector_.InjectTouchEvent(touch_move);
  task_environment_.FastForwardBy(TouchInjectorWin::kKeepAliveInterval / 2);

  TouchEvent touch_end;
  touch_end.set_event_type(TouchEvent::TOUCH_POINT_END);
  touch_end.add_touch_points()->set_id(0u);
  touch_end.add_touch_points()->set_id(1u);
  injector_.InjectTouchEvent(touch_end);
}

}  // namespace
}  // namespace remoting

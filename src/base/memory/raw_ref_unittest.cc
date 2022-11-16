// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"

#include <functional>
#include <type_traits>

#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class BaseClass {};
class SubClass : public BaseClass {};

// raw_ref just defers to the superclass for implementations, so it
// can't add more data types.
static_assert(sizeof(raw_ref<int>) == sizeof(raw_ptr<int>));

// Since it can't hold null, raw_ref is not default-constructible.
static_assert(!std::is_default_constructible_v<raw_ref<int>>);
static_assert(!std::is_default_constructible_v<raw_ref<const int>>);

// A mutable reference can only be constructed from a mutable lvalue reference.
static_assert(!std::is_constructible_v<raw_ref<int>, const int>);
static_assert(!std::is_constructible_v<raw_ref<int>, int>);
static_assert(!std::is_constructible_v<raw_ref<int>, const int&>);
static_assert(std::is_constructible_v<raw_ref<int>, int&>);
static_assert(!std::is_constructible_v<raw_ref<int>, const int*>);
static_assert(!std::is_constructible_v<raw_ref<int>, int*>);
static_assert(!std::is_constructible_v<raw_ref<int>, const int&&>);
static_assert(!std::is_constructible_v<raw_ref<int>, int&&>);
// Same for assignment.
static_assert(!std::is_assignable_v<raw_ref<int>, const int>);
static_assert(!std::is_assignable_v<raw_ref<int>, int>);
static_assert(!std::is_assignable_v<raw_ref<int>, const int&>);
static_assert(std::is_assignable_v<raw_ref<int>, int&>);
static_assert(!std::is_assignable_v<raw_ref<int>, const int*>);
static_assert(!std::is_assignable_v<raw_ref<int>, int*>);
static_assert(!std::is_assignable_v<raw_ref<int>, const int&&>);
static_assert(!std::is_assignable_v<raw_ref<int>, int&&>);

// A const reference can be constructed from a const or mutable lvalue
// reference.
static_assert(!std::is_constructible_v<raw_ref<const int>, const int>);
static_assert(!std::is_constructible_v<raw_ref<const int>, int>);
static_assert(std::is_constructible_v<raw_ref<const int>, const int&>);
static_assert(std::is_constructible_v<raw_ref<const int>, int&>);
static_assert(!std::is_constructible_v<raw_ref<const int>, const int*>);
static_assert(!std::is_constructible_v<raw_ref<const int>, int*>);
static_assert(!std::is_constructible_v<raw_ref<const int>, const int&&>);
static_assert(!std::is_constructible_v<raw_ref<const int>, int&&>);
// Same for assignment.
static_assert(!std::is_assignable_v<raw_ref<const int>, const int>);
static_assert(!std::is_assignable_v<raw_ref<const int>, int>);
static_assert(std::is_assignable_v<raw_ref<const int>, const int&>);
static_assert(std::is_assignable_v<raw_ref<const int>, int&>);
static_assert(!std::is_assignable_v<raw_ref<const int>, const int*>);
static_assert(!std::is_assignable_v<raw_ref<const int>, int*>);
static_assert(!std::is_assignable_v<raw_ref<const int>, const int&&>);
static_assert(!std::is_assignable_v<raw_ref<const int>, int&&>);

// Same trivial operations (or not) as raw_ptr<T>.
static_assert(std::is_trivially_constructible_v<raw_ref<int>, const int&> ==
              std::is_trivially_constructible_v<raw_ptr<int>, const int&>);
static_assert(std::is_trivially_destructible_v<raw_ref<int>> ==
              std::is_trivially_destructible_v<raw_ptr<int>>);
// But constructing from another raw_ref must check if it's internally null
// (which indicates use-after-move).
static_assert(!std::is_trivially_move_constructible_v<raw_ref<int>>);
static_assert(!std::is_trivially_move_assignable_v<raw_ref<int>>);
static_assert(!std::is_trivially_copy_constructible_v<raw_ref<int>>);
static_assert(!std::is_trivially_copy_assignable_v<raw_ref<int>>);

// A raw_ref can be copied or moved.
static_assert(std::is_move_constructible_v<raw_ref<int>>);
static_assert(std::is_copy_constructible_v<raw_ref<int>>);
static_assert(std::is_move_assignable_v<raw_ref<int>>);
static_assert(std::is_copy_assignable_v<raw_ref<int>>);

// A SubClass can be converted to a BaseClass.
static_assert(std::is_constructible_v<raw_ref<BaseClass>, raw_ref<SubClass>>);
static_assert(
    std::is_constructible_v<raw_ref<BaseClass>, const raw_ref<SubClass>&>);
static_assert(std::is_constructible_v<raw_ref<BaseClass>, raw_ref<SubClass>&&>);
static_assert(std::is_assignable_v<raw_ref<BaseClass>, raw_ref<SubClass>>);
static_assert(
    std::is_assignable_v<raw_ref<BaseClass>, const raw_ref<SubClass>&>);
static_assert(std::is_assignable_v<raw_ref<BaseClass>, raw_ref<SubClass>&&>);
// A BaseClass can't be implicitly downcasted.
static_assert(!std::is_constructible_v<raw_ref<SubClass>, raw_ref<BaseClass>>);
static_assert(
    !std::is_constructible_v<raw_ref<SubClass>, const raw_ref<BaseClass>&>);
static_assert(
    !std::is_constructible_v<raw_ref<SubClass>, raw_ref<BaseClass>&&>);
static_assert(!std::is_assignable_v<raw_ref<SubClass>, raw_ref<BaseClass>>);
static_assert(
    !std::is_assignable_v<raw_ref<SubClass>, const raw_ref<BaseClass>&>);
static_assert(!std::is_assignable_v<raw_ref<SubClass>, raw_ref<BaseClass>&&>);

// A raw_ref<BaseClass> can be constructed directly from a SubClass.
static_assert(std::is_constructible_v<raw_ref<BaseClass>, SubClass&>);
static_assert(std::is_assignable_v<raw_ref<BaseClass>, SubClass&>);
static_assert(std::is_constructible_v<raw_ref<const BaseClass>, SubClass&>);
static_assert(std::is_assignable_v<raw_ref<const BaseClass>, SubClass&>);
static_assert(
    std::is_constructible_v<raw_ref<const BaseClass>, const SubClass&>);
static_assert(std::is_assignable_v<raw_ref<const BaseClass>, const SubClass&>);
// But a raw_ref<SubClass> can't be constructed from an implicit downcast from a
// BaseClass.
static_assert(!std::is_constructible_v<raw_ref<SubClass>, BaseClass&>);
static_assert(!std::is_assignable_v<raw_ref<SubClass>, BaseClass&>);
static_assert(!std::is_constructible_v<raw_ref<const SubClass>, BaseClass&>);
static_assert(!std::is_assignable_v<raw_ref<const SubClass>, BaseClass&>);
static_assert(
    !std::is_constructible_v<raw_ref<const SubClass>, const BaseClass&>);
static_assert(!std::is_assignable_v<raw_ref<const SubClass>, const BaseClass&>);

// A mutable reference can be converted to const reference.
static_assert(std::is_constructible_v<raw_ref<const int>, raw_ref<int>>);
static_assert(std::is_assignable_v<raw_ref<const int>, raw_ref<int>>);
// A const reference can't be converted to mutable.
static_assert(!std::is_constructible_v<raw_ref<int>, raw_ref<const int>>);
static_assert(!std::is_assignable_v<raw_ref<int>, raw_ref<const int>>);

// The deref operator gives the internal reference.
static_assert(std::is_same_v<int&, decltype(*std::declval<raw_ref<int>>())>);
static_assert(
    std::is_same_v<int&, decltype(*std::declval<const raw_ref<int>>())>);
static_assert(std::is_same_v<int&, decltype(*std::declval<raw_ref<int>&>())>);
static_assert(
    std::is_same_v<int&, decltype(*std::declval<const raw_ref<int>&>())>);
static_assert(std::is_same_v<int&, decltype(*std::declval<raw_ref<int>&&>())>);
static_assert(
    std::is_same_v<int&, decltype(*std::declval<const raw_ref<int>&&>())>);
// A const T is always returned as const.
static_assert(
    std::is_same_v<const int&, decltype(*std::declval<raw_ref<const int>>())>);

// The arrow operator gives a (non-null) pointer to the internal reference.
static_assert(
    std::is_same_v<int*, decltype(std::declval<raw_ref<int>>().operator->())>);
static_assert(
    std::is_same_v<const int*,
                   decltype(std::declval<raw_ref<const int>>().operator->())>);

TEST(RawRef, Construct) {
  int i = 1;
  auto r = raw_ref<int>(i);
  EXPECT_EQ(&*r, &i);
  auto cr = raw_ref<const int>(i);
  EXPECT_EQ(&*cr, &i);
  const int ci = 1;
  auto cci = raw_ref<const int>(ci);
  EXPECT_EQ(&*cci, &ci);
}

TEST(RawRef, CopyConstruct) {
  {
    int i = 1;
    auto r = raw_ref<int>(i);
    EXPECT_EQ(&*r, &i);
    auto r2 = raw_ref<int>(r);
    EXPECT_EQ(&*r2, &i);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    auto r2 = raw_ref<const int>(r);
    EXPECT_EQ(&*r2, &i);
  }
}

TEST(RawRefDeathTest, CopyConstructAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  EXPECT_CHECK_DEATH({ [[maybe_unused]] auto r3 = r; });
}

TEST(RawRef, MoveConstruct) {
  {
    int i = 1;
    auto r = raw_ref<int>(i);
    EXPECT_EQ(&*r, &i);
    auto r2 = raw_ref<int>(std::move(r));
    EXPECT_EQ(&*r2, &i);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    auto r2 = raw_ref<const int>(std::move(r));
    EXPECT_EQ(&*r2, &i);
  }
}

TEST(RawRefDeathTest, MoveConstructAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  EXPECT_CHECK_DEATH({ [[maybe_unused]] auto r3 = std::move(r); });
}

TEST(RawRef, CopyAssign) {
  {
    int i = 1;
    auto r = raw_ref<int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    auto rj = raw_ref<int>(j);
    r = rj;
    EXPECT_EQ(&*r, &j);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    auto rj = raw_ref<const int>(j);
    r = rj;
    EXPECT_EQ(&*r, &j);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    auto rj = raw_ref<int>(j);
    r = rj;
    EXPECT_EQ(&*r, &j);
  }
}

TEST(RawRefDeathTest, CopyAssignAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  EXPECT_CHECK_DEATH({ r2 = r; });
}

TEST(RawRef, CopyReassignAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  int j = 1;
  r2 = raw_ref<int>(j);
  // Reassign to the moved-from `r` so it can be used again.
  r = r2;
  EXPECT_EQ(&*r, &j);
}

TEST(RawRef, MoveAssign) {
  {
    int i = 1;
    auto r = raw_ref<int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    r = raw_ref<int>(j);
    EXPECT_EQ(&*r, &j);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    r = raw_ref<const int>(j);
    EXPECT_EQ(&*r, &j);
  }
  {
    int i = 1;
    auto r = raw_ref<const int>(i);
    EXPECT_EQ(&*r, &i);
    int j = 2;
    r = raw_ref<int>(j);
    EXPECT_EQ(&*r, &j);
  }
}

TEST(RawRefDeathTest, MoveAssignAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  EXPECT_CHECK_DEATH({ r2 = std::move(r); });
}

TEST(RawRef, MoveReassignAfterMove) {
  int i = 1;
  auto r = raw_ref<int>(i);
  auto r2 = std::move(r);
  int j = 1;
  // Reassign to the moved-from `r` so it can be used again.
  r = raw_ref<int>(j);
  EXPECT_EQ(&*r, &j);
}

TEST(RawRef, CopyConstructUpCast) {
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    EXPECT_EQ(&*r, &s);
    auto r2 = raw_ref<BaseClass>(r);
    EXPECT_EQ(&*r2, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<const SubClass>(s);
    EXPECT_EQ(&*r, &s);
    auto r2 = raw_ref<const BaseClass>(r);
    EXPECT_EQ(&*r2, &s);
  }
}

TEST(RawRefDeathTest, CopyConstructAfterMoveUpCast) {
  auto s = SubClass();
  auto r = raw_ref<SubClass>(s);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH({ [[maybe_unused]] auto r2 = raw_ref<BaseClass>(r); });
}

TEST(RawRef, MoveConstructUpCast) {
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    EXPECT_EQ(&*r, &s);
    auto r2 = raw_ref<BaseClass>(std::move(r));
    EXPECT_EQ(&*r2, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<const SubClass>(s);
    EXPECT_EQ(&*r, &s);
    auto r2 = raw_ref<const BaseClass>(std::move(r));
    EXPECT_EQ(&*r2, &s);
  }
}

TEST(RawRefDeathTest, MoveConstructAfterMoveUpCast) {
  auto s = SubClass();
  auto r = raw_ref<SubClass>(s);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH(
      { [[maybe_unused]] auto r2 = raw_ref<BaseClass>(std::move(r)); });
}

TEST(RawRef, CopyAssignUpCast) {
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<BaseClass>(t);
    rt = r;
    EXPECT_EQ(&*rt, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<const SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<const BaseClass>(t);
    rt = r;
    EXPECT_EQ(&*rt, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<const BaseClass>(t);
    rt = r;
    EXPECT_EQ(&*rt, &s);
  }
}

TEST(RawRefDeathTest, CopyAssignAfterMoveUpCast) {
  auto s = SubClass();
  auto r = raw_ref<const SubClass>(s);
  auto t = BaseClass();
  auto rt = raw_ref<const BaseClass>(t);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH({ rt = r; });
}

TEST(RawRef, MoveAssignUpCast) {
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<BaseClass>(t);
    rt = std::move(r);
    EXPECT_EQ(&*rt, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<const SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<const BaseClass>(t);
    rt = std::move(r);
    EXPECT_EQ(&*rt, &s);
  }
  {
    auto s = SubClass();
    auto r = raw_ref<SubClass>(s);
    auto t = BaseClass();
    auto rt = raw_ref<const BaseClass>(t);
    rt = std::move(r);
    EXPECT_EQ(&*rt, &s);
  }
}

TEST(RawRefDeathTest, MoveAssignAfterMoveUpCast) {
  auto s = SubClass();
  auto r = raw_ref<const SubClass>(s);
  auto t = BaseClass();
  auto rt = raw_ref<const BaseClass>(t);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH({ rt = std::move(r); });
}

TEST(RawRef, Deref) {
  int i;
  auto r = raw_ref<int>(i);
  EXPECT_EQ(&*r, &i);
}

TEST(RawRefDeathTest, DerefAfterMove) {
  int i;
  auto r = raw_ref<int>(i);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH({ r.operator*(); });
}

TEST(RawRef, Arrow) {
  int i;
  auto r = raw_ref<int>(i);
  EXPECT_EQ(r.operator->(), &i);
}

TEST(RawRefDeathTest, ArrowAfterMove) {
  int i;
  auto r = raw_ref<int>(i);
  auto moved = std::move(r);
  EXPECT_CHECK_DEATH({ r.operator->(); });
}

TEST(RawRef, Swap) {
  int i;
  auto ri = raw_ref<int>(i);
  int j;
  auto rj = raw_ref<int>(j);
  swap(ri, rj);
  EXPECT_EQ(&*ri, &j);
  EXPECT_EQ(&*rj, &i);
}

TEST(RawRefDeathTest, SwapAfterMove) {
  {
    int i;
    auto ri = raw_ref<int>(i);
    int j;
    auto rj = raw_ref<int>(j);

    auto moved = std::move(ri);
    EXPECT_CHECK_DEATH({ swap(ri, rj); });
  }
  {
    int i;
    auto ri = raw_ref<int>(i);
    int j;
    auto rj = raw_ref<int>(j);

    auto moved = std::move(rj);
    EXPECT_CHECK_DEATH({ swap(ri, rj); });
  }
}

TEST(RawRef, Equals) {
  int i = 1;
  auto r1 = raw_ref<int>(i);
  auto r2 = raw_ref<int>(i);
  EXPECT_TRUE(r1 == r1);
  EXPECT_TRUE(r1 == r2);
  EXPECT_TRUE(r1 == i);
  EXPECT_TRUE(i == r1);
  int j = 1;
  auto r3 = raw_ref<int>(j);
  EXPECT_FALSE(r1 == r3);
  EXPECT_FALSE(r1 == j);
  EXPECT_FALSE(j == r1);
}

TEST(RawRefDeathTest, EqualsAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 == r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 == r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 == r1; });
  }
}

TEST(RawRef, NotEquals) {
  int i = 1;
  auto r1 = raw_ref<int>(i);
  int j = 1;
  auto r2 = raw_ref<int>(j);
  EXPECT_TRUE(r1 != r2);
  EXPECT_TRUE(r1 != j);
  EXPECT_TRUE(j != r1);
  EXPECT_FALSE(r1 != r1);
  EXPECT_FALSE(r2 != j);
  EXPECT_FALSE(j != r2);
}

TEST(RawRefDeathTest, NotEqualsAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 != r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 != r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 != r1; });
  }
}

TEST(RawRef, LessThan) {
  int i[] = {1, 1};
  auto r1 = raw_ref<int>(i[0]);
  auto r2 = raw_ref<int>(i[1]);
  EXPECT_TRUE(r1 < r2);
  EXPECT_TRUE(r1 < i[1]);
  EXPECT_FALSE(i[1] < r1);
  EXPECT_FALSE(r2 < r1);
  EXPECT_FALSE(r2 < i[0]);
  EXPECT_TRUE(i[0] < r2);
  EXPECT_FALSE(r1 < r1);
  EXPECT_FALSE(r1 < i[0]);
  EXPECT_FALSE(i[0] < r1);
}

TEST(RawRefDeathTest, LessThanAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 < r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 < r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 < r1; });
  }
}

TEST(RawRef, GreaterThan) {
  int i[] = {1, 1};
  auto r1 = raw_ref<int>(i[0]);
  auto r2 = raw_ref<int>(i[1]);
  EXPECT_TRUE(r2 > r1);
  EXPECT_FALSE(r1 > r2);
  EXPECT_FALSE(r1 > i[1]);
  EXPECT_TRUE(i[1] > r1);
  EXPECT_FALSE(r2 > r2);
  EXPECT_FALSE(r2 > i[1]);
  EXPECT_FALSE(i[1] > r2);
}

TEST(RawRefDeathTest, GreaterThanAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 > r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 > r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 > r1; });
  }
}

TEST(RawRef, LessThanOrEqual) {
  int i[] = {1, 1};
  auto r1 = raw_ref<int>(i[0]);
  auto r2 = raw_ref<int>(i[1]);
  EXPECT_TRUE(r1 <= r2);
  EXPECT_TRUE(r1 <= r1);
  EXPECT_TRUE(r2 <= r2);
  EXPECT_FALSE(r2 <= r1);
  EXPECT_TRUE(r1 <= i[1]);
  EXPECT_TRUE(r1 <= i[0]);
  EXPECT_TRUE(r2 <= i[1]);
  EXPECT_FALSE(r2 <= i[0]);
  EXPECT_FALSE(i[1] <= r1);
  EXPECT_TRUE(i[0] <= r1);
  EXPECT_TRUE(i[1] <= r2);
  EXPECT_TRUE(i[0] <= r2);
}

TEST(RawRefDeathTest, LessThanOrEqualAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 <= r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 <= r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 <= r1; });
  }
}

TEST(RawRef, GreaterThanOrEqual) {
  int i[] = {1, 1};
  auto r1 = raw_ref<int>(i[0]);
  auto r2 = raw_ref<int>(i[1]);
  EXPECT_TRUE(r2 >= r1);
  EXPECT_TRUE(r1 >= r1);
  EXPECT_TRUE(r2 >= r2);
  EXPECT_FALSE(r1 >= r2);
  EXPECT_TRUE(r2 >= i[0]);
  EXPECT_TRUE(r1 >= i[0]);
  EXPECT_TRUE(r2 >= i[1]);
  EXPECT_FALSE(r1 >= i[1]);
  EXPECT_FALSE(i[0] >= r2);
  EXPECT_TRUE(i[0] >= r1);
  EXPECT_TRUE(i[1] >= r2);
  EXPECT_TRUE(i[1] >= r1);
}

TEST(RawRefDeathTest, GreaterThanOrEqualAfterMove) {
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 >= r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto r2 = raw_ref<int>(i);
    auto moved = std::move(r2);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 >= r2; });
  }
  {
    int i = 1;
    auto r1 = raw_ref<int>(i);
    auto moved = std::move(r1);
    EXPECT_CHECK_DEATH({ [[maybe_unused]] bool b = r1 >= r1; });
  }
}

TEST(RawRef, CTAD) {
  int i = 1;
  auto r = raw_ref(i);
  EXPECT_EQ(&*r, &i);
}

using RawPtrCountingImpl =
    base::internal::RawPtrCountingImplWrapperForTest<base::DefaultRawPtrImpl>;

template <typename T>
using CountingRawRef = raw_ref<T, RawPtrCountingImpl>;

TEST(RawRef, StdLess) {
  int i[] = {1, 1};
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<int>(i[0]);
    auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, r2));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, r1));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    const auto r1 = CountingRawRef<int>(i[0]);
    const auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, r2));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, r1));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<const int>(i[0]);
    auto r2 = CountingRawRef<const int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, r2));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, r1));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<const int>(i[0]);
    auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, r2));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, r1));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<int>(i[0]);
    auto r2 = CountingRawRef<const int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, r2));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, r1));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<int>(i[0]);
    auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, i[1]));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, i[0]));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    const auto r1 = CountingRawRef<int>(i[0]);
    const auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, i[1]));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, i[0]));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<const int>(i[0]);
    auto r2 = CountingRawRef<const int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, i[1]));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, i[0]));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<const int>(i[0]);
    auto r2 = CountingRawRef<int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, i[1]));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, i[0]));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
  {
    RawPtrCountingImpl::ClearCounters();
    auto r1 = CountingRawRef<int>(i[0]);
    auto r2 = CountingRawRef<const int>(i[1]);
    EXPECT_TRUE(std::less<CountingRawRef<int>>()(r1, i[1]));
    EXPECT_FALSE(std::less<CountingRawRef<int>>()(r2, i[0]));
    EXPECT_EQ(2, RawPtrCountingImpl::wrapped_ptr_less_cnt);
  }
}

}  // namespace

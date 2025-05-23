// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/page_action/page_action_container_view.h"

#include "chrome/browser/ui/views/page_action/page_action_constants.h"
#include "chrome/browser/ui/views/page_action/page_action_view.h"
#include "components/vector_icons/vector_icons.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/views/test/views_test_base.h"

namespace page_actions {
namespace {

class MockIconLabelViewDelegate : public IconLabelBubbleView::Delegate {
 public:
  MOCK_METHOD(SkColor,
              GetIconLabelBubbleSurroundingForegroundColor,
              (),
              (const, override));
  MOCK_METHOD(SkColor,
              GetIconLabelBubbleBackgroundColor,
              (),
              (const, override));
};

class PageActionContainerViewTest : public views::ViewsTestBase {
 public:
  PageActionContainerViewTest() = default;
  ~PageActionContainerViewTest() override = default;

  void TearDown() override {
    views::ViewsTestBase::TearDown();
    actions::ActionManager::Get().ResetActions();
  }
};

TEST_F(PageActionContainerViewTest, GetPageActionView) {
  MockIconLabelViewDelegate icon_label_view_delegate;
  actions::ActionItem* action_item = actions::ActionManager::Get().AddAction(
      actions::ActionItem::Builder()
          .SetImage(ui::ImageModel::FromVectorIcon(vector_icons::kBackArrowIcon,
                                                   ui::kColorSysPrimary,
                                                   /*icon_size=*/16))
          .SetActionId(0)
          .Build());

  auto page_action_container = std::make_unique<PageActionContainerView>(
      std::vector<actions::ActionItem*>{action_item},
      &icon_label_view_delegate);

  PageActionView* page_action_view =
      page_action_container->GetPageActionView(0);
  ASSERT_TRUE(!!page_action_view);
  EXPECT_EQ(0, page_action_view->GetActionId());

  // Returns null if the action ID is not found.
  EXPECT_EQ(nullptr, page_action_container->GetPageActionView(1));
}

TEST_F(PageActionContainerViewTest, EmptyView) {
  MockIconLabelViewDelegate icon_label_view_delegate;
  auto page_action_container = std::make_unique<PageActionContainerView>(
      std::vector<actions::ActionItem*>{}, &icon_label_view_delegate);

  EXPECT_EQ(gfx::Insets().set_right(0),
            page_action_container->GetInsideBorderInsets());
}

TEST_F(PageActionContainerViewTest, NonEmptyViewWithNoVisiblePageAction) {
  MockIconLabelViewDelegate icon_label_view_delegate;
  actions::ActionItem* action_item = actions::ActionManager::Get().AddAction(
      actions::ActionItem::Builder()
          .SetImage(ui::ImageModel::FromVectorIcon(vector_icons::kBackArrowIcon,
                                                   ui::kColorSysPrimary,
                                                   /*icon_size=*/16))
          .SetVisible(false)
          .SetActionId(0)
          .Build());

  auto page_action_container = std::make_unique<PageActionContainerView>(
      std::vector<actions::ActionItem*>{action_item},
      &icon_label_view_delegate);

  EXPECT_EQ(gfx::Insets().set_right(0),
            page_action_container->GetInsideBorderInsets());
}

TEST_F(PageActionContainerViewTest, NonEmptyViewWithVisiblePageAction) {
  MockIconLabelViewDelegate icon_label_view_delegate;
  actions::ActionItem* action_item1 = actions::ActionManager::Get().AddAction(
      actions::ActionItem::Builder()
          .SetImage(ui::ImageModel::FromVectorIcon(vector_icons::kBackArrowIcon,
                                                   ui::kColorSysPrimary,
                                                   /*icon_size=*/16))
          .SetVisible(false)
          .SetActionId(0)
          .Build());
  actions::ActionItem* action_item2 = actions::ActionManager::Get().AddAction(
      actions::ActionItem::Builder()
          .SetImage(ui::ImageModel::FromVectorIcon(vector_icons::kBackArrowIcon,
                                                   ui::kColorSysPrimary,
                                                   /*icon_size=*/16))
          .SetVisible(true)
          .SetActionId(1)
          .Build());

  auto page_action_container = std::make_unique<PageActionContainerView>(
      std::vector<actions::ActionItem*>{action_item1, action_item2},
      &icon_label_view_delegate);

  // Because no model exist, no page action will be visible.
  EXPECT_EQ(gfx::Insets().set_right(0),
            page_action_container->GetInsideBorderInsets());

  page_action_container->GetPageActionView(0)->SetVisible(true);

  EXPECT_EQ(gfx::Insets().set_right(kPageActionBetweenIconSpacing),
            page_action_container->GetInsideBorderInsets());
}

}  // namespace
}  // namespace page_actions

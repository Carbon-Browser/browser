// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_ai/save_autofill_ai_data_bubble_view.h"

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/autofill/autofill_ai/save_autofill_ai_data_controller.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/optimization_guide/proto/features/common_quality_data.pb.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/widget/widget_utils.h"

// TODO(crbug.com/362227379): Consider having an interactive UI test to evaluate
// both the controller and the view working together.
namespace autofill_ai {

class MockSaveAutofillAiDataController : public SaveAutofillAiDataController {
 public:
  MockSaveAutofillAiDataController() = default;
  MOCK_METHOD(
      void,
      OfferSave,
      (std::vector<optimization_guide::proto::UserAnnotationsEntry>,
       user_annotations::PromptAcceptanceCallback PromptAcceptanceCallback,
       LearnMoreClickedCallback,
       UserFeedbackCallback),
      (override));
  MOCK_METHOD(
      const std::vector<optimization_guide::proto::UserAnnotationsEntry>&,
      GetAutofillAiData,
      (),
      (const override));
  MOCK_METHOD(void, OnSaveButtonClicked, (), (override));
  MOCK_METHOD(void, OnThumbsUpClicked, (), (override));
  MOCK_METHOD(void, OnThumbsDownClicked, (), (override));
  MOCK_METHOD(void, OnLearnMoreClicked, (), (override));
  MOCK_METHOD(void, OnBubbleClosed, (AutofillAiBubbleClosedReason), (override));
  base::WeakPtr<SaveAutofillAiDataController> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<SaveAutofillAiDataController> weak_ptr_factory_{this};
};

class SaveAutofillAiDataBubbleViewTest : public ChromeViewsTestBase {
 public:
  SaveAutofillAiDataBubbleViewTest() = default;
  ~SaveAutofillAiDataBubbleViewTest() override = default;

  // views::ViewsTestBase:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  }

  void CreateViewAndShow();

  void TearDown() override {
    view_ = nullptr;
    anchor_widget_.reset();
    ChromeViewsTestBase::TearDown();
  }

  SaveAutofillAiDataBubbleView& view() { return *view_; }
  MockSaveAutofillAiDataController& mock_controller() {
    return mock_controller_;
  }

  void ClickButton(views::ImageButton* button) {
    CHECK(button);
    ui::MouseEvent e(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                     ui::EventTimeForNow(), 0, 0);
    views::test::ButtonTestApi test_api(button);
    test_api.NotifyClick(e);
  }

 private:
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  TestingProfile profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<views::Widget> anchor_widget_;
  raw_ptr<SaveAutofillAiDataBubbleView> view_ = nullptr;
  testing::NiceMock<MockSaveAutofillAiDataController> mock_controller_;
};

void SaveAutofillAiDataBubbleViewTest::CreateViewAndShow() {
  // The bubble needs the parent as an anchor.
  views::Widget::InitParams params =
      CreateParams(views::Widget::InitParams::CLIENT_OWNS_WIDGET,
                   views::Widget::InitParams::TYPE_WINDOW);
  anchor_widget_ = std::make_unique<views::Widget>();
  anchor_widget_->Init(std::move(params));
  anchor_widget_->Show();

  ON_CALL(mock_controller(), GetAutofillAiData())
      .WillByDefault(testing::ReturnRefOfCopy(
          std::vector<optimization_guide::proto::UserAnnotationsEntry>()));

  auto view_unique = std::make_unique<SaveAutofillAiDataBubbleView>(
      anchor_widget_->GetContentsView(), web_contents_.get(),
      &mock_controller_);
  view_ = view_unique.get();
  views::BubbleDialogDelegateView::CreateBubble(std::move(view_unique))->Show();
}

TEST_F(SaveAutofillAiDataBubbleViewTest, HasCloseButton) {
  CreateViewAndShow();
  EXPECT_TRUE(view().ShouldShowCloseButton());
}

TEST_F(SaveAutofillAiDataBubbleViewTest, AcceptInvokesTheController) {
  CreateViewAndShow();
  EXPECT_CALL(mock_controller(), OnSaveButtonClicked);
  view().AcceptDialog();
}

TEST_F(SaveAutofillAiDataBubbleViewTest, CancelInvokesTheController) {
  CreateViewAndShow();
  EXPECT_CALL(mock_controller(), OnBubbleClosed);
  view().CancelDialog();
}

TEST_F(SaveAutofillAiDataBubbleViewTest, ThumbsUpInvokesTheController) {
  CreateViewAndShow();

  // Assert that the controller respective method is called.
  EXPECT_CALL(mock_controller(), OnThumbsUpClicked);

  // Clicks on the thumbs up button.
  ClickButton(views::AsViewClass<views::ImageButton>(
      view().GetBubbleFrameView()->GetViewByID(
          SaveAutofillAiDataBubbleView::kThumbsUpButtonViewID)));
}

TEST_F(SaveAutofillAiDataBubbleViewTest, ThumbsDownInvokesTheController) {
  CreateViewAndShow();

  // Assert that the controller respective method is called.
  EXPECT_CALL(mock_controller(), OnThumbsDownClicked);

  // Clicks on the thumbs down button.
  ClickButton(views::AsViewClass<views::ImageButton>(
      view().GetBubbleFrameView()->GetViewByID(
          SaveAutofillAiDataBubbleView::kThumbsDownButtonViewID)));
}

TEST_F(SaveAutofillAiDataBubbleViewTest, LearnMoreClickTriggersCallback) {
  CreateViewAndShow();

  // Assert that the controller respective method is called.
  EXPECT_CALL(mock_controller(), OnLearnMoreClicked);

  auto* suggestion_text = views::AsViewClass<views::StyledLabel>(
      view().GetBubbleFrameView()->GetViewByID(
          SaveAutofillAiDataBubbleView::kLearnMoreStyledLabelViewID));
  suggestion_text->ClickFirstLinkForTesting();
}
}  // namespace autofill_ai

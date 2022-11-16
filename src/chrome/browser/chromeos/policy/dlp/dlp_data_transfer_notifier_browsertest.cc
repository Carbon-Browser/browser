// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/dlp/dlp_data_transfer_notifier.h"

#include "base/callback_helpers.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"

namespace policy {

namespace {

class MockDlpDataTransferNotifier : public DlpDataTransferNotifier {
 public:
  MockDlpDataTransferNotifier() = default;
  MockDlpDataTransferNotifier(const MockDlpDataTransferNotifier&) = delete;
  MockDlpDataTransferNotifier& operator=(const MockDlpDataTransferNotifier&) =
      delete;
  ~MockDlpDataTransferNotifier() override = default;

  // DlpDataTransferNotifier:
  void NotifyBlockedAction(
      const ui::DataTransferEndpoint* const data_src,
      const ui::DataTransferEndpoint* const data_dst) override {}

  using DlpDataTransferNotifier::CloseWidget;
  using DlpDataTransferNotifier::ShowBlockBubble;
  using DlpDataTransferNotifier::ShowWarningBubble;
  using DlpDataTransferNotifier::widget_;
};

}  // namespace

class DlpDataTransferNotifierBrowserTest : public InProcessBrowserTest {
 public:
  DlpDataTransferNotifierBrowserTest() = default;
  ~DlpDataTransferNotifierBrowserTest() override = default;

  DlpDataTransferNotifierBrowserTest(
      const DlpDataTransferNotifierBrowserTest&) = delete;
  DlpDataTransferNotifierBrowserTest& operator=(
      const DlpDataTransferNotifierBrowserTest&) = delete;

 protected:
  MockDlpDataTransferNotifier notifier_;
};

IN_PROC_BROWSER_TEST_F(DlpDataTransferNotifierBrowserTest, ShowBlockBubble) {
  EXPECT_FALSE(notifier_.widget_.get());
  notifier_.ShowBlockBubble(std::u16string());
  ASSERT_TRUE(notifier_.widget_.get());

  views::test::WidgetDestroyedWaiter waiter(notifier_.widget_.get());
  EXPECT_TRUE(notifier_.widget_->IsVisible());

  // The DLP notification bubble widget is initialized but never activated on
  // Lacros.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  EXPECT_TRUE(notifier_.widget_->IsActive());
#endif

  notifier_.CloseWidget(notifier_.widget_.get(),
                        views::Widget::ClosedReason::kCloseButtonClicked);

  waiter.Wait();

  EXPECT_FALSE(notifier_.widget_.get());
}

IN_PROC_BROWSER_TEST_F(DlpDataTransferNotifierBrowserTest, ShowWarningBubble) {
  EXPECT_FALSE(notifier_.widget_.get());
  notifier_.ShowWarningBubble(std::u16string(), base::DoNothing(),
                              base::DoNothing());
  ASSERT_TRUE(notifier_.widget_.get());

  views::test::WidgetDestroyedWaiter waiter(notifier_.widget_.get());
  EXPECT_TRUE(notifier_.widget_->IsVisible());

  // The DLP notification bubble widget is initialized but never activated on
  // Lacros.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  EXPECT_TRUE(notifier_.widget_->IsActive());
#endif

  notifier_.CloseWidget(notifier_.widget_.get(),
                        views::Widget::ClosedReason::kAcceptButtonClicked);

  waiter.Wait();

  EXPECT_FALSE(notifier_.widget_.get());
}

}  // namespace policy

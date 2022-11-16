// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_controller.h"

#include "base/test/gtest_util.h"
#include "chrome/browser/ui/views/frame/test_with_browser_view.h"
#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_constants.h"
#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_model.h"
#include "chrome/browser/ui/webui/side_panel/read_anything/read_anything_prefs.h"
#include "testing/gmock/include/gmock/gmock.h"

class ReadAnythingControllerTest : public TestWithBrowserView {
 public:
  void SetUp() override {
    TestWithBrowserView::SetUp();

    model_ = std::make_unique<ReadAnythingModel>();
    controller_ =
        std::make_unique<ReadAnythingController>(model_.get(), browser());

    // Reset prefs to default values for test.
    browser()->profile()->GetPrefs()->SetString(
        prefs::kAccessibilityReadAnythingFontName,
        kReadAnythingDefaultFontName);
    browser()->profile()->GetPrefs()->SetDouble(
        prefs::kAccessibilityReadAnythingFontScale,
        kReadAnythingDefaultFontScale);
  }

  void MockOnFontChoiceChanged(int index) {
    controller_->OnFontChoiceChanged(index);
  }

  void MockOnFontSizeChanged(bool increase) {
    controller_->OnFontSizeChanged(increase);
  }

  void MockModelInit(std::string font_name, double font_scale) {
    model_->Init(font_name, font_scale);
  }

  std::string GetPrefFontName() {
    return browser()->profile()->GetPrefs()->GetString(
        prefs::kAccessibilityReadAnythingFontName);
  }

  double GetPrefFontScale() {
    return browser()->profile()->GetPrefs()->GetDouble(
        prefs::kAccessibilityReadAnythingFontScale);
  }

 protected:
  std::unique_ptr<ReadAnythingModel> model_;
  std::unique_ptr<ReadAnythingController> controller_;
};

TEST_F(ReadAnythingControllerTest, ValidIndexUpdatesFontNamePref) {
  std::string expected_font_name = "Avenir";

  MockOnFontChoiceChanged(3);

  EXPECT_EQ(expected_font_name, GetPrefFontName());
}

TEST_F(ReadAnythingControllerTest, OnFontSizeChangedIncreaseUpdatesPref) {
  EXPECT_NEAR(GetPrefFontScale(), 1.0, 0.01);

  MockOnFontSizeChanged(true);

  EXPECT_NEAR(GetPrefFontScale(), 1.2, 0.01);
}

TEST_F(ReadAnythingControllerTest, OnFontSizeChangedDecreasePref) {
  EXPECT_NEAR(GetPrefFontScale(), 1.0, 0.01);

  MockOnFontSizeChanged(false);

  EXPECT_NEAR(GetPrefFontScale(), 0.8, 0.01);
}

TEST_F(ReadAnythingControllerTest, OnFontSizeChangedHonorsMax) {
  EXPECT_NEAR(GetPrefFontScale(), 1.0, 0.01);

  std::string font_name;
  MockModelInit(font_name, 4.9);

  MockOnFontSizeChanged(true);

  EXPECT_NEAR(GetPrefFontScale(), 5.0, 0.01);
}

TEST_F(ReadAnythingControllerTest, OnFontSizeChangedHonorsMin) {
  EXPECT_NEAR(GetPrefFontScale(), 1.0, 0.01);

  std::string font_name;
  MockModelInit(font_name, 0.3);

  MockOnFontSizeChanged(false);

  EXPECT_NEAR(GetPrefFontScale(), 0.2, 0.01);
}

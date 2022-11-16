// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_detail_icon_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/icons/chrome_icon.h"
#import "ios/chrome/browser/ui/icons/chrome_symbol.h"
#import "ios/chrome/browser/ui/icons/custom_symbol.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/table_view/table_view_cells_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using TableViewDetailIconItemTest = PlatformTest;

// Returns the UIImage containing the icon within the cell.
UIImage* GetImage(TableViewDetailIconCell* cell) {
  UIView* container_view =
      base::mac::ObjCCastStrict<UIView>(cell.contentView.subviews[0]);
  UIImageView* image_view =
      base::mac::ObjCCastStrict<UIImageView>(container_view.subviews[0]);
  return image_view.image;
}

// Returns the UIView containing the SF Symbol within the cell.
UIView* GetSymbol(TableViewDetailIconCell* cell) {
  UIView* container_view =
      base::mac::ObjCCastStrict<UIView>(cell.contentView.subviews[0]);
  UIView* symbol_view =
      base::mac::ObjCCastStrict<UIView>(container_view.subviews[1]);
  return symbol_view;
}

}  // namespace

// Tests that the UILabels and icons are set properly after a call to
// `configureCell:`.
TEST_F(TableViewDetailIconItemTest, ItemProperties) {
  NSString* text = @"Cell text";
  NSString* detail_text = @"Cell detail text";

  TableViewDetailIconItem* item =
      [[TableViewDetailIconItem alloc] initWithType:0];
  item.text = text;
  item.detailText = detail_text;
  item.iconImageName = @"ic_search";
  item.textLayoutConstraintAxis = UILayoutConstraintAxisVertical;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewDetailIconCell class]]);

  TableViewDetailIconCell* detail_cell =
      base::mac::ObjCCastStrict<TableViewDetailIconCell>(cell);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:cell withStyler:styler];

  // Check text-based properties.
  EXPECT_NSEQ(text, detail_cell.textLabel.text);
  EXPECT_NSEQ(detail_text, detail_cell.detailTextLabel.text);
  EXPECT_EQ(UILayoutConstraintAxisVertical,
            detail_cell.textLayoutConstraintAxis);
  EXPECT_EQ([UIFont preferredFontForTextStyle:kTableViewSublabelFontStyle],
            detail_cell.detailTextLabel.font);

  // Check image-based property.
  EXPECT_EQ([ChromeIcon searchIcon], GetImage(detail_cell));
}

// Tests that the icon image is updated when set from cell.
TEST_F(TableViewDetailIconItemTest, iconImageUpdate) {
  TableViewDetailIconItem* item =
      [[TableViewDetailIconItem alloc] initWithType:0];
  item.iconImageName = @"ic_search";

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewDetailIconCell class]]);

  TableViewDetailIconCell* detail_cell =
      base::mac::ObjCCastStrict<TableViewDetailIconCell>(cell);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:cell withStyler:styler];

  // Check original image is set.
  EXPECT_EQ([ChromeIcon searchIcon], GetImage(detail_cell));

  [detail_cell setIconImage:[ChromeIcon infoIcon]];

  // Check new image is set.
  EXPECT_EQ([ChromeIcon infoIcon], GetImage(detail_cell));
}

// Tests that the icon image is removed when icon is set to nil from cell.
TEST_F(TableViewDetailIconItemTest, iconImageNilUpdate) {
  TableViewDetailIconItem* item =
      [[TableViewDetailIconItem alloc] initWithType:0];
  item.iconImageName = @"ic_search";

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewDetailIconCell class]]);

  TableViewDetailIconCell* detail_cell =
      base::mac::ObjCCastStrict<TableViewDetailIconCell>(cell);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:cell withStyler:styler];

  // Check original image is set.
  EXPECT_EQ([ChromeIcon searchIcon], GetImage(detail_cell));

  [detail_cell setIconImage:nil];

  // Check image is set to nil.
  ASSERT_EQ(nil, GetImage(detail_cell));
}

// Tests that the UI layout constraint axis for the text labels is updated to
// vertical when set from cell.
TEST_F(TableViewDetailIconItemTest, ItemUpdateUILayoutConstraintAxisVertical) {
  TableViewDetailIconItem* item =
      [[TableViewDetailIconItem alloc] initWithType:0];
  item.text = @"Jane Doe";
  item.detailText = @"janedoe@gmail.com";

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewDetailIconCell class]]);

  TableViewDetailIconCell* detail_cell =
      base::mac::ObjCCastStrict<TableViewDetailIconCell>(cell);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:cell withStyler:styler];

  // Check that the default layout is set to the horizontal axis.
  EXPECT_EQ(UILayoutConstraintAxisHorizontal,
            detail_cell.textLayoutConstraintAxis);
  EXPECT_EQ([UIFont preferredFontForTextStyle:UIFontTextStyleBody],
            detail_cell.detailTextLabel.font);

  [detail_cell setTextLayoutConstraintAxis:UILayoutConstraintAxisVertical];

  EXPECT_EQ(UILayoutConstraintAxisVertical,
            detail_cell.textLayoutConstraintAxis);
  EXPECT_EQ([UIFont preferredFontForTextStyle:kTableViewSublabelFontStyle],
            detail_cell.detailTextLabel.font);
}

// Tests that the SF Symbol view is updated when set from cell.
TEST_F(TableViewDetailIconItemTest, symbolViewUpdate) {
  UIView* symbolImageView1 = ElevatedTableViewSymbolWithBackground(
      DefaultSymbolWithPointSize(kGearShapeSymbol, 18), UIColor.redColor);
  UIView* symbolImageView2 = ElevatedTableViewSymbolWithBackground(
      DefaultSymbolWithPointSize(kGearShapeSymbol, 18), UIColor.blueColor);

  TableViewDetailIconItem* item =
      [[TableViewDetailIconItem alloc] initWithType:0];
  item.symbolView = symbolImageView1;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewDetailIconCell class]]);

  TableViewDetailIconCell* detail_cell =
      base::mac::ObjCCastStrict<TableViewDetailIconCell>(cell);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:cell withStyler:styler];

  // Check original symbol is set.
  UIView* cell_symbol = GetSymbol(detail_cell);
  EXPECT_EQ(symbolImageView1, cell_symbol);
  EXPECT_EQ(UIColor.redColor, cell_symbol.backgroundColor);

  [detail_cell setSymbolView:symbolImageView2];

  // Check new symbol is set.
  cell_symbol = GetSymbol(detail_cell);
  EXPECT_EQ(symbolImageView2, cell_symbol);
  EXPECT_EQ(UIColor.blueColor, cell_symbol.backgroundColor);
}

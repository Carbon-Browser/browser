// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_MAGIC_STACK_MAGIC_STACK_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_MAGIC_STACK_MAGIC_STACK_CONSUMER_H_

#import "base/ios/block_types.h"

@class MagicStackModule;

// Supports setting and updating the Magic Stack's datasource.
@protocol MagicStackConsumer

// Replace the Magic Stack's items with `items`.
- (void)populateItems:(NSArray<MagicStackModule*>*)items;

// Replace `oldItem` with `item`.
- (void)replaceItem:(MagicStackModule*)oldItem withItem:(MagicStackModule*)item;

// Insert `item` at `index` in the Magic Stack.
- (void)insertItem:(MagicStackModule*)item atIndex:(NSUInteger)index;

// Remove `item` from the Magic Stack. Animates if `animate` is YES. The
// `completion` will be executed after the item is removed.
- (void)removeItem:(MagicStackModule*)item
           animate:(BOOL)animate
    withCompletion:(ProceduralBlock)completion;

// Reconfigure existing item.
- (void)reconfigureItem:(MagicStackModule*)item;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_MAGIC_STACK_MAGIC_STACK_CONSUMER_H_

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SHARE_KIT_MODEL_SHARE_KIT_JOIN_CONFIGURATION_H_
#define IOS_CHROME_BROWSER_SHARE_KIT_MODEL_SHARE_KIT_JOIN_CONFIGURATION_H_

#import <UIKit/UIKit.h>

#import "components/data_sharing/public/group_data.h"

enum class ShareKitFlowOutcome;

// Configuration object for joining a shared group.
@interface ShareKitJoinConfiguration : NSObject

// The base view controller on which the join flow will be presented.
@property(nonatomic, weak) UIViewController* baseViewController;

// The token used to join the group, containing the collab ID and the secret.
@property(nonatomic, assign) data_sharing::GroupToken token;

// Executed when the join flow ended. The `result` parameter indicates whether
// the user successfully joined the group.
@property(nonatomic, copy) void (^completionBlock)(BOOL result);

// Executed when the join flow ended.
@property(nonatomic, copy) void (^completion)(ShareKitFlowOutcome outcome);

@end

#endif  // IOS_CHROME_BROWSER_SHARE_KIT_MODEL_SHARE_KIT_JOIN_CONFIGURATION_H_

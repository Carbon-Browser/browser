// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_FOLLOW_FOLLOW_UTIL_H_
#define IOS_CHROME_BROWSER_FOLLOW_FOLLOW_UTIL_H_

#import "ios/chrome/browser/follow/follow_action_state.h"

namespace web {
class WebState;
}

// Key used to store the last shown time of follow in-product help (IPH).
extern NSString* const kFollowIPHLastShownTime;
// Key used to store the last shown event of follow in-product help (IPH).
extern NSString* const kFollowIPHPreviousDisplayEvents;
// Key used to store the site host when showing the follow in-product help
// (IPH).
extern NSString* const kFollowIPHHost;
// Key used to store the date when showing the follow in-product help (IPH).
extern NSString* const kFollowIPHDate;

// Returns the Follow action state for |webState|.
FollowActionState GetFollowActionState(web::WebState* webState);

#pragma mark - For Follow IPH
// Returns true if the time between the last time a Follow IPH was shown and now
// is long enough for another Follow IPH appearance for website `host`.
bool IsFollowIPHShownFrequencyEligible(NSString* host);
// Stores the Follow IPH display event with website `host`.
void StoreFollowIPHDisplayEvent(NSString* host);

#endif  // IOS_CHROME_BROWSER_FOLLOW_FOLLOW_UTIL_H_

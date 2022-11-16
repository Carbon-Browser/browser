// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_H_

#include "base/callback_list.h"
#include "chrome/browser/share/share_attempt.h"
#include "chrome/browser/sharing_hub/sharing_hub_model.h"

namespace content {
class WebContents;
}  // namespace content

namespace sharing_hub {

class SharingHubBubbleView;

// Interface for the controller component of the sharing dialog bubble. Controls
// the Sharing Hub (Windows/Mac/Linux) or the Sharesheet (CrOS) depending on
// platform.
// Responsible for showing and hiding an associated dialog bubble.
class SharingHubBubbleController {
 public:
  static SharingHubBubbleController* CreateOrGetFromWebContents(
      content::WebContents* web_contents);

  // Hides the sharing bubble.
  virtual void HideBubble() = 0;
  // Displays the sharing bubble.
  virtual void ShowBubble(share::ShareAttempt attempt) = 0;

  // Returns nullptr if no bubble is currently shown.
  virtual SharingHubBubbleView* sharing_hub_bubble_view() const = 0;
  // Returns true if the omnibox icon should be shown.
  virtual bool ShouldOfferOmniboxIcon() = 0;

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_FUCHSIA)
  // These two methods return the sets of first- and third-party actions;
  // first-party actions are internal to Chrome and third-party actions are
  // other websites or apps.
  //
  // TODO(ellyjones): Could we feasibly pass these in when we construct
  // ShareAttempt? Does that make sense, even?
  virtual std::vector<SharingHubAction> GetFirstPartyActions() = 0;
  virtual std::vector<SharingHubAction> GetThirdPartyActions() = 0;

  // Returns whether the sharing hub should show a preview section or not.
  // TODO(ellyjones): Remove this once the preview section is launched.
  virtual bool ShouldUsePreview() = 0;

  // The sharing hub can load images asynchronously under some circumstances; to
  // allow for that, the controller allows clients to register a callback to be
  // notified when a new image is loaded.
  using PreviewImageChangedCallback =
      base::RepeatingCallback<void(ui::ImageModel)>;
  virtual base::CallbackListSubscription RegisterPreviewImageChangedCallback(
      PreviewImageChangedCallback callback) = 0;

  virtual base::WeakPtr<SharingHubBubbleController> GetWeakPtr() = 0;

  // Client code should call these when the corresponding things happen in the
  // View.
  virtual void OnBubbleClosed() = 0;
  virtual void OnActionSelected(int command_id,
                                bool is_first_party,
                                std::string feature_name_for_metrics) = 0;
#endif
};

}  // namespace sharing_hub

#endif  // CHROME_BROWSER_UI_SHARING_HUB_SHARING_HUB_BUBBLE_CONTROLLER_INTERFACE_H_

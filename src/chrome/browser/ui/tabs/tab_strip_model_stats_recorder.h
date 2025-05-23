// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_STATS_RECORDER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_STATS_RECORDER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace content {
class WebContents;
}

class BrowserTabStripTracker;

// TabStripModelStatsRecorder records user tab interaction stats.
// In particular, we record tab's lifetime and state transition probability to
// study user interaction with background tabs. (crbug.com/517335)
class TabStripModelStatsRecorder : public TabStripModelObserver {
 public:
  // TabState represents a lifecycle of a tab in TabStripModel.
  // This should match {Current,Next}TabState defined in
  // tools/metrics/histograms/histograms.xml, and
  // constants in Chrome for Android implementation
  // chrome/android/java/src/org/chromium/chrome/browser/tab/TabUma.java
  enum class TabState {
    // Initial tab state.
    kInitial = 0,

    // For active tabs visible in one of the browser windows.
    kActive = 1,

    // For inactive tabs which are present in the tab strip, but their contents
    // are not visible.
    kInactive = 2,

    // Skip 3 to match Chrome for Android implementation.

    // For tabs that are about to be closed.
    kClosed = 4,

    kMaxValue = kClosed,
  };

  TabStripModelStatsRecorder();
  TabStripModelStatsRecorder(const TabStripModelStatsRecorder&) = delete;
  TabStripModelStatsRecorder& operator=(const TabStripModelStatsRecorder&) =
      delete;
  ~TabStripModelStatsRecorder() override;

 private:
  // Called by OnTabStripModelChanged().
  void OnActiveTabChanged(content::WebContents* old_contents,
                          content::WebContents* new_contents,
                          int reason);
  void OnTabClosing(content::WebContents* contents);
  void OnTabReplaced(content::WebContents* old_contents,
                     content::WebContents* new_contents);

  // TabStripModelObserver implementation.
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  class TabInfo;

  std::vector<raw_ptr<content::WebContents, VectorExperimental>>
      active_tab_history_;

  // TODO(crbug.com/364501603): revert smart pointer once the modularization is
  // complete.
  std::unique_ptr<BrowserTabStripTracker> browser_tab_strip_tracker_;
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_STATS_RECORDER_H_

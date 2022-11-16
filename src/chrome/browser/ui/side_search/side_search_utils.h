// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SIDE_SEARCH_SIDE_SEARCH_UTILS_H_
#define CHROME_BROWSER_UI_SIDE_SEARCH_SIDE_SEARCH_UTILS_H_

#include <map>
#include <utility>

#include "chrome/browser/ui/side_search/side_search_tab_contents_helper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace content {
class WebContents;
}

namespace side_search {

// Adds side search state data to a tab's state restore data if applicable.
void MaybeAddSideSearchTabRestoreData(
    content::WebContents* web_contents,
    std::map<std::string, std::string>& extra_data);

// Returns side search tab restore state data if applicable or empty.
absl::optional<std::pair<std::string, std::string>>
MaybeGetSideSearchTabRestoreData(content::WebContents* web_contents);

// If applicable, persists the required tab data to be able to successfully
// restore the side search tab state on restoring a session.
void MaybeSaveSideSearchTabSessionData(content::WebContents* web_contents);

void SetSideSearchTabStateFromRestoreData(
    content::WebContents* web_contents,
    const std::map<std::string, std::string>& extra_data);

// Returns true if `web_contents` is to be embedded in the side panel for the
// side search feature.
bool IsSidePanelWebContents(content::WebContents* web_contents);

// Returns true if side search with DSE support for participating search engines
// is enabled.
bool IsDSESupportEnabled(const Profile* profile);

// Returns true if side search is enabled and is supported for `browser`.
bool IsEnabledForBrowser(const Browser* browser);

}  // namespace side_search

bool IsSideSearchEnabled(const Profile* profile);

#endif  // CHROME_BROWSER_UI_SIDE_SEARCH_SIDE_SEARCH_UTILS_H_

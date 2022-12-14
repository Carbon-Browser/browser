// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_COMMON_OMNIBOX_FEATURES_H_
#define COMPONENTS_OMNIBOX_COMMON_OMNIBOX_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace omnibox {

// Please do not add more features to this "big blob" list.
// Instead, use the categorized and alphabetized lists below this "big blob".
// You can create a new category if none of the existing ones fit.
extern const base::Feature kExperimentalKeywordMode;
extern const base::Feature kImageSearchSuggestionThumbnail;
extern const base::Feature kOmniboxRemoveSuggestionsFromClipboard;
extern const base::Feature kAndroidAuxiliarySearch;

// Flags that affect the "twiddle" step of AutocompleteResult, i.e. SortAndCull.
extern const base::Feature kAutocompleteStability;
extern const base::Feature kOmniboxDemoteByType;

// Features below this line should be sorted alphabetically by their comments.

// Num suggestions - these affect how many suggestions are shown based on e.g.
// focus, page context, provider, or URL v non-URL.
extern const base::Feature kMaxZeroSuggestMatches;
extern const base::Feature kUIExperimentMaxAutocompleteMatches;
// The default value is established here as a bool so it can be referred to in
// OmniboxFieldTrial.
extern const bool kOmniboxMaxURLMatchesEnabledByDefault;
extern const base::Feature kOmniboxMaxURLMatches;
extern const base::Feature kDynamicMaxAutocomplete;
extern const base::Feature kRetainSuggestionsWithHeaders;

// Local history zero-prefix (aka zero-suggest) and prefix suggestions.
extern const base::Feature kAdjustLocalHistoryZeroSuggestRelevanceScore;
extern const base::Feature kClobberTriggersContextualWebZeroSuggest;
extern const base::Feature kClobberTriggersSRPZeroSuggest;
extern const base::Feature kFocusTriggersContextualWebZeroSuggest;
extern const base::Feature kFocusTriggersSRPZeroSuggest;
extern const base::Feature kLocalHistorySuggestRevamp;
extern const base::Feature kOmniboxLocalZeroSuggestAgeThreshold;
extern const base::Feature kZeroSuggestOnNTPForSignedOutUsers;
extern const base::Feature kZeroSuggestPrefetching;
// Related, kMaxZeroSuggestMatches.

// On Device Head Suggest.
extern const base::Feature kOnDeviceHeadProviderIncognito;
extern const base::Feature kOnDeviceHeadProviderNonIncognito;

// Provider-specific - These features change the behavior of specific providers.
extern const base::Feature kOmniboxExperimentalSuggestScoring;
extern const base::Feature kHistoryQuickProviderAblateInMemoryURLIndexCacheFile;
extern const base::Feature kDisableCGIParamMatching;
extern const base::Feature kShortBookmarkSuggestions;
extern const base::Feature kShortBookmarkSuggestionsByTotalInputLength;
extern const base::Feature kBookmarkPaths;
extern const base::Feature kAggregateShortcuts;
extern const base::Feature kShortcutExpanding;
// TODO(crbug.com/1202964): Clean up feature flag used in staged roll-out of
// various CLs related to the contents/description clean-up work.
extern const base::Feature kStoreTitleInContentsAndUrlInDescription;

// Document provider
extern const base::Feature kDocumentProvider;
extern const base::Feature kDocumentProviderAso;

// Suggestions UI - these affect the UI or function of the suggestions popup.
extern const base::Feature kAdaptiveSuggestionsCount;
extern const base::Feature kClipboardSuggestionContentHidden;
extern const base::Feature kDocumentProviderDedupingOptimization;
extern const base::Feature kSuggestionAnswersColorReverse;
extern const base::Feature kMostVisitedTiles;
extern const base::Feature kMostVisitedTilesDynamicSpacing;
extern const base::Feature kMostVisitedTilesTitleWrapAround;
extern const base::Feature kRichAutocompletion;
extern const base::Feature kNtpRealboxPedals;
extern const base::Feature kNtpRealboxSuggestionAnswers;
extern const base::Feature kNtpRealboxTailSuggest;
extern const base::Feature kOmniboxFuzzyUrlSuggestions;
extern const base::Feature kOmniboxRemoveSuggestionHeaderChevron;
extern const base::Feature kStrippedGurlOptimization;

// Omnibox UI - these affect the UI or function of the location bar (not the
// popup).
extern const base::Feature kOmniboxAssistantVoiceSearch;

// Omnibox & Suggestions UI - these affect both the omnibox and the suggestions
// popup.
extern const base::Feature kClosePopupWithEscape;
extern const base::Feature kBlurWithEscape;

// Settings Page - these affect the appearance of the Search Engines settings
// page
extern const base::Feature kSiteSearchStarterPack;

// Experiment to introduce new security indicators for HTTPS.
extern const base::Feature kUpdatedConnectionSecurityIndicators;

// Navigation experiments.
extern const base::Feature kDefaultTypedNavigationsToHttps;
extern const char kDefaultTypedNavigationsToHttpsTimeoutParam[];

// Omnibox Logging.
extern const base::Feature kReportAssistedQueryStats;
extern const base::Feature kReportSearchboxStats;

}  // namespace omnibox

#endif  // COMPONENTS_OMNIBOX_COMMON_OMNIBOX_FEATURES_H_

// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_HISTORY_CLUSTER_PROVIDER_H_
#define COMPONENTS_OMNIBOX_BROWSER_HISTORY_CLUSTER_PROVIDER_H_

#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"

class AutocompleteProviderClient;
class SearchProvider;

// `HistoryClusterProvider` adds suggestions to the history journey page if
// any `SearchProvider` suggestions matched a cluster keyword. Inherits
// `AutocompleteProviderListener` in order to listen to `SearchProvider`
// updates. Uses `SearchProvider` suggestions, as a proxy for what the user may
// be typing if they're typing a query. Doesn't use other search providers'
// (i.e. `VoiceSearchProvider` and `ZeroSuggestProvider`) suggestions for
// simplicity.
class HistoryClusterProvider : public AutocompleteProvider,
                               public AutocompleteProviderListener {
 public:
  HistoryClusterProvider(AutocompleteProviderClient* client,
                         AutocompleteProviderListener* listener,
                         SearchProvider* search_provider);

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;

  // AutocompleteProviderListener:
  // `HistoryClusterProvider` listens to `SearchProvider` updates.
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override;

 private:
  ~HistoryClusterProvider() override = default;

  // Iterates `search_provider_->matches()` and check if any can be used to
  // create a history cluster match. Returns whether any matches were created.
  bool CreateMatches();

  // Creates a `AutocompleteMatch`.
  AutocompleteMatch CreateMatch(std::u16string text);

  // The `AutocompleteInput` passed to `Start()`.
  AutocompleteInput input_;

  // These are never null.
  const raw_ptr<AutocompleteProviderClient> client_;
  const raw_ptr<SearchProvider> search_provider_;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_HISTORY_CLUSTER_PROVIDER_H_

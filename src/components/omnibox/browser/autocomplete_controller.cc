// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_controller.h"

#include <inttypes.h>

#include <cstddef>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/format_macros.h"
#include "base/metrics/histogram_functions.h"
#include "base/observer_list.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/history_clusters/core/config.h"
#include "components/omnibox/browser/actions/omnibox_pedal_provider.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/bookmark_provider.h"
#include "components/omnibox/browser/builtin_provider.h"
#include "components/omnibox/browser/clipboard_provider.h"
#include "components/omnibox/browser/document_provider.h"
#include "components/omnibox/browser/history_fuzzy_provider.h"
#include "components/omnibox/browser/history_quick_provider.h"
#include "components/omnibox/browser/history_url_provider.h"
#include "components/omnibox/browser/keyword_provider.h"
#include "components/omnibox/browser/local_history_zero_suggest_provider.h"
#include "components/omnibox/browser/most_visited_sites_provider.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/on_device_head_provider.h"
#include "components/omnibox/browser/open_tab_provider.h"
#include "components/omnibox/browser/query_tile_provider.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/omnibox/browser/shortcuts_provider.h"
#include "components/omnibox/browser/voice_suggest_provider.h"
#include "components/omnibox/browser/zero_suggest_provider.h"
#include "components/omnibox/browser/zero_suggest_verbatim_match_provider.h"
#include "components/open_from_clipboard/clipboard_recent_content.h"
#include "components/search_engines/omnibox_focus_type.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_starter_pack_data.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/metrics_proto/chrome_searchbox_stats.pb.h"
#include "ui/base/device_form_factor.h"
#include "ui/base/l10n/l10n_util.h"

#if !BUILDFLAG(IS_IOS)
#include "components/omnibox/browser/actions/history_clusters_action.h"
#include "components/omnibox/browser/history_cluster_provider.h"
#include "components/open_from_clipboard/clipboard_recent_content_generic.h"
#endif

namespace {

// Appends available autocompletion of the given type, subtype, and number to
// the existing available autocompletions string, encoding according to the
// spec.
void AppendAvailableAutocompletion(size_t type,
                                   const base::flat_set<int>& subtypes,
                                   int count,
                                   std::string* autocompletions) {
  if (!autocompletions->empty())
    autocompletions->append("j");
  base::StringAppendF(autocompletions, "%" PRIuS, type);

  std::ostringstream subtype_str;
  for (auto subtype : subtypes) {
    if (subtype_str.tellp() > 0)
      subtype_str << 'i';
    subtype_str << subtype;
  }

  // Subtype is optional. Append only if we have subtypes to report.
  if (subtype_str.tellp() > 0)
    base::StringAppendF(autocompletions, "i%s", subtype_str.str().c_str());

  if (count > 1)
    base::StringAppendF(autocompletions, "l%d", count);
}

// Whether this autocomplete match type supports custom descriptions.
bool AutocompleteMatchHasCustomDescription(const AutocompleteMatch& match) {
  if (ui::GetDeviceFormFactor() == ui::DEVICE_FORM_FACTOR_DESKTOP &&
      match.type == AutocompleteMatchType::CALCULATOR) {
    return true;
  }
  return match.type == AutocompleteMatchType::SEARCH_SUGGEST_ENTITY ||
         match.type == AutocompleteMatchType::SEARCH_SUGGEST_PROFILE;
}

// Returns which rich autocompletion type, if any, had (or would have had for
// counterfactual variations) an impact; i.e. whether the top scoring rich
// autocompleted suggestion outscores the top scoring default suggestion.
AutocompleteMatch::RichAutocompletionType TopMatchRichAutocompletionType(
    const AutocompleteResult& result) {
  // Trigger rich autocompletion logging if the highest scoring match has
  // |rich_autocompletion_triggered| set indicating it is, or could have been
  // rich autocompleted. It's not sufficient to check the default match since
  // counterfactual variations will not allow rich autocompleted matches to be
  // the default match.
  if (result.empty())
    return AutocompleteMatch::RichAutocompletionType::kNone;

  auto get_sort_key = [](const AutocompleteMatch& match) {
    return std::make_tuple(
        match.allowed_to_be_default_match ||
            match.rich_autocompletion_triggered !=
                AutocompleteMatch::RichAutocompletionType::kNone,
        match.relevance);
  };

  auto top_match = base::ranges::max_element(result, {}, get_sort_key);
  return top_match->rich_autocompletion_triggered;
}

void RecordMatchDeletion(const AutocompleteMatch& match) {
  if (match.deletable) {
    // This formula combines provider and result type into a single enum as
    // defined in OmniboxProviderAndResultType in enums.xml.
    auto combined_type = match.provider->AsOmniboxEventProviderType() * 100 +
                         match.AsOmniboxEventResultType();
    // This histogram is defined in the internal histograms.xml. This is because
    // the vast majority of OmniboxProviderAndResultType histograms are
    // generated by internal tools, and we wish to keep them together.
    base::UmaHistogramSparse("Omnibox.SuggestionDeleted.ProviderAndResultType",
                             combined_type);
  }
}

}  // namespace

// static
void AutocompleteController::GetMatchTypeAndExtendSubtypes(
    const AutocompleteMatch& match,
    size_t* type,
    base::flat_set<int>* subtypes) {
  // This type indicates a native chrome suggestion.
  *type = 69;

  // If provider is TYPE_ZERO_SUGGEST_LOCAL_HISTORY, TYPE_ZERO_SUGGEST, or
  // TYPE_ON_DEVICE_HEAD, set the subtype accordingly. The type will be set in
  // the switch statement below for SEARCH_SUGGEST or NAVSUGGEST types.
  if (match.provider) {
    if (match.provider->type() == AutocompleteProvider::TYPE_ZERO_SUGGEST &&
        (match.type == AutocompleteMatchType::SEARCH_SUGGEST ||
         match.type == AutocompleteMatchType::NAVSUGGEST)) {
      // Make sure changes here are reflected in UpdateAssistedQueryStats()
      // below in which the zero-prefix suggestions are counted.
      if (match.type == AutocompleteMatchType::NAVSUGGEST) {
        subtypes->emplace(/*SUBTYPE_ZERO_PREFIX_LOCAL_FREQUENT_URLS=*/451);
      }
      // We abuse this subtype and use it to for zero-suggest suggestions that
      // aren't personalized by the server. That is, it indicates either
      // client-side most-likely URL suggestions or server-side suggestions
      // that depend only on the URL as context.
      subtypes->emplace(/*SUBTYPE_URL_BASED=*/66);
    } else if (match.provider->type() ==
               AutocompleteProvider::TYPE_ON_DEVICE_HEAD) {
      // This subtype indicates a match from an on-device head provider.
      subtypes->emplace(/*SUBTYPE_SUGGEST_2G_LITE=*/271);
      // Make sure changes here are reflected in UpdateAssistedQueryStats()
      // below in which the zero-prefix suggestions are counted.
    } else if (match.provider->type() ==
               AutocompleteProvider::TYPE_ZERO_SUGGEST_LOCAL_HISTORY) {
      subtypes->emplace(/*SUBTYPE_ZERO_PREFIX_LOCAL_HISTORY=*/450);
    }
  }

  switch (match.type) {
    case AutocompleteMatchType::SEARCH_SUGGEST: {
      // Do not set subtype here; subtype may have been set above.
      *type = 0;
      return;
    }
    case AutocompleteMatchType::SEARCH_SUGGEST_ENTITY: {
      *type = 46;
      return;
    }
    case AutocompleteMatchType::SEARCH_SUGGEST_TAIL: {
      *type = 33;
      return;
    }
    case AutocompleteMatchType::SEARCH_SUGGEST_PERSONALIZED: {
      *type = 35;
      subtypes->emplace(/*SUBTYPE_PERSONAL=*/39);
      return;
    }
    case AutocompleteMatchType::SEARCH_SUGGEST_PROFILE: {
      *type = 44;
      return;
    }
    case AutocompleteMatchType::NAVSUGGEST: {
      // Do not set subtype here; subtype may have been set above.
      *type = 5;
      return;
    }
    case AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_ECHO_SEARCH=*/57);
      return;
    }
    case AutocompleteMatchType::URL_WHAT_YOU_TYPED: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_ECHO_URL=*/58);
      return;
    }
    case AutocompleteMatchType::SEARCH_HISTORY: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_HISTORY_SEARCH=*/59);
      return;
    }
    case AutocompleteMatchType::HISTORY_URL: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_HISTORY_URL=*/60);
      return;
    }
    case AutocompleteMatchType::HISTORY_TITLE: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_HISTORY_TITLE=*/61);
      return;
    }
    case AutocompleteMatchType::HISTORY_BODY: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_HISTORY_BODY=*/62);
      return;
    }
    case AutocompleteMatchType::HISTORY_KEYWORD: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_HISTORY_KEYWORD=*/63);
      return;
    }
    case AutocompleteMatchType::BOOKMARK_TITLE: {
      subtypes->emplace(/*SUBTYPE_OMNIBOX_BOOKMARK_TITLE=*/65);
      return;
    }
    case AutocompleteMatchType::NAVSUGGEST_PERSONALIZED: {
      *type = 5;
      subtypes->emplace(/*SUBTYPE_PERSONAL=*/39);
      return;
    }
    case AutocompleteMatchType::CALCULATOR: {
      *type = 6;
      return;
    }
    case AutocompleteMatchType::CLIPBOARD_URL: {
      subtypes->emplace(/*SUBTYPE_CLIPBOARD_URL=*/177);
      return;
    }
    case AutocompleteMatchType::CLIPBOARD_TEXT: {
      subtypes->emplace(/*SUBTYPE_CLIPBOARD_TEXT=*/176);
      return;
    }
    case AutocompleteMatchType::CLIPBOARD_IMAGE: {
      subtypes->emplace(/*SUBTYPE_CLIPBOARD_IMAGE=*/327);
      return;
    }
    case AutocompleteMatchType::TILE_SUGGESTION: {
      *type = 171;
      return;
    }
    default: {
      // This value indicates a native chrome suggestion with no named subtype
      // (yet).
      subtypes->emplace(/*SUBTYPE_OMNIBOX_OTHER=*/64);
    }
  }
}

AutocompleteController::AutocompleteController(
    std::unique_ptr<AutocompleteProviderClient> provider_client,
    int provider_types)
    : provider_client_(std::move(provider_client)),
      bookmark_provider_(nullptr),
      document_provider_(nullptr),
      history_url_provider_(nullptr),
      keyword_provider_(nullptr),
      search_provider_(nullptr),
      zero_suggest_provider_(nullptr),
      on_device_head_provider_(nullptr),
      stop_timer_duration_(OmniboxFieldTrial::StopTimerFieldTrialDuration()),
      done_(true),
      in_start_(false),
      search_service_worker_signal_sent_(false),
      template_url_service_(provider_client_->GetTemplateURLService()) {
  provider_types &= ~OmniboxFieldTrial::GetDisabledProviderTypes();

  if (OmniboxFieldTrial::kAutocompleteStabilityAsyncProvidersFirst.Get()) {
    // Providers run in the order they're added. Run these async providers 1st
    // so their async requests can be kicked off before waiting a few
    // milliseconds for the other sync providers to complete.

    if (provider_types & AutocompleteProvider::TYPE_SEARCH) {
      search_provider_ = new SearchProvider(provider_client_.get(), this);
      providers_.push_back(search_provider_.get());
    }
    // Providers run in the order they're added.  Add `HistoryURLProvider` after
    // `SearchProvider` because:
    // - `SearchProvider` synchronously queries the history database's
    //   keyword_search_terms and url table.
    // - `HistoryUrlProvider` schedules a background task that also accesses the
    //    history database.
    // If both db accesses happen concurrently, TSan complains. So put
    // `HistoryURLProvider` later to make sure that `SearchProvider` is done
    // doing its thing by the time the `HistoryURLProvider` task runs. (And hope
    // that it completes before `AutocompleteController::Start()` is called the
    // next time.)
    if (provider_types & AutocompleteProvider::TYPE_HISTORY_URL) {
      history_url_provider_ =
          new HistoryURLProvider(provider_client_.get(), this);
      if (provider_types & AutocompleteProvider::TYPE_HISTORY_URL)
        providers_.push_back(history_url_provider_.get());
    }
    if (provider_types & AutocompleteProvider::TYPE_DOCUMENT) {
      document_provider_ =
          DocumentProvider::Create(provider_client_.get(), this);
      providers_.push_back(document_provider_.get());
    }
    if (provider_types & AutocompleteProvider::TYPE_ON_DEVICE_HEAD) {
      on_device_head_provider_ =
          OnDeviceHeadProvider::Create(provider_client_.get(), this);
      if (on_device_head_provider_) {
        providers_.push_back(on_device_head_provider_.get());
      }
    }
  }

  if (provider_types & AutocompleteProvider::TYPE_BOOKMARK) {
    bookmark_provider_ = new BookmarkProvider(provider_client_.get());
    providers_.push_back(bookmark_provider_.get());
  }
  if (provider_types & AutocompleteProvider::TYPE_BUILTIN)
    providers_.push_back(new BuiltinProvider(provider_client_.get()));
  if (provider_types & AutocompleteProvider::TYPE_HISTORY_QUICK) {
    history_quick_provider_ = new HistoryQuickProvider(provider_client_.get());
    providers_.push_back(history_quick_provider_.get());
  }
  if (provider_types & AutocompleteProvider::TYPE_KEYWORD) {
    keyword_provider_ = new KeywordProvider(provider_client_.get(), this);
    providers_.push_back(keyword_provider_.get());
  }
  if (!OmniboxFieldTrial::kAutocompleteStabilityAsyncProvidersFirst.Get()) {
    if (provider_types & AutocompleteProvider::TYPE_SEARCH) {
      search_provider_ = new SearchProvider(provider_client_.get(), this);
      providers_.push_back(search_provider_.get());
    }
    // Providers run in the order they're added.  Add `HistoryURLProvider` after
    // `SearchProvider` because:
    // - `SearchProvider` synchronously queries the history database's
    //   keyword_search_terms and url table.
    // - `HistoryUrlProvider` schedules a background task that also accesses the
    //    history database.
    // If both db accesses happen concurrently, TSan complains. So put
    // `HistoryURLProvider` later to make sure that `SearchProvider` is done
    // doing its thing by the time the `HistoryURLProvider` task runs. (And hope
    // that it completes before `AutocompleteController::Start()` is called the
    // next time.)
    if (provider_types & AutocompleteProvider::TYPE_HISTORY_URL) {
      history_url_provider_ =
          new HistoryURLProvider(provider_client_.get(), this);
      if (provider_types & AutocompleteProvider::TYPE_HISTORY_URL)
        providers_.push_back(history_url_provider_.get());
    }
  }
  if (provider_types & AutocompleteProvider::TYPE_SHORTCUTS)
    providers_.push_back(new ShortcutsProvider(provider_client_.get()));
  if (provider_types & AutocompleteProvider::TYPE_ZERO_SUGGEST) {
    zero_suggest_provider_ =
        ZeroSuggestProvider::Create(provider_client_.get(), this);
    if (zero_suggest_provider_)
      providers_.push_back(zero_suggest_provider_.get());
  }
  if (provider_types & AutocompleteProvider::TYPE_ZERO_SUGGEST_LOCAL_HISTORY) {
    providers_.push_back(
        LocalHistoryZeroSuggestProvider::Create(provider_client_.get(), this));
  }
  if (provider_types & AutocompleteProvider::TYPE_MOST_VISITED_SITES) {
    providers_.push_back(
        new MostVisitedSitesProvider(provider_client_.get(), this));
    // Note: the need for the always-present verbatim match originates from the
    // search-ready omnibox (SRO) in Incognito mode, where the
    // ZeroSuggestProvider intentionally never gets invoked.
    providers_.push_back(
        new ZeroSuggestVerbatimMatchProvider(provider_client_.get()));
  }
  if (!OmniboxFieldTrial::kAutocompleteStabilityAsyncProvidersFirst.Get()) {
    if (provider_types & AutocompleteProvider::TYPE_DOCUMENT) {
      document_provider_ =
          DocumentProvider::Create(provider_client_.get(), this);
      providers_.push_back(document_provider_.get());
    }
    if (provider_types & AutocompleteProvider::TYPE_ON_DEVICE_HEAD) {
      on_device_head_provider_ =
          OnDeviceHeadProvider::Create(provider_client_.get(), this);
      if (on_device_head_provider_) {
        providers_.push_back(on_device_head_provider_.get());
      }
    }
  }
  if (provider_types & AutocompleteProvider::TYPE_CLIPBOARD) {
#if !BUILDFLAG(IS_IOS)
    // On iOS, a global ClipboardRecentContent should've been created by now
    // (if enabled).  If none has been created (e.g., we're on a different
    // platform), use the generic implementation, which AutocompleteController
    // will own.  Don't try to create a generic implementation on iOS because
    // iOS doesn't want/need to link in the implementation and the libraries
    // that would come with it.
    if (!ClipboardRecentContent::GetInstance()) {
      ClipboardRecentContent::SetInstance(
          std::make_unique<ClipboardRecentContentGeneric>());
    }
#endif
    // ClipboardRecentContent can be null in iOS tests.  For non-iOS, we
    // create a ClipboardRecentContent as above (for both Chrome and tests).
    if (ClipboardRecentContent::GetInstance()) {
      clipboard_provider_ = new ClipboardProvider(
          provider_client_.get(), this, ClipboardRecentContent::GetInstance());
      providers_.push_back(clipboard_provider_.get());
    }
  }
  if (provider_types & AutocompleteProvider::TYPE_QUERY_TILE)
    providers_.push_back(new QueryTileProvider(provider_client_.get(), this));
  if (provider_types & AutocompleteProvider::TYPE_VOICE_SUGGEST) {
    voice_suggest_provider_ = new VoiceSuggestProvider(provider_client_.get());
    providers_.push_back(voice_suggest_provider_.get());
  }
  if (provider_types & AutocompleteProvider::TYPE_HISTORY_FUZZY) {
    providers_.push_back(new HistoryFuzzyProvider(
        provider_client_.get(), history_quick_provider_, bookmark_provider_));
  }
  if (provider_types & AutocompleteProvider::TYPE_OPEN_TAB) {
    open_tab_provider_ = new OpenTabProvider(provider_client_.get());
    providers_.push_back(open_tab_provider_.get());
  }
  // Ideally, we'd check `IsApplicationLocaleSupportedByJourneys()` when
  // constructing `provider_types`. But that's usually constructed in
  // `AutocompleteClassifier::DefaultOmniboxProviders` which can't depend on the
  // browser dir to detect locale. The alternative of piping in the locale from
  // each call site seems too intrusive for a temporary condition (some call
  // sites are also in the components dir). All callers of
  // `DefaultOmniboxProviders` only use it to then construct
  // `AutocompleteController`, so placing the check here instead has no behavior
  // change.
#if !BUILDFLAG(IS_IOS)
  // HistoryClusters is not enabled on iOS.
  if (provider_types & AutocompleteProvider::TYPE_HISTORY_CLUSTER_PROVIDER &&
      history_clusters::IsApplicationLocaleSupportedByJourneys(
          provider_client_->GetApplicationLocale()) &&
      search_provider_ != nullptr) {
    providers_.push_back(new HistoryClusterProvider(provider_client_.get(),
                                                    this, search_provider_));
  }
#endif

  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "AutocompleteController", base::ThreadTaskRunnerHandle::Get());
}

AutocompleteController::~AutocompleteController() {
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      this);

  // The providers may have tasks outstanding that hold refs to them.  We need
  // to ensure they won't call us back if they outlive us.  (Practically,
  // calling Stop() should also cancel those tasks and make it so that we hold
  // the only refs.)  We also don't want to bother notifying anyone of our
  // result changes here, because the notification observer is in the midst of
  // shutdown too, so we don't ask Stop() to clear |result_| (and notify).
  result_.Reset();  // Not really necessary.
  Stop(false);
}

void AutocompleteController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AutocompleteController::Start(const AutocompleteInput& input) {
  TRACE_EVENT1("omnibox", "AutocompleteController::Start", "text",
               base::UTF16ToUTF8(input.text()));

  // Providers assume synchronous inputs (`omit_asynchronous_matches() ==
  // true`) have default focus type (`focus_type() == DEFAULT`). See
  // crbug.com/1339425.
  DCHECK(!input.omit_asynchronous_matches() ||
         input.focus_type() == OmniboxFocusType::DEFAULT);

  // When input.omit_asynchronous_matches() is true, the AutocompleteController
  // is being used for text classification, which should not notify observers.
  // TODO(manukh): This seems unnecessary; `AutocompleteClassifier` and
  //   `OmniboxController` use separate instances of `AutocompleteController`,
  //   the former doesn't add observers, the latter always uses
  //   `omit_asynchrous_matches()` set to false. Besides, if that weren't the
  //   case, e.g. the classifier did add an observer, then
  //   `AutocompleteController` should respect that, not assume it's a mistake
  //   and silently ignore the observer. Audit all call paths of `::Start()` to
  //   remove this check.
  if (!input.omit_asynchronous_matches()) {
    for (Observer& obs : observers_)
      obs.OnStart(this, input);
  }

  // Must be called before `expire_timer_.Stop()`, modifying `done_`, or
  // modifying `AutocompleteProvider::done_` below. If the previous request has
  // not completed, and therefore has not been logged yet, will log it now.
  // Likewise, if the providers have not completed, and therefore have not been
  // logged yet, will log them now.
  metrics_.OnStart();

  const std::u16string old_input_text(input_.text());
  const bool old_allow_exact_keyword_match = input_.allow_exact_keyword_match();
  const bool old_omit_asynchronous_matches = input_.omit_asynchronous_matches();
  const OmniboxFocusType old_focus_type = input_.focus_type();
  input_ = input;

  // See if we can avoid rerunning autocomplete when the query hasn't changed
  // much.  When the user presses or releases the ctrl key, the desired_tld
  // changes, and when the user finishes an IME composition, inline autocomplete
  // may no longer be prevented.  In both these cases the text itself hasn't
  // changed since the last query, and some providers can do much less work (and
  // get matches back more quickly).  Taking advantage of this reduces flicker.
  //
  // NOTE: This comes after constructing |input_| above since that construction
  // can change the text string (e.g. by stripping off a leading '?').
  const bool minimal_changes =
      (input_.text() == old_input_text) &&
      (input_.allow_exact_keyword_match() == old_allow_exact_keyword_match) &&
      (input_.omit_asynchronous_matches() == old_omit_asynchronous_matches) &&
      (input_.focus_type() == old_focus_type);

  expire_timer_.Stop();
  stop_timer_.Stop();

  // Start the new query. Starter Pack engines in keyword mode only run a subset
  // of the providers, so call `ShouldRunProvider()` to determine which.
  in_start_ = true;
  // Use `start_time` rather than `metrics.start_time_` for
  // 'Omnibox.QueryTime2.*'. They differ by 3 μs, which though too small to be
  // distinguished in the ms-scale buckets, is large enough to move the
  // arithmetic mean.
  base::TimeTicks start_time = base::TimeTicks::Now();
  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;

    base::TimeTicks provider_start_time = base::TimeTicks::Now();
    provider->Start(input_, minimal_changes);
    // `UmaHistogramTimes()` uses 1ms - 10s buckets, whereas this uses 1ms - 5s
    // buckets.
    // TODO(crbug.com/1340291|manukh): This isn't handled by `metrics_` yet. It
    //   will "automatically" move to `metrics_` if we make all providers async.
    //   Otherwise, if we decide not to make all providers async, move this
    //   there.
    base::TimeTicks provider_end_time = base::TimeTicks::Now();
    base::UmaHistogramCustomTimes(
        std::string("Omnibox.ProviderTime2.") + provider->GetName(),
        provider_end_time - provider_start_time, base::Milliseconds(1),
        base::Seconds(5), 20);
  }
  if (!input.omit_asynchronous_matches() && (input.text().length() < 6)) {
    // `UmaHistogramTimes()` uses 1ms - 10s buckets, whereas this uses 1ms - 1s
    // buckets.
    // TODO(crbug.com/1340291|manukh): This isn't handled by `metrics_` yet. Do
    //   so after we decide whether to make all providers async.
    base::TimeTicks end_time = base::TimeTicks::Now();
    base::UmaHistogramCustomTimes(
        "Omnibox.QueryTime2." + base::NumberToString(input.text().length()),
        end_time - start_time, base::Milliseconds(1), base::Seconds(1), 50);
  }
  base::UmaHistogramBoolean("Omnibox.Start.WantAsyncMatches",
                            !input.omit_asynchronous_matches());

  // This will usually set |done_| to false, unless all providers are finished
  // after the synchronous pass we just completed.
  CheckIfDone();

  // The second true forces saying the default match has changed.
  // This triggers the edit model to update things such as the inline
  // autocomplete state.  In particular, if the user has typed a key
  // since the last notification, and we're now re-running
  // autocomplete, then we need to update the inline autocompletion
  // even if the current match is for the same URL as the last run's
  // default match.  Likewise, the controller doesn't know what's
  // happened in the edit since the last time it ran autocomplete.
  // The user might have selected all the text and hit delete, then
  // typed a new character.  The selection and delete won't send any
  // signals to the controller so it doesn't realize that anything was
  // cleared or changed.  Even if the default match hasn't changed, we
  // need the edit model to update the display.
  UpdateResult(false, true);

  in_start_ = false;

  // If the input looks like a query, send a signal predicting that the user is
  // going to issue a search (either to the default search engine or to a
  // keyword search engine, as indicated by the destination_url). This allows
  // any associated service worker to start up early and reduce the latency of a
  // resulting search. However, to avoid a potentially expensive operation, we
  // only do this once per session. Additionally, a default match is expected to
  // be available at this point but we check anyway to guard against an invalid
  // dereference.
  if (input.type() == metrics::OmniboxInputType::QUERY &&
      !search_service_worker_signal_sent_ && result_.default_match()) {
    search_service_worker_signal_sent_ = true;
    provider_client_->StartServiceWorker(
        result_.default_match()->destination_url);
  }

  if (!done_) {
    StartExpireTimer();
    StartStopTimer();
  }
}

void AutocompleteController::StartPrefetch(const AutocompleteInput& input) {
  // Start prefetch requests iff no non-prefetch request is in progress. Though
  // not likely, it is possible for the providers to have an active non-prefetch
  // request when a prefetch request is about to be started. In such scenarios,
  // starting a prefetch request will cause the providers to invalidate their
  // active non-prefetch requests and never get a chance to notify the
  // controller of their status; thus resulting in the controller to remain in
  // an invalid state.
  if (!done_) {
    return;
  }

  for (auto provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;

    provider->StartPrefetch(input);
  }
}

void AutocompleteController::Stop(bool clear_result) {
  StopHelper(clear_result, false);
}

void AutocompleteController::DeleteMatch(const AutocompleteMatch& match) {
  DCHECK(match.SupportsDeletion());

  // Delete duplicate matches attached to the main match first.
  for (const auto& duplicate_match : match.duplicate_matches) {
    if (duplicate_match.deletable)
      duplicate_match.provider->DeleteMatch(duplicate_match);
  }

  if (match.deletable) {
    RecordMatchDeletion(match);
    match.provider->DeleteMatch(match);
  }

  OnProviderUpdate(true, nullptr);

  // If we're not done, we might attempt to redisplay the deleted match. Make
  // sure we aren't displaying it by removing any old entries.
  ExpireCopiedEntries();
}

void AutocompleteController::DeleteMatchElement(const AutocompleteMatch& match,
                                                size_t element_index) {
  DCHECK(match.SupportsDeletion());

  if (match.deletable) {
    RecordMatchDeletion(match);
    match.provider->DeleteMatchElement(match, element_index);
  }

  OnProviderUpdate(true, nullptr);
}

void AutocompleteController::ExpireCopiedEntries() {
  // The first true makes UpdateResult() clear out the results and
  // regenerate them, thus ensuring that no results from the previous
  // result set remain.
  UpdateResult(true, false);
}

void AutocompleteController::OnProviderUpdate(
    bool updated_matches,
    const AutocompleteProvider* provider) {
  // Should be called even if `in_start_` is true in order to include early
  // exited async providers. If the provider is done, will log how long the
  // provider took.
  if (provider)
    metrics_.OnProviderUpdate(*provider);

  // Providers should only call this method during the asynchronous pass.
  // There's no reason to call this during the synchronous pass, since we
  // perform these operations anyways after all providers are started.
  //
  // This is not a DCHECK, because in the unusual case that a provider calls an
  // asynchronous method, and that method early exits by calling the callback
  // immediately, it's not necessarily a programmer error. We should just no-op.
  if (in_start_)
    return;

  CheckIfDone();
  // Multiple providers may provide synchronous results, so we only update the
  // results if we're not in Start().
  if (updated_matches || done_)
    UpdateResult(false, false);
}

void AutocompleteController::AddProviderAndTriggeringLogs(
    OmniboxLog* logs) const {
  logs->providers_info.clear();
  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;

    // Add per-provider info, if any.
    provider->AddProviderInfo(&logs->providers_info);

    // This is also a good place to put code to add info that you want to
    // add for every provider.
  }

  // OmniboxPedalProvider is not a "true" AutocompleteProvider and isn't
  // included in the list of providers, though needs to report information for
  // its field trial.  Manually call AddProviderInfo for pedals.
  if (provider_client_->GetPedalProvider()) {
    provider_client_->GetPedalProvider()->AddProviderInfo(
        &logs->providers_info);
  }

  // Add any features that have been triggered.
  provider_client_->GetOmniboxTriggeredFeatureService()->RecordToLogs(
      &logs->feature_triggered_in_session);
}

void AutocompleteController::ResetSession() {
  search_service_worker_signal_sent_ = false;

  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;
    provider->ResetSession();
  }

  // OmniboxPedalProvider is not included in the list of providers as it's not
  // a "true" AutocompleteProvider.  Manually call ResetSession() for pedals.
  if (provider_client_->GetPedalProvider()) {
    provider_client_->GetPedalProvider()->ResetSession();
  }

  provider_client_->GetOmniboxTriggeredFeatureService()->ResetSession();
}

void AutocompleteController::
    UpdateMatchDestinationURLWithAdditionalAssistedQueryStats(
        base::TimeDelta query_formulation_time,
        AutocompleteMatch* match) const {
  // We expect the assisted_query_stats and the searchbox_stats to have been
  // previously set when this method is called. If that is not the case, this
  // method is being called by mistake and assisted_query_stats and the
  // searchbox_stats should not be updated with additional information.
  if (!match->search_terms_args ||
      match->search_terms_args->assisted_query_stats.empty() ||
      match->search_terms_args->searchbox_stats.ByteSizeLong() == 0) {
    return;
  }

  // Append the query formulation time (time from when the user first typed a
  // character into the omnibox to when the user selected a query), whether
  // a field trial has triggered, and the current page classification to the AQS
  // parameter.
  const std::string experiment_stats = base::StringPrintf(
      "%" PRId64 "j%dj%d", query_formulation_time.InMilliseconds(),
      (search_provider_ &&
       search_provider_->field_trial_triggered_in_session()) ||
          (zero_suggest_provider_ &&
           zero_suggest_provider_->field_trial_triggered_in_session()),
      input_.current_page_classification());
  match->search_terms_args->assisted_query_stats += "." + experiment_stats;
  // TODO(crbug.com/1247846): experiment_stats is a deprecated field. We should
  // however continue to report it for parity with what gets reported in aqs=,
  // and for the downstream consumers that expect this field. Once gs_lcrp=
  // fully replaces aqs=, Chrome should start logging the substitute fields and
  // the downstream consumers should migrate to using those fields before we
  // can stop logging this deprecated field.
  match->search_terms_args->searchbox_stats.set_experiment_stats(
      experiment_stats);

  // Append the ExperimentStatsV2 to the AQS parameter to be logged in
  // searchbox_stats.proto's experiment_stats_v2 field.
  if (zero_suggest_provider_) {
    std::vector<std::string> experiment_stats_v2_strings;
    for (const auto& experiment_stat_v2 :
         zero_suggest_provider_->experiment_stats_v2s()) {
      // The string value consists of suggestion type/subtype pairs delimited
      // with colons. However, the SearchboxStats logging flow expects
      // suggestion type/subtype pairs to be delimited with commas instead.
      std::string value = experiment_stat_v2.string_value();
      std::replace(value.begin(), value.end(), ':', ',');
      // The SearchboxStats logging flow expects experiment stats type and value
      // to be delimited with 'i'.
      experiment_stats_v2_strings.push_back(
          base::NumberToString(experiment_stat_v2.type_int()) + "i" + value);
      auto* reported_experiment_stats_v2 =
          match->search_terms_args->searchbox_stats.add_experiment_stats_v2();
      reported_experiment_stats_v2->set_type_int(experiment_stat_v2.type_int());
      reported_experiment_stats_v2->set_string_value(value);
    }
    if (!experiment_stats_v2_strings.empty()) {
      // 'j' is used as a delimiter between individual experiment stat entries.
      match->search_terms_args->assisted_query_stats +=
          "." + base::JoinString(experiment_stats_v2_strings, "j");
    }
  }

  SetMatchDestinationURL(match);
}

void AutocompleteController::SetMatchDestinationURL(
    AutocompleteMatch* match) const {
  const TemplateURL* template_url =
      match->GetTemplateURL(template_url_service_, false);
  if (!template_url)
    return;

  match->destination_url = GURL(template_url->url_ref().ReplaceSearchTerms(
      *match->search_terms_args, template_url_service_->search_terms_data()));
#if BUILDFLAG(IS_ANDROID)
  match->UpdateJavaDestinationUrl();
#endif
}

void AutocompleteController::SetTailSuggestContentPrefixes() {
  result_.SetTailSuggestContentPrefixes();
}

void AutocompleteController::SetTailSuggestCommonPrefixes() {
  result_.SetTailSuggestCommonPrefixes();
}

void AutocompleteController::UpdateResult(
    bool regenerate_result,
    bool force_notify_default_match_changed) {
  TRACE_EVENT0("omnibox", "AutocompleteController::UpdateResult");

  absl::optional<AutocompleteMatch> last_default_match;
  std::u16string last_default_associated_keyword;
  if (result_.default_match()) {
    last_default_match = *result_.default_match();
    if (last_default_match->associated_keyword) {
      last_default_associated_keyword =
          last_default_match->associated_keyword->keyword;
    }
  }

  const auto last_result_for_logging = result_.GetMatchDedupComparators();

  if (regenerate_result)
    result_.Reset();

  AutocompleteResult old_matches_to_reuse;
  old_matches_to_reuse.Swap(&result_);

  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;
    result_.AppendMatches(provider->matches());
    result_.MergeSuggestionGroupsMap(provider->suggestion_groups_map());
  }

  bool perform_tab_match = true;
#if BUILDFLAG(IS_ANDROID)
  // Do not look for matching tabs on Android unless we collected all the
  // suggestions. Tab matching is an expensive process with multiple JNI calls
  // involved. Run it only when all the suggestions are collected.
  perform_tab_match &= done_;
#endif

  if (perform_tab_match)
    result_.ConvertOpenTabMatches(provider_client_.get(), &input_);

  // Sort the matches and trim to a small number of "best" matches. Do this only
  // if preservation is enabled for sync/async updates, and this is a sync/async
  // update.
  const AutocompleteMatch* preserve_default_match = nullptr;
  if ((in_start_
           ? OmniboxFieldTrial::
                 kAutocompleteStabilityPreserveDefaultForSyncUpdates.Get()
           : OmniboxFieldTrial::
                 kAutocompleteStabilityPreserveDefaultForAsyncUpdates.Get()) &&
      last_default_match) {
    preserve_default_match = &last_default_match.value();
  }
  result_.SortAndCull(input_, template_url_service_, preserve_default_match);

  // Only produce Pedals for the default focus case (not on focus or on delete).
  if (input_.focus_type() == OmniboxFocusType::DEFAULT) {
    // TODO(tommycli): It sure seems like this should be moved down below
    // `TransferOldMatches()` along with all the other annotation code.
    result_.AttachPedalsToMatches(input_, *provider_client_);
  }

  // Need to validate before invoking CopyOldMatches as the old matches are not
  // valid against the current input.
#if DCHECK_IS_ON()
  result_.Validate();
#endif  // DCHECK_IS_ON()

  if (!done_) {
    // This conditional needs to match the conditional in Start that invokes
    // StartExpireTimer.
    result_.TransferOldMatches(input_, &old_matches_to_reuse);
    if (OmniboxFieldTrial::kAutocompleteStabilityPreserveDefaultAfterTransfer
            .Get()) {
      result_.SortAndCull(input_, template_url_service_,
                          preserve_default_match);
    } else {
      result_.SortAndCull(input_, template_url_service_);
    }
  }

  // Will log metrics for how many matches changed. Will also log timing metrics
  // for the current request if it's complete; otherwise, will just update
  // timestamps of when the last update changing any or the default suggestion
  // occurred.
  metrics_.OnUpdateResult(last_result_for_logging,
                          result_.GetMatchDedupComparators());

  // Below are all annotations after the match list is ready.
#if !BUILDFLAG(IS_IOS)
  // HistoryClusters is not enabled on iOS.
  AttachHistoryClustersActions(provider_client_->GetHistoryClustersService(),
                               provider_client_->GetPrefs(), result_);
#endif
  UpdateKeywordDescriptions(&result_);
  UpdateAssociatedKeywords(&result_);
  UpdateAssistedQueryStats(&result_);
  if (search_provider_)
    search_provider_->RegisterDisplayedAnswers(result_);

  const bool default_is_valid = result_.default_match();
  std::u16string default_associated_keyword;
  if (default_is_valid && result_.default_match()->associated_keyword) {
    default_associated_keyword =
        result_.default_match()->associated_keyword->keyword;
  }
  // We've gotten async results. Send notification that the default match
  // updated if fill_into_edit, associated_keyword, or keyword differ.  (The
  // second can change if we've just started Chrome and the keyword database
  // finishes loading while processing this request.  The third can change
  // if we swapped from interpreting the input as a search--which gets
  // labeled with the default search provider's keyword--to a URL.)
  // We don't check the URL as that may change for the default match
  // even though the fill into edit hasn't changed (see SearchProvider
  // for one case of this).
  const bool notify_default_match =
      (last_default_match.has_value() != default_is_valid) ||
      (last_default_match &&
       ((result_.default_match()->fill_into_edit !=
         last_default_match->fill_into_edit) ||
        (default_associated_keyword != last_default_associated_keyword) ||
        (result_.default_match()->keyword != last_default_match->keyword)));
  if (notify_default_match)
    last_time_default_match_changed_ = base::TimeTicks::Now();

  // Mark the rich autocompletion feature triggered if the top match, or
  // would-be-top-match if rich autocompletion is counterfactual enabled, is
  // rich autocompleted.
  const auto top_match_rich_autocompletion_type =
      TopMatchRichAutocompletionType(result_);
  provider_client_->GetOmniboxTriggeredFeatureService()
      ->RichAutocompletionTypeTriggered(top_match_rich_autocompletion_type);
  if (top_match_rich_autocompletion_type !=
      AutocompleteMatch::RichAutocompletionType::kNone) {
    provider_client_->GetOmniboxTriggeredFeatureService()->FeatureTriggered(
        OmniboxTriggeredFeatureService::Feature::kRichAutocompletion);
  }

  NotifyChanged(force_notify_default_match_changed || notify_default_match);
}

void AutocompleteController::UpdateAssociatedKeywords(
    AutocompleteResult* result) {
  if (!keyword_provider_)
    return;

  // Determine if the user's input is an exact keyword match.
  std::u16string exact_keyword =
      keyword_provider_->GetKeywordForText(input_.text());

  std::set<std::u16string> keywords;
  for (auto& match : *result) {
    std::u16string keyword(
        match.GetSubstitutingExplicitlyInvokedKeyword(template_url_service_));
    if (!keyword.empty()) {
      keywords.insert(keyword);
      continue;
    }

    // When the user has typed an exact keyword, we want tab-to-search on the
    // default match to select that keyword, even if the match
    // inline-autocompletes to a different keyword.  (This prevents inline
    // autocompletions from blocking a user's attempts to use an explicitly-set
    // keyword of their own creation.)  So use |exact_keyword| if it's
    // available.
    if (!exact_keyword.empty() && !keywords.count(exact_keyword)) {
      keywords.insert(exact_keyword);
      // If the match has an answer, it will look strange to try to display
      // it along with a keyword hint. Prefer the keyword hint, and revert
      // to a typical search.
      match.answer.reset();
      match.associated_keyword = std::make_unique<AutocompleteMatch>(
          keyword_provider_->CreateVerbatimMatch(exact_keyword, exact_keyword,
                                                 input_));
#if BUILDFLAG(IS_ANDROID)
      match.UpdateJavaAnswer();
#endif
      continue;
    }

    // Otherwise, set a match's associated keyword based on the match's
    // fill_into_edit, which should take inline autocompletions into account.
    keyword = keyword_provider_->GetKeywordForText(match.fill_into_edit);

    // Only add the keyword if the match does not have a duplicate keyword with
    // a more relevant match.
    if (!keyword.empty() && !keywords.count(keyword)) {
      keywords.insert(keyword);
      match.associated_keyword = std::make_unique<AutocompleteMatch>(
          keyword_provider_->CreateVerbatimMatch(match.fill_into_edit, keyword,
                                                 input_));
    } else {
      match.associated_keyword.reset();
    }
  }
}

void AutocompleteController::UpdateKeywordDescriptions(
    AutocompleteResult* result) {
  std::u16string last_keyword;
  for (auto i(result->begin()); i != result->end(); ++i) {
    if (AutocompleteMatch::IsSearchType(i->type)) {
      if (AutocompleteMatchHasCustomDescription(*i))
        continue;
      i->description.clear();
      i->description_class.clear();
      DCHECK(!i->keyword.empty());
      if (i->keyword != last_keyword &&
          !ShouldCurbKeywordDescriptions(i->keyword)) {
        const TemplateURL* template_url =
            i->GetTemplateURL(template_url_service_, false);
        if (template_url) {
          if (OmniboxFieldTrial::IsSiteSearchStarterPackEnabled() &&
              template_url->starter_pack_id() > 0) {
            i->description =
                TemplateURLStarterPackData::GetDestinationUrlForStarterPackID(
                    template_url->starter_pack_id());
          } else {
            // For extension keywords, just make the description the extension
            // name -- don't assume that the normal search keyword description
            // is applicable.
            i->description =
                template_url->AdjustedShortNameForLocaleDirection();
            if (template_url->type() != TemplateURL::OMNIBOX_API_EXTENSION) {
              i->description = l10n_util::GetStringFUTF16(
                  IDS_AUTOCOMPLETE_SEARCH_DESCRIPTION, i->description);
            }
          }
          i->description_class.push_back(
              ACMatchClassification(0, ACMatchClassification::DIM));
        }
#if BUILDFLAG(IS_ANDROID)
        i->UpdateJavaDescription();
#endif

        last_keyword = i->keyword;
      }
    } else {
      last_keyword.clear();
    }
  }
}

void AutocompleteController::UpdateAssistedQueryStats(
    AutocompleteResult* result) {
  if (result->empty())
    return;

  metrics::ChromeSearchboxStats searchbox_stats;
  searchbox_stats.set_client_name("chrome");

  // Build the impressions string (the AQS part after ".").
  std::string autocompletions;
  int count = 0;
  int num_zero_prefix_suggestions_shown = 0;
  size_t last_type = std::u16string::npos;
  base::flat_set<int> last_subtypes = {};
  for (size_t index = 0; index < result->size(); ++index) {
    AutocompleteMatch* match = result->match_at(index);
    auto subtypes = match->subtypes;
    size_t type = std::u16string::npos;
    GetMatchTypeAndExtendSubtypes(*match, &type, &subtypes);

    // Count any suggestions that constitute zero-prefix suggestions.
    if (subtypes.contains(/*SUBTYPE_ZERO_PREFIX_LOCAL_HISTORY*/ 450) ||
        subtypes.contains(
            /*SUBTYPE_ZERO_PREFIX_LOCAL_FREQUENT_URLS*/ 451) ||
        subtypes.contains(/*SUBTYPE_ZERO_PREFIX*/ 362)) {
      num_zero_prefix_suggestions_shown++;
    }

    auto* available_suggestion = searchbox_stats.add_available_suggestions();
    available_suggestion->set_index(index);
    available_suggestion->set_type(type);
    for (const auto subtype : subtypes) {
      available_suggestion->add_subtypes(subtype);
    }

    if (last_type != std::u16string::npos &&
        (type != last_type || subtypes != last_subtypes)) {
      AppendAvailableAutocompletion(last_type, last_subtypes, count,
                                    &autocompletions);
      count = 1;
    } else {
      count++;
    }
    last_type = type;
    last_subtypes = subtypes;
  }
  AppendAvailableAutocompletion(last_type, last_subtypes, count,
                                &autocompletions);

  // TODO(crbug.com/1307142): These two fields should take into account all the
  // zero-prefix suggestions shown during the session and not only the ones
  // shown at the time of user making a selection.
  searchbox_stats.set_num_zero_prefix_suggestions_shown(
      num_zero_prefix_suggestions_shown);
  searchbox_stats.set_zero_prefix_enabled(num_zero_prefix_suggestions_shown >
                                          0);

  // Go over all matches and set AQS if the match supports it.
  for (size_t index = 0; index < result->size(); ++index) {
    AutocompleteMatch* match = result->match_at(index);
    const TemplateURL* template_url =
        match->GetTemplateURL(template_url_service_, false);
    if (!template_url || !match->search_terms_args)
      continue;

    match->search_terms_args->searchbox_stats = searchbox_stats;

    std::string selected_index;
    // Prevent trivial suggestions from getting credit for being selected.
    if (!match->IsTrivialAutocompletion()) {
      DCHECK_LT(static_cast<int>(index),
                match->search_terms_args->searchbox_stats
                    .available_suggestions_size());
      const auto& selected_suggestion =
          match->search_terms_args->searchbox_stats.available_suggestions(
              index);
      DCHECK_EQ(static_cast<int>(index), selected_suggestion.index());
      match->search_terms_args->searchbox_stats.mutable_assisted_query_info()
          ->MergeFrom(selected_suggestion);

      selected_index = base::StringPrintf("%" PRIuS, index);
    }
    match->search_terms_args->assisted_query_stats = base::StringPrintf(
        "chrome.%s.%s", selected_index.c_str(), autocompletions.c_str());
  }
}

void AutocompleteController::NotifyChanged(bool notify_default_match) {
  for (Observer& obs : observers_)
    obs.OnResultChanged(this, notify_default_match);
}

void AutocompleteController::CheckIfDone() {
  bool all_providers_done = true;
  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;

    if (!provider->done()) {
      all_providers_done = false;
      break;
    }
  }
  // If asynchronous matches have been disallowed, all providers should be done.
  DCHECK(!input_.omit_asynchronous_matches() || all_providers_done);
  done_ = all_providers_done;
}

void AutocompleteController::StartExpireTimer() {
  // Amount of time (in ms) between when the user stops typing and
  // when we remove any copied entries. We do this from the time the
  // user stopped typing as some providers (such as SearchProvider)
  // wait for the user to stop typing before they initiate a query.
  const int kExpireTimeMS = 500;

  if (result_.HasCopiedMatches())
    expire_timer_.Start(FROM_HERE, base::Milliseconds(kExpireTimeMS), this,
                        &AutocompleteController::ExpireCopiedEntries);
}

void AutocompleteController::StartStopTimer() {
  stop_timer_.Start(FROM_HERE, stop_timer_duration_,
                    base::BindOnce(&AutocompleteController::StopHelper,
                                   base::Unretained(this), false, true));
}

void AutocompleteController::StopHelper(bool clear_result,
                                        bool due_to_user_inactivity) {
  // Must be called before `expire_timer_.Stop()`, modifying `done_`, or
  // modifying `AutocompleteProvider::done_` below. If the current request has
  // not completed, and therefore has not been logged yet, will log it now.
  // Likewise, if the providers have not completed, and therefore have not been
  // logged yet, will log them now.
  metrics_.OnStop();

  for (const auto& provider : providers_) {
    if (!ShouldRunProvider(provider.get()))
      continue;
    provider->Stop(clear_result, due_to_user_inactivity);
  }

  expire_timer_.Stop();
  stop_timer_.Stop();
  done_ = true;
  if (clear_result && !result_.empty()) {
    result_.Reset();
    // NOTE: We pass in false since we're trying to only clear the popup, not
    // touch the edit... this is all a mess and should be cleaned up :(
    NotifyChanged(false);
  }
}

bool AutocompleteController::ShouldCurbKeywordDescriptions(
    const std::u16string& keyword) {
  return AutocompleteProvider::InExplicitExperimentalKeywordMode(input_,
                                                                 keyword);
}

bool AutocompleteController::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* process_memory_dump) {
  size_t res = 0;

  // provider_client_ seems to be small enough to ignore it.

  // TODO(dyaroshev): implement memory estimation for scoped_refptr in
  // base::trace_event.
  res += std::accumulate(providers_.begin(), providers_.end(), 0u,
                         [](size_t sum, const auto& provider) {
                           return sum + sizeof(AutocompleteProvider) +
                                  provider->EstimateMemoryUsage();
                         });

  res += input_.EstimateMemoryUsage();
  res += result_.EstimateMemoryUsage();

  auto* dump = process_memory_dump->CreateAllocatorDump(
      base::StringPrintf("omnibox/autocomplete_controller/0x%" PRIXPTR,
                         reinterpret_cast<uintptr_t>(this)));
  dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes, res);
  return true;
}

void AutocompleteController::SetStartStopTimerDurationForTesting(
    base::TimeDelta duration) {
  stop_timer_duration_ = duration;
}

bool AutocompleteController::ShouldRunProvider(
    AutocompleteProvider* provider) const {
  if (OmniboxFieldTrial::IsSiteSearchStarterPackEnabled() &&
      input_.keyword_mode_entry_method() !=
          metrics::OmniboxEventProto_KeywordModeEntryMethod_INVALID) {
    // We're in keyword mode. Only a subset of providers are run when we're in a
    // starter pack keyword mode. Try to grab the TemplateURL to determine if
    // we're in starter pack mode and whether this provider should be run.
    AutocompleteInput keyword_input = input_;
    const TemplateURL* keyword_turl =
        KeywordProvider::GetSubstitutingTemplateURLForInput(
            template_url_service_, &keyword_input);
    if (keyword_turl && keyword_turl->starter_pack_id() > 0) {
      switch (provider->type()) {
        // Search provider and keyword provider are still run because we would
        // lose the suggestion the keyword chip is attached to otherwise. Search
        // provider suggestions are curbed for starter pack scopes in
        // `SearchProvider::ShouldCurbDefaultSuggestions()`.
        case AutocompleteProvider::TYPE_SEARCH:
        case AutocompleteProvider::TYPE_KEYWORD:
          return true;

        // @Bookmarks starter pack scope - run only the bookmarks provider.
        case AutocompleteProvider::TYPE_BOOKMARK:
          return (keyword_turl->starter_pack_id() ==
                  TemplateURLStarterPackData::kBookmarks);

        // @History starter pack scope - run history quick and history url
        // providers.
        case AutocompleteProvider::TYPE_HISTORY_QUICK:
        case AutocompleteProvider::TYPE_HISTORY_URL:
          return (keyword_turl->starter_pack_id() ==
                  TemplateURLStarterPackData::kHistory);

        // @Tabs starter pack scope - run the open tab provider.
        case AutocompleteProvider::TYPE_OPEN_TAB:
          return (keyword_turl->starter_pack_id() ==
                  TemplateURLStarterPackData::kTabs);

        // No other providers should run when in a starter pack scope.
        default:
          return false;
      }
    }
  }

  // Open Tab Provider should only be run for @tabs starter pack mode and in the
  // CrOS launcher.  As a temporary condition, we don't run the open tab
  // provider when IsSiteSearchStarterPackEnabled() is true, even though that
  // could interfere with the launcher.
  // TODO(crbug/1287313): This needs to be updated before running live
  // experiments.
  if (provider->type() == AutocompleteProvider::TYPE_OPEN_TAB &&
      OmniboxFieldTrial::IsSiteSearchStarterPackEnabled()) {
    return false;
  }

  // Otherwise, run all providers.
  return true;
}

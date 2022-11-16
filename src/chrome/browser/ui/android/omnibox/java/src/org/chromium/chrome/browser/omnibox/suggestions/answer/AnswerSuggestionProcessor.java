// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox.suggestions.answer;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.TextUtils;

import androidx.annotation.DrawableRes;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.LocaleUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.OmniboxSuggestionType;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionUiType;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.base.SuggestionDrawableState;
import org.chromium.components.image_fetcher.ImageFetcher;
import org.chromium.components.omnibox.AnswerType;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.SuggestionAnswer;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A class that handles model and view creation for the most commonly used omnibox suggestion.
 */
public class AnswerSuggestionProcessor extends BaseSuggestionViewProcessor {
    private final Map<String, List<PropertyModel>> mPendingAnswerRequestUrls;
    private final SuggestionHost mSuggestionHost;
    private final UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private final Supplier<ImageFetcher> mImageFetcherSupplier;
    private boolean mOmniBoxAnswerColorReversal;
    private boolean mOmniBoxAnswerColorReversalFinanceOnly;

    /**
     * @param context An Android context.
     * @param suggestionHost A handle to the object using the suggestions.
     */
    public AnswerSuggestionProcessor(Context context, SuggestionHost suggestionHost,
            UrlBarEditingTextStateProvider editingTextProvider,
            Supplier<ImageFetcher> imageFetcherSupplier) {
        super(context, suggestionHost, null);
        mSuggestionHost = suggestionHost;
        mPendingAnswerRequestUrls = new HashMap<>();
        mUrlBarEditingTextProvider = editingTextProvider;
        mImageFetcherSupplier = imageFetcherSupplier;
    }

    /**
     * Evaluates whether the current locale uses "green" or "red" color to indicate
     * growth, allowing locale-adjusted representation of stock market changes.
     */
    @Override
    public void onNativeInitialized() {
        mOmniBoxAnswerColorReversal =
                ChromeFeatureList.isEnabled(ChromeFeatureList.SUGGESTION_ANSWERS_COLOR_REVERSE);

        mOmniBoxAnswerColorReversalFinanceOnly =
                ChromeFeatureList.getFieldTrialParamByFeatureAsBoolean(
                        ChromeFeatureList.SUGGESTION_ANSWERS_COLOR_REVERSE,
                        "omnibox_answer_color_reversal_finance_only",
                        /* default= */ true);

        String stockColorReversalCountryList = ChromeFeatureList.getFieldTrialParamByFeature(
                ChromeFeatureList.SUGGESTION_ANSWERS_COLOR_REVERSE,
                "omnibox_answer_color_reversal_countries");
        if (!TextUtils.isEmpty(stockColorReversalCountryList)) {
            List<String> financeTickerColorReversalCountryList =
                    Arrays.asList(stockColorReversalCountryList.split(","));
            String localeString = LocaleUtils.getDefaultLocaleString();
            mOmniBoxAnswerColorReversalFinanceOnly &=
                    financeTickerColorReversalCountryList.contains(localeString);
        }
    }

    @Override
    public boolean doesProcessSuggestion(AutocompleteMatch suggestion, int position) {
        // Calculation answers are specific in a way that these are basic suggestions, but processed
        // as answers, when new answer layout is enabled.
        return suggestion.hasAnswer() || suggestion.getType() == OmniboxSuggestionType.CALCULATOR;
    }

    @Override
    public int getViewTypeId() {
        return OmniboxSuggestionUiType.ANSWER_SUGGESTION;
    }

    @Override
    public PropertyModel createModel() {
        return new PropertyModel(AnswerSuggestionViewProperties.ALL_KEYS);
    }

    @Override
    public void populateModel(AutocompleteMatch suggestion, PropertyModel model, int position) {
        super.populateModel(suggestion, model, position);
        setStateForSuggestion(model, suggestion, position);
    }

    private void maybeFetchAnswerIcon(PropertyModel model, AutocompleteMatch suggestion) {
        ThreadUtils.assertOnUiThread();

        // Ensure an image fetcher is available prior to requesting images.
        ImageFetcher imageFetcher = mImageFetcherSupplier.get();
        if (imageFetcher == null) return;

        // Note: we also handle calculations here, which do not have answer defined.
        if (!suggestion.hasAnswer()) return;
        final String url = suggestion.getAnswer().getSecondLine().getImage();
        if (url == null) return;

        // Do not make duplicate answer image requests for the same URL (to avoid generating
        // duplicate bitmaps for the same image).
        if (mPendingAnswerRequestUrls.containsKey(url)) {
            mPendingAnswerRequestUrls.get(url).add(model);
            return;
        }

        List<PropertyModel> models = new ArrayList<>();
        models.add(model);
        mPendingAnswerRequestUrls.put(url, models);
        ImageFetcher.Params params =
                ImageFetcher.Params.create(url, ImageFetcher.ANSWER_SUGGESTIONS_UMA_CLIENT_NAME);
        imageFetcher.fetchImage(
                params, (Bitmap bitmap) -> {
                    ThreadUtils.assertOnUiThread();
                    // Remove models for the URL ahead of all the checks to ensure we
                    // do not keep them around waiting in case image fetch failed.
                    List<PropertyModel> currentModels = mPendingAnswerRequestUrls.remove(url);
                    if (currentModels == null || bitmap == null) return;

                    for (int i = 0; i < currentModels.size(); i++) {
                        PropertyModel currentModel = currentModels.get(i);
                        setSuggestionDrawableState(currentModel,
                                SuggestionDrawableState.Builder.forBitmap(getContext(), bitmap)
                                        .setLarge(true)
                                        .build());
                    }
                });
    }

    /**
     * Sets both lines of the Omnibox suggestion based on an Answers in Suggest result.
     */
    private void setStateForSuggestion(
            PropertyModel model, AutocompleteMatch suggestion, int position) {
        @AnswerType
        int answerType = suggestion.getAnswer() == null ? AnswerType.INVALID
                                                        : suggestion.getAnswer().getType();
        boolean suggestionTextColorReversal = checkColorReversalRequired(
                answerType, mOmniBoxAnswerColorReversal, mOmniBoxAnswerColorReversalFinanceOnly);
        AnswerText[] details = AnswerTextNewLayout.from(getContext(), suggestion,
                mUrlBarEditingTextProvider.getTextWithoutAutocomplete(),
                suggestionTextColorReversal);

        model.set(AnswerSuggestionViewProperties.TEXT_LINE_1_TEXT, details[0].mText);
        model.set(AnswerSuggestionViewProperties.TEXT_LINE_2_TEXT, details[1].mText);

        model.set(AnswerSuggestionViewProperties.TEXT_LINE_1_ACCESSIBILITY_DESCRIPTION,
                details[0].mAccessibilityDescription);
        model.set(AnswerSuggestionViewProperties.TEXT_LINE_2_ACCESSIBILITY_DESCRIPTION,
                details[1].mAccessibilityDescription);

        model.set(AnswerSuggestionViewProperties.TEXT_LINE_1_MAX_LINES, details[0].mMaxLines);
        model.set(AnswerSuggestionViewProperties.TEXT_LINE_2_MAX_LINES, details[1].mMaxLines);

        setSuggestionDrawableState(model,
                SuggestionDrawableState.Builder
                        .forDrawableRes(getContext(), getSuggestionIcon(suggestion))
                        .setLarge(true)
                        .build());

        setTabSwitchOrRefineAction(model, suggestion, position);
        maybeFetchAnswerIcon(model, suggestion);
    }

    /**
     * Checks if we need to apply red-green color reversion on the answer suggestion.
     *
     * @param suggestion the AutocompleteMatch type for the answer
     * @param omniBoxAnswerColorReversal flag to indicate whether we need color reversal for all
     *         types of suggestion answer
     * @param omniBoxAnswerColorReversalFinanceOnly flag to indicate whether we need color reversal
     *         for finance answer
     *
     */
    @VisibleForTesting
    public static boolean checkColorReversalRequired(@AnswerType int answerType,
            boolean omniBoxAnswerColorReversal, boolean omniBoxAnswerColorReversalFinanceOnly) {
        boolean isFinanceAnswer = answerType == AnswerType.FINANCE;
        return (omniBoxAnswerColorReversal && !omniBoxAnswerColorReversalFinanceOnly)
                || (omniBoxAnswerColorReversal && omniBoxAnswerColorReversalFinanceOnly
                        && isFinanceAnswer);
    }

    /**
     * Get default suggestion icon for supplied suggestion.
     */
    @DrawableRes
    int getSuggestionIcon(AutocompleteMatch suggestion) {
        SuggestionAnswer answer = suggestion.getAnswer();
        if (answer != null) {
            switch (answer.getType()) {
                case AnswerType.DICTIONARY:
                    return R.drawable.ic_book_round;
                case AnswerType.FINANCE:
                    return R.drawable.ic_swap_vert_round;
                case AnswerType.KNOWLEDGE_GRAPH:
                    return R.drawable.ic_google_round;
                case AnswerType.SUNRISE:
                    return R.drawable.ic_wb_sunny_round;
                case AnswerType.TRANSLATION:
                    return R.drawable.logo_translate_round;
                case AnswerType.WEATHER:
                    return R.drawable.logo_partly_cloudy;
                case AnswerType.WHEN_IS:
                    return R.drawable.ic_event_round;
                case AnswerType.CURRENCY:
                    return R.drawable.ic_loop_round;
                case AnswerType.SPORTS:
                    return R.drawable.ic_google_round;
                default:
                    break;
            }
        } else if (suggestion.getType() == OmniboxSuggestionType.CALCULATOR) {
            return R.drawable.ic_equals_sign_round;
        }
        return R.drawable.ic_google_round;
    }
}

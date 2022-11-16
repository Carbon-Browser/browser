// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox.suggestions.mostvisited;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;

import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.OmniboxSuggestionType;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.suggestions.FaviconFetcher;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionUiType;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionCommonProperties;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionsMetrics;
import org.chromium.chrome.browser.omnibox.suggestions.carousel.BaseCarouselSuggestionItemViewBuilder;
import org.chromium.chrome.browser.omnibox.suggestions.carousel.BaseCarouselSuggestionProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.carousel.BaseCarouselSuggestionViewProperties;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.components.browser_ui.widget.tile.TileViewProperties;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.AutocompleteMatch.SuggestTile;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

/**
 * SuggestionProcessor for Most Visited URL tiles.
 */
public class MostVisitedTilesProcessor extends BaseCarouselSuggestionProcessor {
    private final @NonNull Context mContext;
    private final @NonNull SuggestionHost mSuggestionHost;
    private final @NonNull FaviconFetcher mFaviconFetcher;
    private final int mMinCarouselItemViewHeight;
    private boolean mShouldWrapTitleText;
    private boolean mEnableOrganicRepeatableQueries;

    /**
     * Constructor.
     *
     * @param context An Android context.
     * @param host SuggestionHost receiving notifications about user actions.
     * @param faviconFetcher Class retrieving favicons for the MV Tiles.
     */
    public MostVisitedTilesProcessor(@NonNull Context context, @NonNull SuggestionHost host,
            @NonNull FaviconFetcher faviconFetcher) {
        super(context);
        mContext = context;
        mSuggestionHost = host;
        mFaviconFetcher = faviconFetcher;
        mMinCarouselItemViewHeight =
                mContext.getResources().getDimensionPixelSize(R.dimen.tile_view_min_height);
    }

    @Override
    public boolean doesProcessSuggestion(AutocompleteMatch suggestion, int matchIndex) {
        return suggestion.getType() == OmniboxSuggestionType.TILE_NAVSUGGEST;
    }

    @Override
    public int getViewTypeId() {
        return OmniboxSuggestionUiType.TILE_NAVSUGGEST;
    }

    @Override
    public PropertyModel createModel() {
        return new PropertyModel(BaseCarouselSuggestionViewProperties.ALL_KEYS);
    }

    @Override
    public int getMinimumCarouselItemViewHeight() {
        return mMinCarouselItemViewHeight;
    }

    @Override
    public void onNativeInitialized() {
        mShouldWrapTitleText = ChromeFeatureList.isEnabled(
                ChromeFeatureList.OMNIBOX_MOST_VISITED_TILES_TITLE_WRAP_AROUND);
        mEnableOrganicRepeatableQueries =
                ChromeFeatureList.isEnabled(ChromeFeatureList.HISTORY_ORGANIC_REPEATABLE_QUERIES);
    }

    @Override
    public void populateModel(AutocompleteMatch suggestion, PropertyModel model, int matchIndex) {
        final List<AutocompleteMatch.SuggestTile> tiles = suggestion.getSuggestTiles();
        final int tilesCount = tiles.size();
        final List<ListItem> tileList = new ArrayList<>(tilesCount);

        for (int elementIndex = 0; elementIndex < tilesCount; elementIndex++) {
            final PropertyModel tileModel = new PropertyModel(TileViewProperties.ALL_KEYS);
            final SuggestTile tile = tiles.get(elementIndex);
            final String title = tile.title;
            final GURL url = tile.url;
            final boolean isSearch = tile.isSearch && mEnableOrganicRepeatableQueries;
            final int itemIndex = elementIndex;

            tileModel.set(TileViewProperties.TITLE, title);
            tileModel.set(TileViewProperties.TITLE_LINES, mShouldWrapTitleText ? 2 : 1);
            tileModel.set(TileViewProperties.ON_FOCUS_VIA_SELECTION,
                    () -> mSuggestionHost.setOmniboxEditingText(url.getSpec()));
            tileModel.set(TileViewProperties.ON_CLICK, v -> {
                SuggestionsMetrics.recordSuggestTileTypeUsed(itemIndex, isSearch);
                mSuggestionHost.onSuggestionClicked(suggestion, matchIndex, url);
            });

            final int elementIndexForDeletion = elementIndex;
            tileModel.set(TileViewProperties.ON_LONG_CLICK, v -> {
                mSuggestionHost.onDeleteMatchElement(
                        suggestion, title, matchIndex, elementIndexForDeletion);
                return true;
            });

            if (isSearch) {
                // Note: we should never show most visited tiles in incognito mode. Catch this early
                // if we ever do.
                assert model.get(SuggestionCommonProperties.COLOR_SCHEME)
                        != BrandedColorScheme.INCOGNITO;
                tileModel.set(TileViewProperties.ICON_TINT,
                        ChromeColors.getSecondaryIconTint(mContext, /* isIncognito= */ false));
                tileModel.set(TileViewProperties.ICON,
                        AppCompatResources.getDrawable(
                                mContext, R.drawable.ic_suggestion_magnifier));
                tileModel.set(TileViewProperties.CONTENT_DESCRIPTION,
                        mContext.getString(
                                R.string.accessibility_omnibox_most_visited_tile_search, title));
            } else {
                tileModel.set(TileViewProperties.ICON_TINT, null);
                tileModel.set(TileViewProperties.CONTENT_DESCRIPTION,
                        mContext.getString(
                                R.string.accessibility_omnibox_most_visited_tile_navigate, title,
                                url.getHost()));

                tileModel.set(TileViewProperties.SMALL_ICON_ROUNDING_RADIUS,
                        mContext.getResources().getDimensionPixelSize(
                                R.dimen.omnibox_carousel_icon_rounding_radius));
                mFaviconFetcher.fetchFaviconWithBackoff(url, true, (icon, type) -> {
                    tileModel.set(TileViewProperties.ICON, new BitmapDrawable(icon));
                });
            }

            tileList.add(new ListItem(
                    BaseCarouselSuggestionItemViewBuilder.ViewType.TILE_VIEW, tileModel));
        }

        model.set(BaseCarouselSuggestionViewProperties.TILES, tileList);
        model.set(BaseCarouselSuggestionViewProperties.SHOW_TITLE, false);
    }
}

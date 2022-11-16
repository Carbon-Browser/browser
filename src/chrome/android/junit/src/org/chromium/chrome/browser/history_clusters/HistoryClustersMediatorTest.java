// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history_clusters;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.mockito.AdditionalMatchers.geq;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.robolectric.Shadows.shadowOf;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.net.Uri;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatcher;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Promise;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.history_clusters.HistoryCluster.MatchPosition;
import org.chromium.chrome.browser.history_clusters.HistoryClusterView.ClusterViewAccessibilityState;
import org.chromium.chrome.browser.history_clusters.HistoryClustersItemProperties.ItemType;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabCreator;
import org.chromium.components.browser_ui.widget.MoreProgressButton.State;
import org.chromium.components.browser_ui.widget.selectable_list.SelectionDelegate;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.shadows.ShadowAppCompatResources;
import org.chromium.ui.util.AccessibilityUtil;
import org.chromium.url.GURL;
import org.chromium.url.JUnitTestGURLs;
import org.chromium.url.ShadowGURL;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** Unit tests for HistoryClustersMediator. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {ShadowAppCompatResources.class, ShadowGURL.class})
@SuppressWarnings("DoNotMock") // Mocks GURL
public class HistoryClustersMediatorTest {
    private static final String ITEM_URL_SPEC = "https://www.wombats.com/";
    private static final String INCOGNITO_EXTRA = "history_clusters.incognito";
    private static final String NEW_TAB_EXTRA = "history_clusters.new_tab";
    private static final String TAB_GROUP_EXTRA = "history_clusters.tab_group";
    private static final String ADDTIONAL_URLS_EXTRA = "history_clusters.addtional_urls";

    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock
    private HistoryClustersBridge mBridge;
    @Mock
    private Context mContext;
    @Mock
    private Resources mResources;
    @Mock
    private LargeIconBridge mLargeIconBridge;
    @Mock
    private GURL mGurl1;
    @Mock
    private GURL mGurl2;
    @Mock
    private GURL mGurl3;
    @Mock
    private Tab mTab;
    @Mock
    private Tab mTab2;
    @Mock
    private GURL mMockGurl;
    @Mock
    private HistoryClustersMediator.Clock mClock;
    @Mock
    private TemplateUrlService mTemplateUrlService;
    @Mock
    private RecyclerView mRecyclerView;
    @Mock
    private LinearLayoutManager mLayoutManager;
    @Mock
    private TabCreator mTabCreator;
    @Mock
    private HistoryClustersMetricsLogger mMetricsLogger;
    @Mock
    private AccessibilityUtil mAccessibilityUtil;
    @Mock
    private Configuration mConfiguration;

    private ClusterVisit mVisit1;
    private ClusterVisit mVisit2;
    private ClusterVisit mVisit3;
    private ClusterVisit mVisit4;
    private HistoryCluster mCluster1;
    private HistoryCluster mCluster2;
    private HistoryCluster mCluster3;
    private HistoryClustersResult mHistoryClustersResultWithQuery;
    private HistoryClustersResult mHistoryClustersFollowupResultWithQuery;
    private HistoryClustersResult mHistoryClustersResultEmptyQuery;
    private ModelList mModelList;
    private PropertyModel mToolbarModel;
    private Intent mIntent = new Intent();
    private HistoryClustersMediator mMediator;
    private boolean mIsSeparateActivity;
    private HistoryClustersDelegate mHistoryClustersDelegate;
    private SelectionDelegate mSelectionDelegate = new SelectionDelegate();
    private final ObservableSupplierImpl<Boolean> mShouldShowPrivacyDisclaimerSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<Boolean> mShouldShowClearBrowsingDataSupplier =
            new ObservableSupplierImpl<>();
    private List<ClusterVisit> mVisitsForRemoval = new ArrayList<>();

    @Before
    public void setUp() {
        ContextUtils.initApplicationContextForTests(mContext);
        doReturn(mResources).when(mContext).getResources();
        doReturn(ITEM_URL_SPEC).when(mMockGurl).getSpec();
        doReturn(mLayoutManager).when(mRecyclerView).getLayoutManager();
        mConfiguration.keyboard = Configuration.KEYBOARD_NOKEYS;
        doReturn(mConfiguration).when(mResources).getConfiguration();
        mModelList = new ModelList();
        mToolbarModel = new PropertyModel(HistoryClustersToolbarProperties.ALL_KEYS);

        mHistoryClustersDelegate = new HistoryClustersDelegate() {
            @Override
            public boolean isSeparateActivity() {
                return mIsSeparateActivity;
            }

            @Override
            public Tab getTab() {
                return mTab;
            }

            @Override
            public Intent getHistoryActivityIntent() {
                return mIntent;
            }

            @Override
            public <SerializableList extends List<String> & Serializable> Intent getOpenUrlIntent(
                    GURL gurl, boolean inIncognito, boolean createNewTab, boolean inTabGroup,
                    @Nullable SerializableList additionalUrls) {
                mIntent = new Intent();
                mIntent.setData(Uri.parse(gurl.getSpec()));
                mIntent.putExtra(INCOGNITO_EXTRA, inIncognito);
                mIntent.putExtra(NEW_TAB_EXTRA, createNewTab);
                mIntent.putExtra(TAB_GROUP_EXTRA, inTabGroup);
                mIntent.putExtra(ADDTIONAL_URLS_EXTRA, additionalUrls);
                return mIntent;
            }

            @Override
            public ViewGroup getToggleView(ViewGroup parent) {
                return null;
            }

            @Override
            public TabCreator getTabCreator(boolean isIncognito) {
                return mTabCreator;
            }

            @Nullable
            @Override
            public ViewGroup getPrivacyDisclaimerView(ViewGroup parent) {
                return null;
            }

            @Nullable
            @Override
            public ObservableSupplier<Boolean> shouldShowPrivacyDisclaimerSupplier() {
                return mShouldShowPrivacyDisclaimerSupplier;
            }

            @Override
            public void toggleInfoHeaderVisibility() {}

            @Nullable
            @Override
            public ViewGroup getClearBrowsingDataView(ViewGroup parent) {
                return null;
            }

            @Nullable
            @Override
            public ObservableSupplier<Boolean> shouldShowClearBrowsingDataSupplier() {
                return mShouldShowClearBrowsingDataSupplier;
            }

            @Override
            public void markVisitForRemoval(ClusterVisit clusterVisit) {
                mVisitsForRemoval.add(clusterVisit);
            }
        };

        mShouldShowPrivacyDisclaimerSupplier.set(true);
        mShouldShowClearBrowsingDataSupplier.set(true);
        doReturn("http://spec1.com").when(mGurl1).getSpec();
        doReturn("http://spec2.com").when(mGurl2).getSpec();
        doReturn("http://spec3.com").when(mGurl3).getSpec();

        mMediator = new HistoryClustersMediator(mBridge, mLargeIconBridge, mContext, mResources,
                mModelList, mToolbarModel, mHistoryClustersDelegate, mClock, mTemplateUrlService,
                mSelectionDelegate, mMetricsLogger, mAccessibilityUtil);
        mVisit1 = new ClusterVisit(1.0F, mGurl1, "Title 1", "url1.com/", new ArrayList<>(),
                new ArrayList<>(), mGurl1, 123L, new ArrayList<>());
        mVisit2 = new ClusterVisit(1.0F, mGurl2, "Title 2", "url2.com/", new ArrayList<>(),
                new ArrayList<>(), mGurl2, 123L, new ArrayList<>());
        mVisit3 = new ClusterVisit(1.0F, mGurl3, "Title 3", "url3.com/", new ArrayList<>(),
                new ArrayList<>(), mGurl3, 123L, new ArrayList<>());
        mVisit4 = new ClusterVisit(1.0F, mGurl3, "Title 4", "url3.com/foo", new ArrayList<>(),
                new ArrayList<>(), mGurl3, 123L, new ArrayList<>());
        mCluster1 = new HistoryCluster(Arrays.asList(mVisit1, mVisit2), "\"label1\"", "label1",
                new ArrayList<>(), 456L, Arrays.asList("search 1", "search 2"));
        mCluster2 = new HistoryCluster(Arrays.asList(mVisit3), "hostname.com", "hostname.com",
                new ArrayList<>(), 123L, Collections.emptyList());
        mCluster3 = new HistoryCluster(Arrays.asList(mVisit4), "\"label3\"", "label3",
                new ArrayList<>(), 789L, Collections.EMPTY_LIST);
        mHistoryClustersResultWithQuery =
                new HistoryClustersResult(Arrays.asList(mCluster1, mCluster2),
                        new LinkedHashMap<>(ImmutableMap.of("label", 1)), "query", true, false);
        mHistoryClustersFollowupResultWithQuery = new HistoryClustersResult(
                Arrays.asList(mCluster3),
                new LinkedHashMap<>(ImmutableMap.of("label", 1, "hostname.com", 1, "label3", 1)),
                "query", false, true);
        mHistoryClustersResultEmptyQuery =
                new HistoryClustersResult(Arrays.asList(mCluster1, mCluster2),
                        new LinkedHashMap<>(ImmutableMap.of("label", 1, "hostname.com", 1)), "",
                        false, false);
    }

    @Test
    public void testCreateDestroy() {
        mMediator.destroy();
    }

    @Test
    public void testQuery() {
        Promise<HistoryClustersResult> promise = new Promise<>();
        doReturn(promise).when(mBridge).queryClusters("query");

        // In production code, calling setQueryState() will end up calling startQuery via
        // onSearchTextChanged. In mediator tests we don't have view binders set up so we need to
        // call both.
        mMediator.setQueryState(QueryState.forQuery("query", ""));
        mMediator.startQuery("query");
        assertEquals(1, mModelList.size());
        ListItem spinnerItem = mModelList.get(0);
        assertEquals(spinnerItem.type, ItemType.MORE_PROGRESS);
        assertEquals(spinnerItem.model.get(HistoryClustersItemProperties.PROGRESS_BUTTON_STATE),
                State.LOADING);

        fulfillPromise(promise, mHistoryClustersResultWithQuery);

        assertEquals(6, mModelList.size());
        ListItem clusterItem = mModelList.get(0);
        assertEquals(clusterItem.type, ItemType.CLUSTER);
        PropertyModel clusterModel = clusterItem.model;
        assertTrue(clusterModel.getAllSetProperties().containsAll(ImmutableList.of(
                HistoryClustersItemProperties.ACCESSIBILITY_STATE,
                HistoryClustersItemProperties.CLICK_HANDLER, HistoryClustersItemProperties.LABEL,
                HistoryClustersItemProperties.END_BUTTON_DRAWABLE)));
        assertEquals(ClusterViewAccessibilityState.COLLAPSIBLE,
                clusterModel.get(HistoryClustersItemProperties.ACCESSIBILITY_STATE));
        assertEquals(shadowOf(clusterModel.get(HistoryClustersItemProperties.END_BUTTON_DRAWABLE))
                             .getCreatedFromResId(),
                R.drawable.ic_expand_more_black_24dp);
        verify(mMetricsLogger).incrementQueryCount();

        ListItem visitItem = mModelList.get(1);
        assertEquals(visitItem.type, ItemType.VISIT);
        PropertyModel visitModel = visitItem.model;
        assertTrue(visitModel.getAllSetProperties().containsAll(
                Arrays.asList(HistoryClustersItemProperties.CLICK_HANDLER,
                        HistoryClustersItemProperties.TITLE, HistoryClustersItemProperties.URL)));

        ListItem relatedSearchesItem = mModelList.get(3);
        assertEquals(relatedSearchesItem.type, ItemType.RELATED_SEARCHES);
        PropertyModel relatedSearchesModel = relatedSearchesItem.model;
        assertTrue(relatedSearchesModel.getAllSetProperties().containsAll(
                Arrays.asList(HistoryClustersItemProperties.CHIP_CLICK_HANDLER,
                        HistoryClustersItemProperties.RELATED_SEARCHES)));
    }

    @Test
    public void testScrollToLoadDisabled() {
        mConfiguration.keyboard = Configuration.KEYBOARD_12KEY;
        mMediator = new HistoryClustersMediator(mBridge, mLargeIconBridge, mContext, mResources,
                mModelList, mToolbarModel, mHistoryClustersDelegate, mClock, mTemplateUrlService,
                mSelectionDelegate, mMetricsLogger, mAccessibilityUtil);

        Promise<HistoryClustersResult> promise = new Promise<>();
        doReturn(promise).when(mBridge).queryClusters("query");
        Promise<HistoryClustersResult> secondPromise = new Promise();
        doReturn(secondPromise).when(mBridge).loadMoreClusters("query");

        mMediator.setQueryState(QueryState.forQuery("query", ""));
        mMediator.startQuery("query");

        assertEquals(1, mModelList.size());
        ListItem spinnerItem = mModelList.get(0);
        assertEquals(spinnerItem.type, ItemType.MORE_PROGRESS);
        assertEquals(spinnerItem.model.get(HistoryClustersItemProperties.PROGRESS_BUTTON_STATE),
                State.LOADING);

        fulfillPromise(promise, mHistoryClustersResultWithQuery);

        spinnerItem = mModelList.get(mModelList.size() - 1);
        assertEquals(spinnerItem.type, ItemType.MORE_PROGRESS);
        assertEquals(spinnerItem.model.get(HistoryClustersItemProperties.PROGRESS_BUTTON_STATE),
                State.BUTTON);

        mMediator.onScrolled(mRecyclerView, 1, 1);
        verify(mBridge, Mockito.never()).loadMoreClusters("query");

        spinnerItem.model.get(HistoryClustersItemProperties.CLICK_HANDLER).onClick(null);
        ShadowLooper.idleMainLooper();

        verify(mBridge).loadMoreClusters("query");
        spinnerItem = mModelList.get(mModelList.size() - 1);
        assertEquals(spinnerItem.type, ItemType.MORE_PROGRESS);
        assertEquals(spinnerItem.model.get(HistoryClustersItemProperties.PROGRESS_BUTTON_STATE),
                State.LOADING);

        fulfillPromise(secondPromise, mHistoryClustersFollowupResultWithQuery);
        // There should no longer be a spinner or "load more" button once all possible results for
        // the current query have been loaded.
        for (int i = 0; i < mModelList.size(); i++) {
            ListItem item = mModelList.get(i);
            assertNotEquals(item.type, ItemType.MORE_PROGRESS);
        }
    }

    @Test
    public void testEmptyQuery() {
        Promise<HistoryClustersResult> promise = new Promise<>();
        doReturn(promise).when(mBridge).queryClusters("");

        mMediator.setQueryState(QueryState.forQueryless());
        mMediator.startQuery("");
        fulfillPromise(promise, mHistoryClustersResultEmptyQuery);

        // Two clusters + the header views (privacy disclaimer, clear browsing data, toggle).
        assertEquals(mModelList.size(), mHistoryClustersResultEmptyQuery.getClusters().size() + 3);
        assertThat(mModelList,
                hasItemTypes(ItemType.PRIVACY_DISCLAIMER, ItemType.CLEAR_BROWSING_DATA,
                        ItemType.TOGGLE, ItemType.CLUSTER, ItemType.CLUSTER));

        ListItem item = mModelList.get(3);
        PropertyModel model = item.model;
        assertTrue(model.getAllSetProperties().containsAll(
                Arrays.asList(HistoryClustersItemProperties.CLICK_HANDLER,
                        HistoryClustersItemProperties.TITLE, HistoryClustersItemProperties.LABEL)));
    }

    @Test
    public void testHeaderVisibility() {
        Promise<HistoryClustersResult> promise = new Promise<>();
        doReturn(promise).when(mBridge).queryClusters("");

        mMediator.setQueryState(QueryState.forQueryless());
        mMediator.startQuery("");
        fulfillPromise(promise, HistoryClustersResult.emptyResult());

        assertThat(mModelList,
                hasItemTypes(ItemType.PRIVACY_DISCLAIMER, ItemType.CLEAR_BROWSING_DATA,
                        ItemType.TOGGLE));

        mShouldShowPrivacyDisclaimerSupplier.set(false);
        assertThat(mModelList, hasItemTypes(ItemType.CLEAR_BROWSING_DATA, ItemType.TOGGLE));

        mShouldShowClearBrowsingDataSupplier.set(false);
        assertThat(mModelList, hasItemTypes(ItemType.TOGGLE));

        mShouldShowClearBrowsingDataSupplier.set(true);
        assertThat(mModelList, hasItemTypes(ItemType.CLEAR_BROWSING_DATA, ItemType.TOGGLE));

        mShouldShowPrivacyDisclaimerSupplier.set(true);
        assertThat(mModelList,
                hasItemTypes(ItemType.PRIVACY_DISCLAIMER, ItemType.CLEAR_BROWSING_DATA,
                        ItemType.TOGGLE));
    }

    @Test
    public void testOpenInFullPageTablet() {
        doReturn(2).when(mResources).getInteger(R.integer.min_screen_width_bucket);
        mMediator.openHistoryClustersUi("pandas");
        verify(mTab).loadUrl(argThat(hasSameUrl("chrome://history/journeys?q=pandas")));
    }

    @Test
    public void testOpenInFullPagePhone() {
        doReturn(1).when(mResources).getInteger(R.integer.min_screen_width_bucket);
        mMediator.openHistoryClustersUi("pandas");

        verify(mContext).startActivity(mIntent);
        assertTrue(IntentUtils.safeGetBooleanExtra(
                mIntent, HistoryClustersConstants.EXTRA_SHOW_HISTORY_CLUSTERS, false));
        assertEquals(IntentUtils.safeGetStringExtra(
                             mIntent, HistoryClustersConstants.EXTRA_HISTORY_CLUSTERS_QUERY),
                "pandas");
    }

    @Test
    public void testSearchTextChanged() {
        doReturn(new Promise<>()).when(mBridge).queryClusters("pan");
        // Add a dummy entry to mModelList so we can check it was cleared.
        mModelList.add(new ListItem(42, new PropertyModel()));
        mMediator.setQueryState(QueryState.forQuery("pan", ""));
        mMediator.onSearchTextChanged("pan");

        assertEquals(mModelList.size(), 1);
        ListItem spinnerItem = mModelList.get(0);
        assertEquals(spinnerItem.type, ItemType.MORE_PROGRESS);
        assertEquals(spinnerItem.model.get(HistoryClustersItemProperties.PROGRESS_BUTTON_STATE),
                State.LOADING);
        verify(mBridge).queryClusters("pan");

        doReturn(new Promise<>()).when(mBridge).queryClusters("");

        mMediator.onEndSearch();

        verify(mBridge).queryClusters("");
    }

    @Test
    public void testSetQueryState() {
        mMediator.setQueryState(QueryState.forQuery("pandas", "empty string"));
        assertEquals(mToolbarModel.get(HistoryClustersToolbarProperties.QUERY_STATE).getQuery(),
                "pandas");
    }

    @Test
    public void testNavigate() {
        mMediator.navigateToUrlInCurrentTab(mMockGurl, false);

        verify(mTab).loadUrl(argThat(hasSameUrl(ITEM_URL_SPEC)));
    }

    @Test
    public void testNavigateSeparateActivity() {
        mIsSeparateActivity = true;
        mMediator.navigateToUrlInCurrentTab(mMockGurl, false);
        verify(mContext).startActivity(mIntent);
    }

    @Test
    public void testItemClicked() {
        mIsSeparateActivity = true;
        mMediator.onClusterVisitClicked(null, mVisit1);

        verify(mContext).startActivity(mIntent);
        assertEquals(mIntent.getDataString(), mVisit1.getNormalizedUrl().getSpec());
        verify(mMetricsLogger)
                .recordVisitAction(HistoryClustersMetricsLogger.VisitAction.CLICKED, mVisit1);
    }

    @Test
    public void testRelatedSearchesClick() {
        doReturn(true).when(mTemplateUrlService).isLoaded();
        doReturn(JUnitTestGURLs.GOOGLE_URL_DOGS)
                .when(mTemplateUrlService)
                .getUrlForSearchQuery("dogs");
        mMediator.onRelatedSearchesChipClicked("dogs", 2);
        verify(mMetricsLogger).recordRelatedSearchesClick(2);
        verify(mTab).loadUrl(argThat(hasSameUrl(JUnitTestGURLs.GOOGLE_URL_DOGS)));
    }

    @Test
    public void testToggleClusterVisibility() {
        PropertyModel clusterModel = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        PropertyModel clusterModel2 = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        PropertyModel visitModel1 = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        PropertyModel visitModel2 = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        PropertyModel visitModel3 = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        PropertyModel visitModel4 = new PropertyModel(HistoryClustersItemProperties.ALL_KEYS);
        List<ListItem> visitItemsToHide = Arrays.asList(new ListItem(ItemType.VISIT, visitModel1),
                new ListItem(ItemType.VISIT, visitModel2));
        List<ListItem> visitItemsToHide2 = Arrays.asList(new ListItem(ItemType.VISIT, visitModel3),
                new ListItem(ItemType.VISIT, visitModel4));
        ListItem clusterItem1 = new ListItem(ItemType.CLUSTER, clusterModel);
        ListItem clusterItem2 = new ListItem(ItemType.CLUSTER, clusterModel2);
        mModelList.add(clusterItem1);
        mModelList.addAll(visitItemsToHide);
        mModelList.add(clusterItem2);
        mModelList.addAll(visitItemsToHide2);

        mMediator.hideCluster(clusterItem1, visitItemsToHide);
        assertEquals(mModelList.indexOf(visitItemsToHide.get(0)), -1);
        assertEquals(mModelList.indexOf(visitItemsToHide.get(1)), -1);
        assertEquals(4, mModelList.size());
        assertEquals(ClusterViewAccessibilityState.EXPANDABLE,
                clusterModel.get(HistoryClustersItemProperties.ACCESSIBILITY_STATE));

        mMediator.hideCluster(clusterItem2, visitItemsToHide2);
        assertEquals(mModelList.indexOf(visitItemsToHide2.get(0)), -1);
        assertEquals(mModelList.indexOf(visitItemsToHide2.get(1)), -1);
        assertEquals(2, mModelList.size());
        assertEquals(ClusterViewAccessibilityState.EXPANDABLE,
                clusterModel2.get(HistoryClustersItemProperties.ACCESSIBILITY_STATE));

        mMediator.showCluster(clusterItem2, visitItemsToHide2);
        assertEquals(mModelList.indexOf(visitItemsToHide2.get(0)), 2);
        assertEquals(mModelList.indexOf(visitItemsToHide2.get(1)), 3);
        assertEquals(4, mModelList.size());
        assertEquals(ClusterViewAccessibilityState.COLLAPSIBLE,
                clusterModel2.get(HistoryClustersItemProperties.ACCESSIBILITY_STATE));

        mMediator.showCluster(clusterItem1, visitItemsToHide);
        assertEquals(mModelList.indexOf(visitItemsToHide.get(0)), 1);
        assertEquals(mModelList.indexOf(visitItemsToHide.get(1)), 2);
        assertEquals(6, mModelList.size());
        assertEquals(ClusterViewAccessibilityState.COLLAPSIBLE,
                clusterModel.get(HistoryClustersItemProperties.ACCESSIBILITY_STATE));
    }

    @Test
    public void testGetTimeString() {
        String dayString = "1 day ago";
        String hourString = "1 hour ago";
        String minuteString = "1 minute ago";
        String justNowString = "Just now";

        doReturn(dayString)
                .when(mResources)
                .getQuantityString(eq(R.plurals.n_days_ago), geq(1), geq(1));
        doReturn(hourString)
                .when(mResources)
                .getQuantityString(eq(R.plurals.n_hours_ago), geq(1), geq(1));
        doReturn(minuteString)
                .when(mResources)
                .getQuantityString(eq(R.plurals.n_minutes_ago), geq(1), geq(1));
        doReturn(justNowString).when(mResources).getString(R.string.just_now);

        doReturn(TimeUnit.DAYS.toMillis(1)).when(mClock).currentTimeMillis();
        String timeString = mMediator.getTimeString(0L);
        assertEquals(timeString, dayString);

        doReturn(TimeUnit.HOURS.toMillis(1)).when(mClock).currentTimeMillis();
        timeString = mMediator.getTimeString(0L);
        assertEquals(timeString, hourString);

        doReturn(TimeUnit.MINUTES.toMillis(1)).when(mClock).currentTimeMillis();
        timeString = mMediator.getTimeString(0L);
        assertEquals(timeString, minuteString);

        doReturn(TimeUnit.SECONDS.toMillis(1)).when(mClock).currentTimeMillis();
        timeString = mMediator.getTimeString(0L);
        assertEquals(timeString, justNowString);
    }

    @Test
    public void testScrolling() {
        Promise<HistoryClustersResult> promise = new Promise();
        doReturn(promise).when(mBridge).queryClusters("query");
        Promise<HistoryClustersResult> secondPromise = new Promise();
        doReturn(secondPromise).when(mBridge).loadMoreClusters("query");
        doReturn(3).when(mLayoutManager).findLastVisibleItemPosition();

        mMediator.setQueryState(QueryState.forQuery("query", ""));
        mMediator.startQuery("query");
        fulfillPromise(promise, mHistoryClustersResultWithQuery);

        mMediator.onScrolled(mRecyclerView, 1, 1);
        ShadowLooper.idleMainLooper();

        verify(mBridge).loadMoreClusters("query");
        fulfillPromise(secondPromise, mHistoryClustersFollowupResultWithQuery);

        doReturn(4).when(mLayoutManager).findLastVisibleItemPosition();
        mMediator.onScrolled(mRecyclerView, 1, 1);
        // There should have been no further calls to loadMoreClusters since the current result
        // specifies that it has no more data.
        verify(mBridge).loadMoreClusters("query");
    }

    @Test
    public void testBolding() {
        String text = "this is fun";
        List<MatchPosition> matchPositions =
                Arrays.asList(new MatchPosition(0, 5), new MatchPosition(8, 10));
        SpannableString result = mMediator.applyBolding(text, matchPositions);

        StyleSpan[] styleSpans = result.getSpans(0, 5, StyleSpan.class);
        assertEquals(styleSpans.length, 1);
        assertEquals(styleSpans[0].getStyle(), Typeface.BOLD);

        styleSpans = result.getSpans(8, 10, StyleSpan.class);
        assertEquals(styleSpans.length, 1);
        assertEquals(styleSpans[0].getStyle(), Typeface.BOLD);
    }

    @Test
    public void testDeleteItems() {
        Promise<HistoryClustersResult> promise = new Promise();
        doReturn(promise).when(mBridge).queryClusters("query");
        mMediator.setQueryState(QueryState.forQuery("query", ""));
        mMediator.startQuery("query");
        fulfillPromise(promise, mHistoryClustersResultWithQuery);
        int initialSize = mModelList.size();

        mMediator.deleteVisits(Arrays.asList(mVisit1, mVisit3));
        assertThat(mVisitsForRemoval, Matchers.containsInAnyOrder(mVisit1, mVisit3));
        verify(mMetricsLogger)
                .recordVisitAction(HistoryClustersMetricsLogger.VisitAction.DELETED, mVisit1);
        verify(mMetricsLogger)
                .recordVisitAction(HistoryClustersMetricsLogger.VisitAction.DELETED, mVisit3);
        // Deleting all of the visits in a cluster should also delete the ModelList entry for the
        // cluster itself.
        assertEquals(initialSize - 3, mModelList.size());

        ListItem clusterItem = mModelList.get(0);
        assertEquals(clusterItem.type, ItemType.CLUSTER);

        ListItem visitItem = mModelList.get(1);
        assertEquals(visitItem.type, ItemType.VISIT);
        PropertyModel visitModel = visitItem.model;
        assertEquals(mMediator.applyBolding(mVisit2.getTitle(), mVisit2.getTitleMatchPositions()),
                visitModel.get(HistoryClustersItemProperties.TITLE));
        assertEquals(
                mMediator.applyBolding(mVisit2.getUrlForDisplay(), mVisit2.getUrlMatchPositions()),
                visitModel.get(HistoryClustersItemProperties.URL));

        ListItem relatedSearchesItem = mModelList.get(2);
        assertEquals(relatedSearchesItem.type, ItemType.RELATED_SEARCHES);
        PropertyModel relatedSearchesModel = relatedSearchesItem.model;
        assertEquals(mCluster1.getRelatedSearches(),
                relatedSearchesModel.get(HistoryClustersItemProperties.RELATED_SEARCHES));

        mMediator.deleteVisits(Arrays.asList(mVisit2));
        // Deleting the final visit should result in an entirely empty list.
        assertEquals(0, mModelList.size());
    }

    @Test
    public void testOpenInNewTab() {
        mIsSeparateActivity = true;
        mMediator.openVisitsInNewTabs(Arrays.asList(mVisit1, mVisit2), false, false);
        verify(mContext).startActivity(mIntent);
        assertEquals(true, mIntent.getBooleanExtra(NEW_TAB_EXTRA, false));
        assertEquals(false, mIntent.getBooleanExtra(INCOGNITO_EXTRA, true));
        assertEquals(false, mIntent.getBooleanExtra(TAB_GROUP_EXTRA, true));
        assertEquals(mGurl2.getSpec(),
                ((List<String>) mIntent.getSerializableExtra(ADDTIONAL_URLS_EXTRA)).get(0));

        mMediator.openVisitsInNewTabs(Arrays.asList(mVisit1, mVisit2), true, false);
        assertEquals(true, mIntent.getBooleanExtra(INCOGNITO_EXTRA, true));

        mIsSeparateActivity = false;
        doReturn(mTab2).when(mTabCreator).createNewTab(any(), anyInt(), any());
        mMediator.openVisitsInNewTabs(Arrays.asList(mVisit1, mVisit2), false, false);
        verify(mTabCreator)
                .createNewTab(argThat(hasSameUrl(mGurl1.getSpec())),
                        eq(TabLaunchType.FROM_CHROME_UI), eq(null));
        verify(mTabCreator)
                .createNewTab(argThat(hasSameUrl(mGurl2.getSpec())),
                        eq(TabLaunchType.FROM_CHROME_UI), eq(mTab2));
    }

    @Test
    public void testOpenInGroup() {
        mIsSeparateActivity = true;
        mMediator.openVisitsInNewTabs(Arrays.asList(mVisit1, mVisit2), false, true);
        verify(mContext).startActivity(mIntent);
        assertEquals(true, mIntent.getBooleanExtra(NEW_TAB_EXTRA, false));
        assertEquals(false, mIntent.getBooleanExtra(INCOGNITO_EXTRA, true));
        assertEquals(true, mIntent.getBooleanExtra(TAB_GROUP_EXTRA, true));
        assertEquals(mGurl2.getSpec(),
                ((List<String>) mIntent.getSerializableExtra(ADDTIONAL_URLS_EXTRA)).get(0));

        mIsSeparateActivity = false;
        doReturn(mTab2).when(mTabCreator).createNewTab(any(), anyInt(), any());
        mMediator.openVisitsInNewTabs(Arrays.asList(mVisit1, mVisit2), false, true);
        verify(mTabCreator)
                .createNewTab(argThat(hasSameUrl(mGurl1.getSpec())),
                        eq(TabLaunchType.FROM_CHROME_UI), eq(null));
        verify(mTabCreator)
                .createNewTab(argThat(hasSameUrl(mGurl2.getSpec())),
                        eq(TabLaunchType.FROM_CHROME_UI), eq(mTab2));
    }

    @Test
    public void testHideAfterDelete() {
        Promise<HistoryClustersResult> promise = new Promise<>();
        doReturn(promise).when(mBridge).queryClusters("query");

        mMediator.setQueryState(QueryState.forQuery("query", ""));
        mMediator.startQuery("query");
        fulfillPromise(promise, mHistoryClustersResultWithQuery);

        mMediator.deleteVisits(Arrays.asList(mVisit1));
        assertEquals(ItemType.CLUSTER, mModelList.get(0).type);
        PropertyModel clusterModel = mModelList.get(0).model;
        clusterModel.get(HistoryClustersItemProperties.CLICK_HANDLER).onClick(null);
    }

    private <T> void fulfillPromise(Promise<T> promise, T result) {
        promise.fulfill(result);
        ShadowLooper.idleMainLooper();
    }

    static ArgumentMatcher<LoadUrlParams> hasSameUrl(String url) {
        return argument -> argument.getUrl().equals(url);
    }

    static Matcher<ModelList> hasItemTypes(@ItemType int... itemTypes) {
        return new BaseMatcher<ModelList>() {
            @Override
            public void describeTo(Description description) {
                description.appendText("Has item types: " + Arrays.toString(itemTypes));
            }

            @Override
            public boolean matches(Object object) {
                ModelList modelList = (ModelList) object;
                if (itemTypes.length != modelList.size()) {
                    return false;
                }
                int i = 0;
                for (ListItem listItem : modelList) {
                    if (listItem.type != itemTypes[i++]) {
                        return false;
                    }
                }
                return true;
            }
        };
    }
}

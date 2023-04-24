package org.chromium.chrome.browser.toolbar.circlemenu;

import android.view.View;
import android.widget.AbsListView;

import androidx.annotation.NonNull;

abstract class AbsListViewScrollDetector implements AbsListView.OnScrollListener {

    private int mLastScrollY;
    private int mPreviousFirstVisibleItem;
    private AbsListView mListView;
    private int mScrollThreshold;

    abstract void onScrollUp();

    abstract void onScrollDown();

    /**
     *
     * @param view
     * @param scrollState
	 */
    @Override
    public void onScrollStateChanged(AbsListView view, int scrollState) {
    }

    /**
     *
     * @param view
     * @param firstVisibleItem
     * @param visibleItemCount
     * @param totalItemCount
	 */
    @Override
    public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount) {
        if(totalItemCount == 0) return;
        if (isSameRow(firstVisibleItem)) {
            int newScrollY = getTopItemScrollY();
            boolean isSignificantDelta = Math.abs(mLastScrollY - newScrollY) > mScrollThreshold;
            if (isSignificantDelta) {
                if (mLastScrollY > newScrollY) {
                    onScrollUp();
                } else {
                    onScrollDown();
                }
            }
            mLastScrollY = newScrollY;
        } else {
            if (firstVisibleItem > mPreviousFirstVisibleItem) {
                onScrollUp();
            } else {
                onScrollDown();
            }

            mLastScrollY = getTopItemScrollY();
            mPreviousFirstVisibleItem = firstVisibleItem;
        }
    }

    /**
     *
     * @param scrollThreshold
	 */
    public void setScrollThreshold(int scrollThreshold) {
        mScrollThreshold = scrollThreshold;
    }

    /**
     *
     * @param listView
	 */
    public void setListView(@NonNull AbsListView listView) {
        mListView = listView;
    }

    private boolean isSameRow(int firstVisibleItem) {
        return firstVisibleItem == mPreviousFirstVisibleItem;
    }

    /**
     *
     * @return
	 */
    private int getTopItemScrollY() {
        if (mListView == null || mListView.getChildAt(0) == null) return 0;
        View topChild = mListView.getChildAt(0);
        return topChild.getTop();
    }
}

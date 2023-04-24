package org.chromium.chrome.browser.toolbar.circlemenu;

import androidx.recyclerview.widget.RecyclerView;

abstract class RecyclerViewScrollDetector extends RecyclerView.OnScrollListener {
	private int mScrollThreshold;

	abstract void onScrollUp();

	abstract void onScrollDown();

	/**
	 *
	 * @param recyclerView
	 * @param dx
	 * @param dy
	 */
	@Override
	public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
		boolean isSignificantDelta = Math.abs(dy) > mScrollThreshold;
		if (isSignificantDelta) {
			if (dy > 0) {
				onScrollUp();
			} else {
				onScrollDown();
			}
		}
	}

	/**
	 *
	 * @param scrollThreshold
	 */
	public void setScrollThreshold(int scrollThreshold) {
		mScrollThreshold = scrollThreshold;
	}
}

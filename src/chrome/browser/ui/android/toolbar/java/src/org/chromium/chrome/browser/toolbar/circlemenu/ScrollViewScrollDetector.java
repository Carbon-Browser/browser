package org.chromium.chrome.browser.toolbar.circlemenu;

import android.widget.ScrollView;

import org.chromium.chrome.browser.toolbar.circlemenu.ScrollViewScrollDetector;
import org.chromium.chrome.browser.toolbar.circlemenu.widget.ObservableScrollView;

public abstract class ScrollViewScrollDetector implements ObservableScrollView.OnScrollChangedListener {
	private int mLastScrollY;
	private int mScrollThreshold;

	abstract void onScrollUp();

	abstract void onScrollDown();

	/**
	 *
	 * @param who
	 * @param l
	 * @param t
	 * @param oldl
	 * @param oldt
	 */
	@Override
	public void onScrollChanged(ScrollView who, int l, int t, int oldl, int oldt) {
		boolean isSignificantDelta = Math.abs(t - mLastScrollY) > mScrollThreshold;
		if (isSignificantDelta) {
			if (t > mLastScrollY) {
				onScrollUp();
			} else {
				onScrollDown();
			}
		}
		mLastScrollY = t;
	}

	/**
	 *
	 * @param scrollThreshold
	 */
	public void setScrollThreshold(int scrollThreshold) {
		mScrollThreshold = scrollThreshold;
	}
}

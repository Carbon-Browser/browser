package org.chromium.chrome.browser.toolbar.circlemenu.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ScrollView;


public class ObservableScrollView extends ScrollView {

	/**
	 *
	 */
	public interface OnScrollChangedListener {
		void onScrollChanged(ScrollView who, int l, int t, int oldl, int oldt);
	}

	/**
	 *
	 */
	private OnScrollChangedListener mOnScrollChangedListener;

	public ObservableScrollView(Context context) {
		super(context);
	}

	/**
	 *
	 * @param context
	 * @param attrs
	 */
	public ObservableScrollView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	/**
	 *
	 * @param context
	 * @param attrs
	 * @param defStyle
	 */
	public ObservableScrollView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	/**
	 *
	 * @param l
	 * @param t
	 * @param oldl
	 * @param oldt
	 */
	@Override
	protected void onScrollChanged(int l, int t, int oldl, int oldt) {
		super.onScrollChanged(l, t, oldl, oldt);
		if (mOnScrollChangedListener != null) {
			mOnScrollChangedListener.onScrollChanged(this, l, t, oldl, oldt);
		}
	}

	/**
	 *
	 * @param listener
	 */
	public void setOnScrollChangedListener(OnScrollChangedListener listener) {
		mOnScrollChangedListener = listener;
	}
}

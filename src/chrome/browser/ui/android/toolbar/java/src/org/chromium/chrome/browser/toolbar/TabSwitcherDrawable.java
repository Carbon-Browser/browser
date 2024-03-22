// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint.Align;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.text.TextPaint;

import androidx.annotation.VisibleForTesting;

import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.browser_ui.widget.TintedDrawable;

import java.util.Locale;

import android.graphics.RectF;
import android.graphics.Paint;
import android.graphics.LinearGradient;
import android.graphics.Shader;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff;

/** A drawable for the tab switcher icon. */
public class TabSwitcherDrawable extends TintedDrawable {
    private final float mSingleDigitTextSize;
    private final float mDoubleDigitTextSize;

    private final Rect mTextBounds = new Rect();
    private final TextPaint mTextPaint;

    // Tab Count Label
    private int mTabCount;
    private boolean mIncognito;

    private float density;

    /**
     * Creates a {@link TabSwitcherDrawable}.
     * @param context A {@link Context} instance.
     * @param brandedColorScheme The {@link BrandedColorScheme} used to tint the drawable.
     * @return          A {@link TabSwitcherDrawable} instance.
     */
    public static TabSwitcherDrawable createTabSwitcherDrawable(
            Context context, @BrandedColorScheme int brandedColorScheme) {
        Bitmap icon =
                BitmapFactory.decodeResource(
                        context.getResources(), R.drawable.btn_tabswitcher_modern);
        return new TabSwitcherDrawable(context, brandedColorScheme, icon);
    }

    private TabSwitcherDrawable(
            Context context, @BrandedColorScheme int brandedColorScheme, Bitmap bitmap) {
        super(context, bitmap);
        // setTint(ThemeUtils.getThemedToolbarIconTint(context, brandedColorScheme));
        mSingleDigitTextSize =
                context.getResources().getDimension(R.dimen.toolbar_tab_count_text_size_1_digit);
        mDoubleDigitTextSize =
                context.getResources().getDimension(R.dimen.toolbar_tab_count_text_size_2_digit);

        density = context.getResources().getDisplayMetrics().density;

        mTextPaint = new TextPaint();
        mTextPaint.setAntiAlias(true);
        mTextPaint.setTextAlign(Align.CENTER);
        mTextPaint.setTypeface(Typeface.create("sans-serif-condensed", Typeface.BOLD));
        // mTextPaint.setColor(getColorForState());
    }

    @Override
    protected boolean onStateChange(int[] state) {
        boolean retVal = super.onStateChange(state);
        if (retVal) mTextPaint.setColor(getColorForState());
        return retVal;
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        Rect drawableBounds = getBounds();

        RectF boundsF = new RectF(drawableBounds);
        int padding = Math.round(density*4);
        // boundsF.top = boundsF.top-padding;
        // boundsF.bottom = boundsF.bottom+padding;
        // boundsF.left = boundsF.left-padding;
        // boundsF.right = boundsF.right+padding;

        Paint paint = new Paint();
        LinearGradient shader = new LinearGradient(0, 0, 0, drawableBounds.height(), 0xFFFF882F, 0xFFFF2C07, Shader.TileMode.CLAMP);
        paint.setShader(shader);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
        canvas.drawRoundRect(boundsF, 14f, 14f, paint);

        String textString = getTabCountString();
        if (!textString.isEmpty()) {
            mTextPaint.getTextBounds(textString, 0, textString.length(), mTextBounds);

            int textX = drawableBounds.width() / 2;
            int textY =
                    drawableBounds.height() / 2
                            + (mTextBounds.bottom - mTextBounds.top) / 2
                            - mTextBounds.bottom;

            canvas.drawText(textString, textX, textY, mTextPaint);
        }
    }

    /**
     * @return The current tab count this drawable is displaying.
     */
    @VisibleForTesting
    public int getTabCount() {
        return mTabCount;
    }

    /**
     * Update the visual state based on the number of tabs present.
     * @param tabCount The number of tabs.
     */
    public void updateForTabCount(int tabCount, boolean incognito) {
        if (tabCount == mTabCount && incognito == mIncognito) return;
        mTabCount = tabCount;
        mIncognito = incognito;
        float textSizePx = mTabCount > 9 ? mDoubleDigitTextSize : mSingleDigitTextSize;
        mTextPaint.setTextSize(textSizePx);
        invalidateSelf();
    }

    private String getTabCountString() {
        if (mTabCount <= 0) {
            return "";
        } else if (mTabCount > 99) {
            return mIncognito ? ";)" : ":D";
        } else {
            return String.format(Locale.getDefault(), "%d", mTabCount);
        }
    }

    private int getColorForState() {
        // return mTint.getColorForState(getState(), 0);
        return -1;
    }

    @Override
    public void setTint(ColorStateList tint) {
        // super.setTint(tint);
        // if (mTextPaint != null) mTextPaint.setColor(getColorForState());
    }
}

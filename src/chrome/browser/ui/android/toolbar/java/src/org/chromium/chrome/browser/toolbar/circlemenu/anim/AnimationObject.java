package org.chromium.chrome.browser.toolbar.circlemenu.anim;

import android.view.animation.Animation;
import android.view.animation.Interpolator;
import android.view.animation.RotateAnimation;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;

public class AnimationObject {

    /*
     *
     */
    public static Animation scaleAnimationRelativeToSelf(float xStart,
	    float xEnd, float yStart, float yEnd, float pivotX, float pivotY,
	    int durationMillis, boolean fill) {
		Animation anim = new ScaleAnimation(xStart, xEnd, // Start and end
			// values for
			// the X axis scaling
			yStart, yEnd, // Start and end values for the Y axis scaling
			Animation.RELATIVE_TO_SELF, pivotX, // Pivot point of X scaling
			Animation.RELATIVE_TO_SELF, pivotY); // Pivot point of Y scaling
		anim.setDuration(durationMillis);
		//anim.setFillAfter(fill); // Needed to keep the result of the animation
		return anim;
    }

    /*
     *
     */
    public static Animation scaleAnimation(float xStart, float xEnd,
	    float yStart, float yEnd, int pivotXType, int pivotYType,
	    float pivotX, float pivotY, int durationMillis, boolean fill) {
		Animation anim = new ScaleAnimation(xStart, xEnd, // Start and end
			// values for
			// the X axis scaling
			yStart, yEnd, // Start and end values for the Y axis scaling
			pivotXType, pivotX, // Pivot point of X scaling
			pivotYType, pivotY); // Pivot point of Y scaling
		anim.setDuration(durationMillis);
		//anim.setFillAfter(fill); // Needed to keep the result of the animation
		return anim;
    }

    /*
     *
     */
    public static Animation translateAnimationRelativeToParent(float fromXDelta,
	    float toXDelta, float fromYDelta, float toYDelta,
	    Interpolator interpolator, int durationMillis, boolean fill) {
		Animation anim = new TranslateAnimation(Animation.RELATIVE_TO_PARENT,fromXDelta,
				Animation.RELATIVE_TO_PARENT,toXDelta,
				Animation.RELATIVE_TO_PARENT,fromYDelta,
				Animation.RELATIVE_TO_PARENT, toYDelta);
		// Start and end
		// values for
		// Start and end values for the Y axis scaling
		anim.setInterpolator(interpolator);
		anim.setDuration(durationMillis);
		//anim.setFillAfter(fill); // Needed to keep the result of the animation
		return anim;
    }

	/*
  *
  */
	public static Animation translateAnimation(int relativeType,float fromXDelta,
	                                                           float toXDelta, float fromYDelta, float toYDelta,
	                                                           Interpolator interpolator, int durationMillis , boolean fill) {
		Animation anim = new TranslateAnimation(relativeType,fromXDelta,
				relativeType,toXDelta,
				relativeType,fromYDelta,
				relativeType, toYDelta);
		// Start and end
		// values for
		// Start and end values for the Y axis scaling
		anim.setInterpolator(interpolator);
		anim.setDuration(durationMillis);
		//anim.setFillAfter(fill); // Needed to keep the result of the animation
		return anim;
	}

	public static Animation rotationAnimationRelativeToSelf(float fromDeg,
	                                                        float toDeg, float pivotX, float pivotY,
	                                                        int durationMillis,boolean fill) {
		Animation anim = new RotateAnimation(fromDeg,toDeg, // Start and end values for the Y axis scaling
				Animation.RELATIVE_TO_SELF, pivotX, // Pivot point of X scaling
				Animation.RELATIVE_TO_SELF, pivotY); // Pivot point of Y scaling
		anim.setDuration(durationMillis);
		//anim.setFillAfter(fill); // Needed to keep the result of the animation
		return anim;
	}

}

package org.chromium.chrome.browser.ntp;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.util.Random;

import org.chromium.chrome.browser.app.ChromeActivity;
import android.graphics.drawable.Drawable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import android.view.View;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import org.chromium.base.ContextUtils;

import android.widget.ImageView;
import android.content.Context;

/**
* Class to set the background image of the homepage
* sequence goes -> getBackground from NewTabPageLayout -> onBackgroundReceived called in NewTabPageLayout
* -> setBackground called from NewTabPageLayout
*/
public class BackgroundController {

    public interface NTPBackgroundInterface {
        void onBackgroundReceived(String credit, String creditUrl, String imgUrl);
    }

    public BackgroundController() {
    }

    public void setBackground(final ImageView v, final String imageUrl) {
        Glide.with(v)
            .load(imageUrl)
            .thumbnail(0.1f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .into(new CustomTarget<Drawable>() {
                @Override
                public void onResourceReady(@NonNull Drawable resource, @Nullable Transition<? super Drawable> transition) {
                    v.setImageDrawable(resource);
                }

                @Override
                public void onLoadCleared(@Nullable Drawable placeholder) {

                }
            });
    }

    public void getBackground(ChromeActivity activity, NTPBackgroundInterface mInterface) {
        SharedPreferences pref = ContextUtils.getAppSharedPreferences();
        //int msSinceUpdate = Long.valueOf(System.currentTimeMillis()).intValue() - pref.getInt("ntp_bg_last_update", 0);

        boolean shouldCache = pref.getBoolean("should_cache_bg_images", true);

        // cache an image, refresh after 2 hours
        // Commenting all of this so the background is refreshed for every new tab
        /*if (msSinceUpdate <= 7200000) {
            String credit = pref.getString("ntp_bg_credit", null);
            String creditUrl = pref.getString("ntp_bg_credit_url", null);
            String imgUrl = pref.getString("ntp_bg_url", null);

            if (credit == null || creditUrl == null || imgUrl == null) {
                fetchImagesJSON(activity, mInterface);
            } else {
                mInterface.onBackgroundReceived(credit, creditUrl, imgUrl);
            }
        } else {*/
            // REMOVE THIS TO RESTORE  // fetchImagesJSON(shouldCache, activity, mInterface);
        //}
    }

    public void fetchImagesJSON(final boolean shouldCache, final ChromeActivity activity, final NTPBackgroundInterface mInterface) {
        new Thread(() -> {
            URL url;
            StringBuffer response = new StringBuffer();
            try {
                url = new URL("https://sigmawolf.io/android-resources/unsplash/bg_images.json");
              } catch (MalformedURLException e) {
                  throw new IllegalArgumentException("invalid url");
              }

              HttpURLConnection conn = null;
              try {
                  conn = (HttpURLConnection) url.openConnection();
                  conn.setDoOutput(false);
                  conn.setConnectTimeout(4000);
                  conn.setDoInput(true);
                  conn.setUseCaches(false);
                  conn.setRequestMethod("GET");
                  conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

                  // handle the response
                  int status = conn.getResponseCode();
                  if (status != 200) {
                      throw new IOException("Post failed with error code " + status);
                  } else {
                      BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                      String inputLine;
                      while ((inputLine = in.readLine()) != null) {
                          response.append(inputLine);
                      }
                      in.close();
                  }
              } catch (SocketTimeoutException timeout) {
                  // Time out, don't set a background - lazy
              } catch (Exception e) {
                  e.printStackTrace();

                  // error occured and no background WOULD be set...
                  // get the bg info from the last fetch and use that
                  SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();

                  String lastCredit = mPrefs.getString("ntp_last_credit", null);
                  String lastCreditUrl = mPrefs.getString("ntp_last_credit_url", null);
                  String lastImgUrl = mPrefs.getString("ntp_last_img_url", null);

                  if (lastCredit != null && lastCreditUrl != null && lastImgUrl != null) {
                      mInterface.onBackgroundReceived(lastCredit, lastCreditUrl, lastImgUrl);
                  }
              } finally {
                  if (conn != null) {
                      conn.disconnect();
                  }

                  activity.runOnUiThread(() -> {
                      try {
                          JSONObject jsonObject = new JSONObject(response.toString());
                          JSONArray photos = jsonObject.getJSONArray("photos");

                          final int min = 0;
                          final int max = photos.length() - 1;
                          final int random = new Random().nextInt((max - min) + 1) + min;

                          JSONObject photoObject = (JSONObject) photos.get(random);

                          String credit = photoObject.getString("credit");
                          String creditUrl = photoObject.getString("credit_url_photographer");
                          String imgUrl = photoObject.getString("image_url");

                          // cache the background
                          SharedPreferences.Editor editor = ContextUtils.getAppSharedPreferences().edit();
                          editor.putInt("ntp_bg_last_update", Long.valueOf(System.currentTimeMillis()).intValue());
                          editor.putString("ntp_bg_credit", credit);
                          editor.putString("ntp_bg_credit_url", creditUrl);
                          editor.putString("ntp_bg_url", imgUrl);

                          if (shouldCache) {
                              editor.putBoolean("should_cache_bg_images", false);

                              cacheBackgroundImages(photos, activity);
                          }

                          mInterface.onBackgroundReceived(credit, creditUrl, imgUrl);

                          // cache the bg img info incase it fails to get it next time
                          editor.putString("ntp_last_credit", credit);
                          editor.putString("ntp_last_credit_url", creditUrl);
                          editor.putString("ntp_last_img_url", imgUrl);

                          // save
                          editor.apply();
                      } catch (Exception e) { }
                  });
              }
          }).start();
      }

    private void cacheBackgroundImages(final JSONArray photos, final Context context) {
        try {
            for (int i = 0; i != (photos.length() - 1); i++) {
                JSONObject photoObject = (JSONObject) photos.get(i);

                Glide.with(context)
                    .load(photoObject.getString("image_url"))
                    .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                    .preload();
            }
        } catch (Exception e) { }
    }
}

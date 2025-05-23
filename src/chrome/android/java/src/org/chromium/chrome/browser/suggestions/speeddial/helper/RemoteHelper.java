package org.chromium.chrome.browser.suggestions.speeddial.helper;

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
import org.chromium.base.task.AsyncTask;

public class RemoteHelper {
    private static String ip;

    public interface SpeedDialInterface {
        void onDappReceived(String json);
    }

    public interface TakeoverInterface {
        void onTakeoverReceived(String json);
    }

    public RemoteHelper(SpeedDialInterface mInterface) {
      SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();

      String tempIP = mPrefs.getString("u_ip", null);
      String userGeo = mPrefs.getString("u_geo", null);
      if (tempIP != null) {
        ip = tempIP;
        if (userGeo == null) {
          getUserGeo(mInterface);
        }
      } else {
        getUserIP(mInterface);
      }
    }

    public void getUserGeo(SpeedDialInterface mInterface) {
      new AsyncTask<String>() {
          @Override
          protected String doInBackground() {
              HttpURLConnection conn = null;
              StringBuffer response = new StringBuffer();
              try {
                  URL mUrl = new URL("https://carbon.website/carbon/android-resources/country-getter/?key=4ktn59ugn93473474nedkdvwoeegzz");

                  conn = (HttpURLConnection) mUrl.openConnection();
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

              } finally {
                  if (conn != null)
                      conn.disconnect();
              }

              return response.toString();
          }

          @Override
          protected void onPostExecute(String result) {
              try {
                  if (result == null || result == "") return;

                  SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
                  mPrefs.edit().putString("u_geo", result.toString()).apply();

                  String json = mPrefs.getString("cached_dapps", null);
                  if (json != null && !json.trim().isEmpty()) {
                      // Only proceed if mInterface is not null
                      if (mInterface != null) {
                          mInterface.onDappReceived(json);
                      }
                  }
              } catch (Exception ignore) {}
          }
      }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getUserIP(SpeedDialInterface mInterface) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL("https://carbon.website/carbon/android-resources/ip-getter/?key=trccfbgf3q98hr9ofpbjevlksjdcb");

                    conn = (HttpURLConnection) mUrl.openConnection();
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

                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                try {
                    if (result == null || result == "") return;

                    SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
                    mPrefs.edit().putString("u_ip", result.toString()).apply();

                    if (ip == null) ip = result;
                    getUserGeo(mInterface);
                } catch (Exception ignore) {}
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getDapps(ChromeActivity activity, SpeedDialInterface mInterface) {
        SharedPreferences pref = ContextUtils.getAppSharedPreferences();
        int msSinceUpdate = Long.valueOf(System.currentTimeMillis()).intValue() - pref.getInt("dapps_last_update", 0);

        // boolean shouldCache = pref.getBoolean("should_cache_dapps", true);

        // cache, refresh after 1 day
        if (msSinceUpdate <= 86400000) {
            String json = pref.getString("cached_dapps", null);

            if (json == null || json.equals("") || json.trim().isEmpty()) {
                fetchDappsJSON(activity, mInterface);
            } else {
                mInterface.onDappReceived(json);
            }
        } else {
            fetchDappsJSON(activity, mInterface);
        }
    }

    public void getTakeover(ChromeActivity activity, TakeoverInterface mInterface) {
        SharedPreferences pref = ContextUtils.getAppSharedPreferences();
        int msSinceUpdate = Long.valueOf(System.currentTimeMillis()).intValue() - pref.getInt("takeover_last_update", 0);

        // boolean shouldCache = pref.getBoolean("should_cache_dapps", true);

        // cache, refresh after 1 day
        if (msSinceUpdate <= 86400000) {
            String json = pref.getString("cached_takeover", null);

            if (json == null || json.equals("") || json.trim().isEmpty()) {
                fetchTakeoverJSON(activity, mInterface);
            } else {
                mInterface.onTakeoverReceived(json);
            }
        } else {
            fetchTakeoverJSON(activity, mInterface);
        }
    }

    public void fetchTakeoverJSON(final ChromeActivity activity, final TakeoverInterface mInterface) {
        new Thread(() -> {
            URL url;
            StringBuffer response = new StringBuffer();
            try {
                url = new URL("https://carbon.website/carbon/android-resources/takeover/info.json");
              } catch (MalformedURLException e) {
                  throw new IllegalArgumentException("invalid url");
              }

              HttpURLConnection conn = null;
              try {
                  conn = (HttpURLConnection) url.openConnection();
                  conn.setDoOutput(true);
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

                  String json = mPrefs.getString("cached_takeover", null);

                  if (json != null) {
                      mInterface.onTakeoverReceived(json);
                  }
              } finally {
                  if (conn != null) {
                      conn.disconnect();
                  }

                  activity.runOnUiThread(() -> {
                      try {
                          JSONObject test = new JSONObject(response.toString());

                          mInterface.onTakeoverReceived(response.toString());

                          // cache the background
                          SharedPreferences.Editor editor = ContextUtils.getAppSharedPreferences().edit();
                          editor.putInt("takeover_last_update", Long.valueOf(System.currentTimeMillis()).intValue());
                          editor.putString("cached_takeover", response.toString());

                          // if (shouldCache) {
                          //     editor.putBoolean("should_cache_bg_images", false);
                          //
                          //     cacheDapps(json, activity);
                          // }

                          // save
                          editor.commit();
                      } catch (Exception e) { }
                  });
              }
          }).start();
      }

    public void fetchDappsJSON(final ChromeActivity activity, final SpeedDialInterface mInterface) {
        new Thread(() -> {
            URL url;
            StringBuffer response = new StringBuffer();
            try {
                url = new URL("https://carbon.website/carbon/android-resources/dapps/info.json");
              } catch (MalformedURLException e) {
                  throw new IllegalArgumentException("invalid url");
              }

              HttpURLConnection conn = null;
              try {
                  conn = (HttpURLConnection) url.openConnection();
                  conn.setDoOutput(true);
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

                  String json = mPrefs.getString("cached_dapps", null);

                  if (json != null) {
                      mInterface.onDappReceived(json);
                  }
              } finally {
                  if (conn != null) {
                      conn.disconnect();
                  }

                  activity.runOnUiThread(() -> {
                      try {
                          JSONArray test = new JSONArray(response.toString());

                          mInterface.onDappReceived(response.toString());

                          // cache the background
                          SharedPreferences.Editor editor = ContextUtils.getAppSharedPreferences().edit();
                          editor.putInt("dapps_last_update", Long.valueOf(System.currentTimeMillis()).intValue());
                          editor.putString("cached_dapps", response.toString());

                          // if (shouldCache) {
                          //     editor.putBoolean("should_cache_bg_images", false);
                          //
                          //     cacheDapps(json, activity);
                          // }

                          // save
                          editor.commit();
                      } catch (Exception e) { }
                  });
              }
          }).start();
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

    // private void cacheDapps(final JSONArray photos, final Context context) {
    //     try {
    //         for (int i = 0; i != (photos.length() - 1); i++) {
    //             JSONObject photoObject = (JSONObject) photos.get(i);
    //
    //             Glide.with(context)
    //                 .load(photoObject.getString("image_url"))
    //                 .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
    //                 .preload();
    //         }
    //     } catch (Exception e) { }
    // }
}

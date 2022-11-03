package org.chromium.chrome.browser.monetization;

import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;

import com.amplitude.api.Amplitude;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.lang.System;
import org.chromium.base.task.AsyncTask;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.util.UUID;
import org.json.JSONObject;
import org.json.JSONArray;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import org.chromium.url.GURL;
import org.chromium.chrome.browser.tabmodel.document.TabDelegate;
import android.content.ComponentName;
import android.content.Context;
import org.chromium.chrome.browser.monetization.VeveUniversalObj;

public class VeveBridge {

    public interface VeveBookmarkCommunicator {
        void onVeveBookmarkReceived(String url, TabDelegate tabDelegate);

        void onReceiveVeveError(String url, TabDelegate tabDelegate);
    }

    public interface VeveBookmarkUtilCommunicator {
        void onVeveBookmarkReceived(String url, Context context, ComponentName openBookmarkComponentName);

        void onReceiveVeveError(String url, Context context, ComponentName openBookmarkComponentName);
    }

    public interface VeveUniversalCommunicator {
        void onUniversalAdsReceived(ArrayList<VeveUniversalObj> ads);
    }

    private static VeveBridge sVeveBridge;

    private static String deviceID;

    private SharedPreferences mPrefs;

    private static String ip;

    public VeveBridge() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        if (deviceID == null) {
            deviceID = mPrefs.getString("unique_id", null);
            if (deviceID == null) {
                deviceID = UUID.randomUUID().toString().replace("-", "").substring(0, 16);
                mPrefs.edit().putString("unique_id", deviceID).apply();
            }
        }

        if (ip == null) {
            String tempIP = mPrefs.getString("u_ip", null);
            if (tempIP != null) ip = tempIP;

            this.getUserIP();
        }
    }

    public static VeveBridge getInstance() {
        if (sVeveBridge == null) {
            sVeveBridge = new VeveBridge();
        }

        return sVeveBridge;
    }

    public void logImpression(String url) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL();

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setDoOutput(false);
                    conn.setConnectTimeout(3000);
                    conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("PUT");
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

            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void getUserIP() {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL();

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

                    if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
                    mPrefs.edit().putString("u_ip", result.toString()).apply();

                    if (ip == null) ip = result;
                } catch (Exception ignore) {}
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getUniversalAds(VeveUniversalCommunicator communicator) {
        if (ip == null) return;

        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL();

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
                if (result != null) {
                    if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
                    try {
                        JSONObject jsonObject = new JSONObject(result);
                        JSONArray jsonArray = jsonObject.getJSONArray("data");

                        ArrayList<VeveUniversalObj> arr = new ArrayList<>();
                        for (int i = 0; i != jsonArray.length(); i++) {
                            JSONObject jsonArrayObject = jsonArray.getJSONObject(0);

                            String title = jsonArrayObject.getString("title");
                            String description = jsonArrayObject.getString("desc");
                            String imgUrl = jsonArrayObject.getString("iurl");
                            String url = jsonArrayObject.getString("rurl");
                            String brand = jsonArrayObject.getString("brand");
                            String impurl = jsonArrayObject.getString("impurl");

                            arr.add(new VeveUniversalObj(imgUrl, url, title, description, brand, impurl));
                        }

                        communicator.onUniversalAdsReceived(arr);

                        // if (error) {
                        //     communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                        //     return;
                        // }

                        // todo process success
                    } catch (Exception e) {
                        // communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getBookmarkUrl(final String bookmarkUrl, VeveBookmarkCommunicator communicator, TabDelegate tabDelegate) {
        if (ip == null) {
            communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
            return;
        }

        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    String processedUrl = bookmarkUrl;
                    if (processedUrl.endsWith("/")) {
                        processedUrl = processedUrl.substring(0, processedUrl.length() - 1);
                    }

                    URL mUrl = new URL();

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
                    communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                } catch (Exception e) {
                    communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                if (result != null) {
                    if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
                    try {
                        JSONObject jsonObject = new JSONObject(result);
                        boolean error = false;
                        try {
                            int errorCode = -1;
                            errorCode = jsonObject.getInt("error");
                            if (errorCode != -1 && errorCode != 0) error = true;
                        } catch (Exception ignore) { }

                        if (error) {
                            communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                            return;
                        }

                        JSONArray jsonDataArray = jsonObject.getJSONArray("data");
                        JSONObject jsonDataObject = jsonDataArray.getJSONObject(0);
                        String rurl = jsonDataObject.getString("rurl");

                        communicator.onVeveBookmarkReceived(rurl, tabDelegate);
                        // todo process success
                    } catch (Exception e) {
                        communicator.onReceiveVeveError(bookmarkUrl, tabDelegate);
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getBookmarkUrlUtil(final String bookmarkUrl, VeveBookmarkUtilCommunicator communicator, Context context, ComponentName componentName) {
        if (ip == null) {
            communicator.onReceiveVeveError(bookmarkUrl, context, componentName);
            return;
        }

        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    String processedUrl = bookmarkUrl;
                    if (processedUrl.endsWith("/")) {
                        processedUrl = processedUrl.substring(0, processedUrl.length() - 1);
                    }

                    URL mUrl = new URL();

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
                    communicator.onReceiveVeveError(bookmarkUrl, context, componentName);
                } catch (Exception e) {
                    communicator.onReceiveVeveError(bookmarkUrl, context, componentName);
                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                if (result != null) {
                    if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
                    try {
                        JSONObject jsonObject = new JSONObject(result);
                        boolean error = false;
                        try {
                            int errorCode = -1;
                            errorCode = jsonObject.getInt("error");
                            if (errorCode != -1 && errorCode != 0) error = true;
                        } catch (Exception ignore) { }

                        if (error) {
                            communicator.onReceiveVeveError(bookmarkUrl, context, componentName);
                            return;
                        }

                        JSONArray jsonDataArray = jsonObject.getJSONArray("data");
                        JSONObject jsonDataObject = jsonDataArray.getJSONObject(0);
                        String rurl = jsonDataObject.getString("rurl");

                        communicator.onVeveBookmarkReceived(rurl, context, componentName);
                    } catch (Exception e) {
                        communicator.onReceiveVeveError(bookmarkUrl, context, componentName);
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public boolean isUrlAbsolute(GURL url) {
        String path = url.getPath();
        return (path.equals("") || path.equals("/"));
    }
}

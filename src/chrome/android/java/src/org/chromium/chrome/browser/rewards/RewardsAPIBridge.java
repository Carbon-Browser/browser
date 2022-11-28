package org.chromium.chrome.browser.rewards;

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
import org.chromium.chrome.browser.rewards.RewardObject;

public class RewardsAPIBridge {

    public interface RewardsAPIBridgeCommunicator {
        void onRewardsReceived(ArrayList<RewardObject> rewards);

        void onReceiveRewardsError();
    }

    public interface RewardsRedeemCommunicator {
        void onRewardRedeemed();

        void onRewardRedeemError();
    }

    private static final String API_BASE_URL =
    private static final String API_ACCESS_KEY = 

    public boolean isLoggedIn = false;
    public int balance = 0;

    private String deviceID = null;
    private String tempDeviceID = null;

    public interface ReviewInterface {
        void showRatingPopup();
    }

    private static RewardsAPIBridge sRewardsAPIBridge;

    private SharedPreferences mPrefs;

    //private static final String KEY_INSTALL_DATE = "install_date";
    private static final String KEY_LAUNCH_TIMES = "launch_times";
    private static final String KEY_OPT_OUT = "rating_dialog_opt_out";
    private static final String KEY_ASK_LATER_DATE = "time_since_last_rating_dialog";

    //private static int mInstallDate;
    private static int mLaunchTimes;
    private static boolean mOptOut;

    private ReviewInterface mReviewInterface;

    private int TYPE_REGISTER = 0;
    private int TYPE_LOGIN = 1;

    private int TYPE_ADS = 0;
    private int TYPE_SEARCH = 1;

    public RewardsAPIBridge() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = mPrefs.edit();

        // if (mPrefs.getInt(KEY_INSTALL_DATE, 0) == 0) {
        //     editor.putInt(KEY_INSTALL_DATE, System.currentTimeMillis());
        // }

        // Increment launch times
        int launchTimes = mPrefs.getInt(KEY_LAUNCH_TIMES, 0);
        launchTimes = launchTimes + 1;
        editor.putInt(KEY_LAUNCH_TIMES, launchTimes);

        editor.apply();

        // mInstallDate = mPrefs.getInt(KEY_INSTALL_DATE, 0);
        mLaunchTimes = mPrefs.getInt(KEY_LAUNCH_TIMES, 0);
        mOptOut = mPrefs.getBoolean(KEY_OPT_OUT, false);

        if (isLoggedIn) return;

        // device id = null means user needs to register
        deviceID = mPrefs.getString("rewards_api_device_id", null);
        if (deviceID == null) {
            tempDeviceID = UUID.randomUUID().toString().replace("-", "");
            loginOrRegister(API_BASE_URL + "/v1/onboarding/users/register/autoUserOnboarding", TYPE_REGISTER);
        } else {
            loginOrRegister(API_BASE_URL + "/v1/users/login", TYPE_LOGIN);
        }
    }

    public static RewardsAPIBridge getInstance() {
        if (sRewardsAPIBridge == null) {
            sRewardsAPIBridge = new RewardsAPIBridge();
        }

        return sRewardsAPIBridge;
    }

    public void getAllRewards(RewardsAPIBridgeCommunicator communicator) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(API_BASE_URL + "/v1/rewards/getAllRewards");

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setConnectTimeout(8000);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("GET");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("platform", "mobile");
                    conn.setRequestProperty("os", "android");
                    conn.setRequestProperty("device-id", deviceID);
                    conn.setRequestProperty("session-id", deviceID);
                    conn.setRequestProperty("api-access", API_ACCESS_KEY);

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
                    communicator.onReceiveRewardsError();
                } catch (Exception e) {
                    communicator.onReceiveRewardsError();
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
                        int code = jsonObject.getInt("code");
                        if (code == 200) {
                            ArrayList<RewardObject> mRewards = new ArrayList<>();
                            JSONArray jsonArray = jsonObject.getJSONArray("result");
                            for (int i = 0; i < jsonArray.length(); i++) {
                                JSONObject row = jsonArray.getJSONObject(i);

                                JSONObject rewardValueObj = row.getJSONObject("reward_value");

                                String imageUrl = row.getString("image_url");
                                String name = row.getString("name");
                                int valueDollar = rewardValueObj.getInt("monetary");
                                int valuePoints = rewardValueObj.getInt("points");
                                String id = row.getString("_id");

                                RewardObject mRewardObject = new RewardObject(imageUrl, name, valueDollar, valuePoints, id);
                                mRewards.add(mRewardObject);
                            }

                            communicator.onRewardsReceived(mRewards);
                        }
                    } catch (Exception e) {
                        communicator.onReceiveRewardsError();
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void logout() {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(API_BASE_URL + "/v1/users/logout");

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setConnectTimeout(8000);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("platform", "mobile");
                    conn.setRequestProperty("os", "android");
                    conn.setRequestProperty("device-id", deviceID);
                    conn.setRequestProperty("session-id", deviceID);
                    conn.setRequestProperty("api-access", API_ACCESS_KEY);

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
                        int code = jsonObject.getInt("code");
                        if (code == 200) {

                        }
                    } catch (Exception e) { }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void loginOrRegister(final String url, final int type) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(url);

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setConnectTimeout(8000);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("platform", "mobile");
                    conn.setRequestProperty("os", "android");
                    conn.setRequestProperty("device-id", type == TYPE_LOGIN ? deviceID : tempDeviceID);
                    conn.setRequestProperty("session-id", type == TYPE_LOGIN ? deviceID : tempDeviceID);
                    conn.setRequestProperty("api-access", API_ACCESS_KEY);

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
                        int code = jsonObject.getInt("code");
                        if (code == 200) {
                            JSONObject jsonResultObject = jsonObject.getJSONObject("result");

                            if (type == TYPE_REGISTER) {
                                SharedPreferences.Editor editor = mPrefs.edit();
                                editor.putString("rewards_api_device_id", tempDeviceID);
                                editor.apply();
                                isLoggedIn = true;
                            } else if (type == TYPE_LOGIN) {
                                isLoggedIn = true;

                                JSONObject jsonWalletObj = jsonResultObject.getJSONObject("wallet");
                                JSONObject jsonDailyObj = jsonWalletObj.getJSONObject("daily_reward_balance");
                                JSONObject jsonTotalObj = jsonWalletObj.getJSONObject("total_reward_balance");
                                int dailyPoints = jsonDailyObj.getInt("points");
                                int totalPoints = jsonTotalObj.getInt("points");

                                mPrefs.edit().putInt("total_credit_balance", totalPoints).apply();
                                mPrefs.edit().putInt("credits_earned_today", dailyPoints).apply();
                            }

                            if (getUserId() == null) {
                                String userId = jsonResultObject.getString("_id");
                                setUserId(userId);
                            }
                        }
                    } catch (Exception e) {

                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void logUserActivity(final int type, final int amount) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(API_BASE_URL + "/v1/users/logUserActivity");

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setConnectTimeout(8000);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("platform", "mobile");
                    conn.setRequestProperty("os", "android");
                    conn.setRequestProperty("device-id", deviceID);
                    conn.setRequestProperty("session-id", deviceID);
                    conn.setRequestProperty("api-access", API_ACCESS_KEY);

                    String jsonInputStringSearch = "{\"user_activity_log\": {\"search\": " + amount + "}}";
                    String jsonInputStringAds = "{\"user_activity_log\": {\"ads_blocked\": " + amount + "}}";

                    String jsonInputString = type == TYPE_ADS ? jsonInputStringAds : jsonInputStringSearch;

                    try(OutputStream os = conn.getOutputStream()) {
                        byte[] input = jsonInputString.getBytes("utf-8");
                        os.write(input, 0, input.length);
                    }

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
                        int code = jsonObject.getInt("code");
                        if (code == 200) {
                            /*if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

                            int nCredits = mPrefs.getInt("total_credit_balance", 0);
                            // int nCreditsToday = mPrefs.getInt("credits_earned_today", 0);

                            SharedPreferences.Editor editor = mPrefs.edit();
                            editor.putInt("total_credit_balance", nCredits + 1);
                            // editor.putInt("credits_earned_today", nCreditsToday + 1);
                            editor.apply();*/
                        }
                    } catch (Exception e) {}
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void redeemReward(String country, String email, String rewardId, final RewardsRedeemCommunicator communicator) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(API_BASE_URL + "/v1/rewards/redeemReward");

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setConnectTimeout(8000);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("platform", "mobile");
                    conn.setRequestProperty("os", "android");
                    conn.setRequestProperty("device-id", deviceID);
                    conn.setRequestProperty("session-id", deviceID);
                    conn.setRequestProperty("api-access", API_ACCESS_KEY);

                    String bodyString = "{"+
                      "\"reward_id\":" + "\"" + rewardId + "\"," +
                      "\"user_id\":" + "\"" + getUserId() + "\"," +
                      "\"email\":" + "\"" + email + "\"," +
                      "\"location\":" + "\"" + country + "\"" +
                    "}";

                    try(OutputStream os = conn.getOutputStream()) {
                        byte[] input = bodyString.getBytes("utf-8");
                        os.write(input, 0, input.length);
                    }

                    // handle the response
                    int status = conn.getResponseCode();
                    if (status != 200) {
                        throw new IOException("Post failed with error code " + status + ". Message: " + conn.getResponseMessage());
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
                    communicator.onRewardRedeemError();
                } catch (Exception e) {
                    communicator.onRewardRedeemError();
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
                        int code = jsonObject.getInt("code");
                        if (code == 200) {
                            communicator.onRewardRedeemed();
                        }
                    } catch (Exception e) {
                        communicator.onRewardRedeemError();
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void setupRewardsInterface(ReviewInterface reviewInterface) {
        mReviewInterface = reviewInterface;
    }

    public String getUserId() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        return mPrefs.getString("rewards_user_id", null);
    }

    public void setUserId(String id) {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        mPrefs.edit().putString("rewards_user_id", id).apply();
    }

    public void logSearch() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        Amplitude.getInstance().logEvent("search_event");

        int nSearches = getSearches() + 1;
        mPrefs.edit().putInt("searches_count", nSearches).apply();

        if (nSearches % 10 == 0) {
            logUserActivity(TYPE_SEARCH, 10);
        }
    }

    public void logAdBlocked() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        maybeAskForRating();

        int nAdsBlocked = getAdsBlocked() + 1;
        mPrefs.edit().putLong("ads_blocked_count", nAdsBlocked).apply();

        if (nAdsBlocked % 20 == 0) {
            logUserActivity(TYPE_ADS, 20);
        }
    }

    public int getAdsBlocked() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        int nAdsBlocked = Long.valueOf(mPrefs.getLong("ads_blocked_count", 0)).intValue();
        return nAdsBlocked;
    }

    public int getSearches() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        maybeAskForRating();

        int nSearches = mPrefs.getInt("searches_count", 0);
        return nSearches;
    }

    public int getCreditsEarnedToday() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        int nCredits = mPrefs.getInt("credits_earned_today", 0);
        return nCredits;
    }

    public int getTotalCreditBalance() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        int nCredits = mPrefs.getInt("total_credit_balance", 0);
        return nCredits;
    }

    private void maybeAskForRating() {
        if (mOptOut) return;

        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        int timestamp = mPrefs.getInt(KEY_ASK_LATER_DATE, 0);
        int timeSinceLastDialog =  Long.valueOf(System.currentTimeMillis()).intValue() - timestamp;

        // TODODO change to 0 if using ads blocked as a parameter
        int nAdsBlocked = getAdsBlocked();

        boolean doesMeetTimeCriteria = (timestamp == 0 || timeSinceLastDialog > 216000000);

        if ((nAdsBlocked > 20 && doesMeetTimeCriteria) || (mLaunchTimes >= 3 && doesMeetTimeCriteria)) {
            SharedPreferences.Editor editor = mPrefs.edit();
            editor.putInt(KEY_ASK_LATER_DATE, Long.valueOf(System.currentTimeMillis()).intValue());
            editor.apply();

            if (mReviewInterface != null)
                mReviewInterface.showRatingPopup();
        }
    }
}

// Copyright 2019 Hydris Apps Ltd. All rights reserved.

package org.chromium.chrome.browser.suggestions.speeddial;

import android.content.SharedPreferences;

import java.util.Date;
import java.util.List;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import org.chromium.base.ContextUtils;

/**
 * Data handler for speed dials
 */
public class SpeedDialDatabase {

    private SharedPreferences mPrefs;

    private static final String TITLE_KEY = "SPEED_DIAL_TITLE";
    private static final String URL_KEY = "SPEED_DIAL_URL";
    private static final String IMAGE_URL_KEY = "SPEED_DIAL_IMAGE_URL";
    private static final String POSITION_KEY = "SPEED_DIAL_POSITION";
    private static final String SPONSORED_KEY = "SPEED_DIAL_SPONSORED";
    private static final String DEFAULT_KEY = "SPEED_DIAL_DEFAULT";
    private static final String SPEED_DIAL_COUNT_KEY = "SPEED_DIAL_COUNT";
    private static final String SPEED_DIAL_INITIALISED = "SPEED_DIAL_INITIALISED";
    private static final String IMPRESSION_URL_KEY = "SPEED_DIAL_IMPR_URL";

    public SpeedDialDatabase() {
         mPrefs = ContextUtils.getAppSharedPreferences();

         boolean isInitialised = mPrefs.getBoolean(SPEED_DIAL_INITIALISED, false);
         if (!isInitialised) writeDefaultSpeedDials();
    }

    // defaults
    private static final String[] defaultTitles = {
        "Bing",       // 1
        "Google",     // 2
        "YouTube",    // 3
        "ESPN",       // 4
    };

    private static final String[] defaultUrls = {
        "https://www.bing.com/",       // 1
        "https://www.google.com/",     // 2
        "https://www.youtube.com/",    // 3
        "https://www.espn.com/",       // 4
    };

    public void writeDefaultSpeedDials() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = mPrefs.edit();

        // 10 speed dials by default
        editor.putInt(SPEED_DIAL_COUNT_KEY, 14);

        int defaultCounter = 0;
        for (int i = 0; i != 14; i++) {
            // monetize 2, 4, 6, 7, 8, 9
            if (i == 2 || i == 4 || i == 6 || i == 7 || i == 8 || i == 9 || i > 9) {
                // all we're doing here is basically noting the positions
                // of the sponsored speed dials

                // speed dial is monetized
                editor.putString(TITLE_KEY+i, null);
                // write the url
                editor.putString(URL_KEY+i, null);
                // write the position
                editor.putInt(POSITION_KEY+i, i);
                // write sponsored
                editor.putBoolean(SPONSORED_KEY+i, true);
                // write default
                editor.putBoolean(DEFAULT_KEY+i, false);
                // write image url
                editor.putString(IMAGE_URL_KEY+i, null);
                // write impression url
                editor.putString(IMPRESSION_URL_KEY+i, null);
            } else {
                // write default speed dial

                // write the title
                editor.putString(TITLE_KEY+i, defaultTitles[defaultCounter]);
                // write the url
                editor.putString(URL_KEY+i, defaultUrls[defaultCounter]);
                // write the position
                editor.putInt(POSITION_KEY+i, i);
                // write not sponsored
                editor.putBoolean(SPONSORED_KEY+i, false);
                // write default
                editor.putBoolean(DEFAULT_KEY+i, true);
                // write image url
                editor.putString(IMAGE_URL_KEY+i, null);
                // write impression url
                editor.putString(IMPRESSION_URL_KEY+i, null);

                defaultCounter++;
            }
        }

        // record that speed dials have been initialised
        editor.putBoolean(SPEED_DIAL_INITIALISED, true);

        editor.apply();
    }

    /**
    * Get speed dials with pre-defined defaults
    */
    public List<SpeedDialDataItem> getSpeedDials() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        List<SpeedDialDataItem> suggestions = new ArrayList<>();

        int nSpeedDials = mPrefs.getInt(SPEED_DIAL_COUNT_KEY, 14);

        for (int i = 0; i != nSpeedDials; i++) {
            // User defined speed dial
            String title = mPrefs.getString(TITLE_KEY+i, null);
            String url = mPrefs.getString(URL_KEY+i, null);
            int position = mPrefs.getInt(POSITION_KEY+i, i);

            boolean isSponsored = mPrefs.getBoolean(SPONSORED_KEY+i, false);
            boolean isDefault = mPrefs.getBoolean(DEFAULT_KEY+i, false);

            String imageUrl = mPrefs.getString(IMAGE_URL_KEY+i, null);

            String impressionUrl = mPrefs.getString(IMPRESSION_URL_KEY+i, null);

            suggestions.add(new SpeedDialDataItem(title, url, position, isSponsored, isDefault, imageUrl, impressionUrl));
        }

        return suggestions;
    }

    /**
    * Set a speed dial
    */
    public void setSpeedDial(String title, String url, int position, boolean isSponsored, boolean isDefault,
          String imageUrl, String impressionUrl) {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        SharedPreferences.Editor editor = mPrefs.edit();

        editor.putString(TITLE_KEY+position, title);
        editor.putString(URL_KEY+position, url);
        editor.putInt(POSITION_KEY+position, position);
        editor.putBoolean(SPONSORED_KEY+position, isSponsored);
        editor.putBoolean(DEFAULT_KEY+position, isDefault);
        editor.putString(IMAGE_URL_KEY+position, imageUrl);
        editor.putString(IMPRESSION_URL_KEY+position, impressionUrl);

        editor.apply();
    }

    public void moveSpeedDial(int startPos, int endPos) {
        List<SpeedDialDataItem> mSpeedDials = getSpeedDials();

        // move the tile that moved first, then re-order the tiles it effected
        SpeedDialDataItem movedSD = mSpeedDials.get(startPos);
        setSpeedDial(movedSD.title, movedSD.url, endPos, movedSD.isSponsored, movedSD.isDefault,
                movedSD.imageUrl, movedSD.impressionUrl);

        if (startPos < endPos) {
            for (int i = endPos; i != startPos; i--) {
                SpeedDialDataItem effectedSD = mSpeedDials.get(i);
                setSpeedDial(effectedSD.title, effectedSD.url, (effectedSD.position - 1), effectedSD.isSponsored,
                        effectedSD.isDefault, effectedSD.imageUrl, effectedSD.impressionUrl);
            }
        } else if (startPos > endPos){
            for (int i = endPos; i != startPos; i++) {
                SpeedDialDataItem effectedSD = mSpeedDials.get(i);

                setSpeedDial(effectedSD.title, effectedSD.url, (effectedSD.position + 1), effectedSD.isSponsored,
                        effectedSD.isDefault, effectedSD.imageUrl, effectedSD.impressionUrl);
            }
        }
    }

    public void removeSpeedDial(int position) {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        List<SpeedDialDataItem> mSpeedDials = getSpeedDials();

        SharedPreferences.Editor editor = mPrefs.edit();
        // delete the speed dial
        editor.remove(TITLE_KEY+position);
        editor.remove(URL_KEY+position);
        editor.remove(POSITION_KEY+position);
        editor.remove(SPONSORED_KEY+position);
        editor.remove(DEFAULT_KEY+position);
        editor.remove(IMAGE_URL_KEY+position);
        editor.remove(IMPRESSION_URL_KEY+position);

        mSpeedDials.remove(position);

        int nSpeedDials = mPrefs.getInt(SPEED_DIAL_COUNT_KEY, 10);
        editor.putInt(SPEED_DIAL_COUNT_KEY, (nSpeedDials - 1));

        // move all the speed dials after the deleted one down 1 position
        for (int i = position; i != mSpeedDials.size(); i++) {
            SpeedDialDataItem effectedSD = mSpeedDials.get(i);
            setSpeedDial(effectedSD.title, effectedSD.url, (effectedSD.position - 1), effectedSD.isSponsored,
                    effectedSD.isDefault, effectedSD.imageUrl, effectedSD.impressionUrl);
        }

        editor.apply();
    }

    public void addSpeedDial(String title, String url) {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        int position = mPrefs.getInt(SPEED_DIAL_COUNT_KEY, 10);

        // add 1 to speed dial count
        SharedPreferences.Editor editor = mPrefs.edit();
        editor.putInt(SPEED_DIAL_COUNT_KEY, (position + 1));

        // write the title
        editor.putString(TITLE_KEY+position, title);
        // write the url
        editor.putString(URL_KEY+position, url);
        // write the position
        editor.putInt(POSITION_KEY+position, position);
        // write sponsored
        editor.putBoolean(SPONSORED_KEY+position, false);
        // write default
        editor.putBoolean(DEFAULT_KEY+position, false);
        // write image url
        editor.putString(IMAGE_URL_KEY+position, null);
        // write impression url
        editor.putString(IMPRESSION_URL_KEY+position, null);

        // save
        editor.apply();
    }

    /**
    * Clear SharedPreferences reference
    */
    public void destroy() {
        mPrefs = null;
    }
}

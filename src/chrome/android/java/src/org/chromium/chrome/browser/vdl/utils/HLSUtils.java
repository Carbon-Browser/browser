package org.chromium.chrome.browser.vdl.utils;

import java.net.MalformedURLException;
import java.net.URL;

public class HLSUtils {

    public static String getBaseUrl(URL url) {
        String urlString = url.toString();
        int index = urlString.lastIndexOf('/');
        return urlString.substring(0, ++index);
    }

    public static URL updateUrlForSubPlaylist(String subUrl, String mainUrl) {
        URL aUrl = null;
        String newUrl = !subUrl.startsWith("http") ?
                getBaseUrl(getURL(mainUrl)) + subUrl :
                subUrl;

        try {
            aUrl = new URL(newUrl);
        } catch (MalformedURLException e) {
            e.printStackTrace();
        }
        return aUrl;
    }

    private static URL getURL(String urlString) {
        try {
            return new URL(urlString);
        } catch (MalformedURLException e) {
            e.printStackTrace();
            return null;
        }
    }
}

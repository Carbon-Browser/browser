package org.chromium.chrome.browser.tab;

import android.content.Context;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Set;

public final class TLDUtils {

    private static TLDUtils sInstance;

    private static Set<String> tldSet = null;

    public static TLDUtils getInstance(Context context, int resId) {
        if (sInstance == null) {
            sInstance = new TLDUtils();
            loadTlds(context, resId);
        }
        return sInstance;
    }

    // Method to load TLDs from the resource file
    public static void loadTlds(Context context, int resId) {
        if (tldSet == null) {
            tldSet = new HashSet<>();
            try {
                InputStream is = context.getResources().openRawResource(resId);
                BufferedReader reader = new BufferedReader(new InputStreamReader(is));
                String line;
                while ((line = reader.readLine()) != null) {
                    tldSet.add(line.trim().toLowerCase());
                }
                reader.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    // Method to check if a domain has a standard TLD
    public static boolean isStandardTLD(String domain, Context context, int resId) {
        if (tldSet == null) {
            loadTlds(context, resId);
        }
        try {
          String tld = extractTLD(domain);
          return tldSet.contains(tld);
        } catch (Exception e) {
          return false;
        }
    }

    // Method to extract TLD from a domain
    private static String extractTLD(String domain) {
        try {
          int lastIndex = domain.lastIndexOf(".");
          if (lastIndex != -1) {
              return domain.substring(lastIndex + 1).toLowerCase();
          }
          return "";
        } catch (Exception e) {
          return "";
        }
    }
}

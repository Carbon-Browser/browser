package org.chromium.chrome.browser.suggestions.speeddial;

import java.util.Date;
import org.chromium.url.GURL;

/**
 * Data class that holds the site suggestion data provided by the tiles component.
 */
public class SpeedDialDataItem {

    /** Title of the suggested site. */
    public final String title;

    /** URL of the suggested site. */
    public final String url;

    public final int position;

    public final boolean isSponsored;

    public final boolean isDefault;

    public final String imageUrl;

    public final String impressionUrl;

    public SpeedDialDataItem(String title, String url, int position, boolean isSponsored,
              boolean isDefault, String imageUrl, String impressionUrl) {
        this.title = title;
        this.url = url;
        this.position = position;
        this.isSponsored = isSponsored;
        this.isDefault = isDefault;
        this.imageUrl = imageUrl;
        this.impressionUrl = impressionUrl;
    }
}

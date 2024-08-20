package org.chromium.chrome.browser.mirada;

public class ChatDataObj {

    public String id;
    public boolean isUserData;
    public String text;
    public boolean isLoading;
    public boolean shouldDisplay;

    public ChatDataObj(String id, boolean isUserData, String text, boolean isLoading, boolean shouldDisplay) {
        this.id = id;
        this.isUserData = isUserData;
        this.text = text;
        this.isLoading = isLoading;
        this.shouldDisplay = shouldDisplay;
    }
}

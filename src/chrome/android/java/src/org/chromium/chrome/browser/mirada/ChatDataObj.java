package org.chromium.chrome.browser.mirada;

public class ChatDataObj {

    public String id;
    public boolean isUserData;
    public String text;
    public boolean isLoading;

    public ChatDataObj(String id, boolean isUserData, String text, boolean isLoading) {
        this.id = id;
        this.isUserData = isUserData;
        this.text = text;
        this.isLoading = isLoading;
    }
}

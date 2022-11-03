package org.chromium.chrome.browser.ntp.news;

public class NewsDataObject {

    public String articleUrl;
    public String imageUrl;
    public String articleTitle;
    public String publisher;
    public String articleDate;
    public String imageGetterUrl;

    public NewsDataObject(String articleUrl, String imageUrl, String articleTitle, String publisher, String articleDate, String imageGetterUrl) {
        this.articleUrl = articleUrl;
        this.imageUrl = imageUrl;
        this.articleTitle = articleTitle;
        this.publisher = publisher;
        this.articleDate = articleDate;
        this.imageGetterUrl = imageGetterUrl;
    }
}

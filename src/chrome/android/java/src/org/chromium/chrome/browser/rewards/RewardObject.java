package org.chromium.chrome.browser.rewards;

public class RewardObject {

    public String id;
    public String imageUrl;
    public String name;
    public int valueDollar;
    public int valuePoints;

    public RewardObject(String imageUrl, String name, int valueDollar, int valuePoints, String id) {
      this.imageUrl = imageUrl;
      this.name = name;
      this.valueDollar = valueDollar;
      this.valuePoints = valuePoints;
      this.id = id;
    }
}

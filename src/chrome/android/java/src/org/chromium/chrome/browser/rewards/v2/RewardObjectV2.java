package org.chromium.chrome.browser.rewards.v2;

public class RewardObjectV2 {

    public String id;
    public String name;
    public String cost;
    public String amount;
    public String uri;
    public String imageUrl;
    public int inventory;

    public RewardObjectV2(String id, String name, String cost, String amount, String uri, String imageUrl, int inventory) {
      this.id = id;
      this.name = name;
      this.cost = cost;
      this.amount = amount;
      this.uri = uri;
      this.imageUrl = imageUrl;
      this.inventory = inventory;
    }
}

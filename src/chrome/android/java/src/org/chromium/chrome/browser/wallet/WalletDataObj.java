package org.chromium.chrome.browser.wallet;

import android.graphics.drawable.Drawable;
import wallet.core.jni.CoinType;

public class WalletDataObj {

    public String network;
    public String address;

    public WalletDataObj(String network, String address) {
        this.network = network;
        this.address = address;
    }
}

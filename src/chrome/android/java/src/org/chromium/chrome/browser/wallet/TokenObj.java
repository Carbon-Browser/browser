package org.chromium.chrome.browser.wallet;

import android.graphics.drawable.Drawable;
import wallet.core.jni.CoinType;

public class TokenObj {

    public CoinType coinType;
    public String name;
    public String usdValue;
    public String balance;
    public String ticker;

    public String chainName; // erc/bep

    public boolean isCustomType = false;

    public String iconUrl;
    public String chain; // contract address
    // public String decimals;

    public TokenObj(CoinType coinType, String iconUrl, String name, String usdValue, String balance, String ticker, String chainName) {
        this.coinType = coinType;
        this.iconUrl = iconUrl;
        this.name = name;
        this.usdValue = usdValue;
        this.balance = balance;
        this.ticker = ticker;
        this.chainName = chainName;
    }

    public TokenObj(boolean isCustomType, String iconUrl, String name, String usdValue, String balance, String ticker, String chain, String chainName/*, String decimals*/) {
        this.isCustomType = isCustomType;
        this.chain = chain;
        this.chainName = chainName;

        this.iconUrl = iconUrl;
        this.name = name;
        this.usdValue = usdValue;
        this.balance = balance;
        this.ticker = ticker;

        // this.decimals = decimals;
    }
}

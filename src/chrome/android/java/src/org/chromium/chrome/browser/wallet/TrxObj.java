package org.chromium.chrome.browser.wallet;

import android.graphics.drawable.Drawable;
import wallet.core.jni.CoinType;

public class TrxObj {

    public String coinName;
    public int drawableIcon;
    public String iconUrl;
    public String tokenPriceUsd;
    public String tokenAmount;
    public String date;
    public String tokenTicker;

    // links to etherscan/bscscan
    public String externalUrl;

    // token contract address
    public String contractAddress;

    // has the trx been sent
    public boolean hasBeenSent = false;

    // eg bep20/erc20
    public String chainType;

    // recipient address
    public String recipient;

    // 1 = pending 2 = cancelled 3 = complete
    public int status;

    public int coinType;

    public String gas;

    public TrxObj(int drawableIcon, String iconUrl, String tokenPriceUsd, String tokenAmount, String date, String externalUrl,
            String contractAddress, boolean hasBeenSent, String chainType, String recipient, int status, String coinName,
            int coinType, String gas, String tokenTicker) {
        this.drawableIcon = drawableIcon;
        this.iconUrl = iconUrl;
        this.tokenPriceUsd = tokenPriceUsd;
        this.tokenAmount = tokenAmount;
        this.date = date;
        this.externalUrl = externalUrl;
        this.contractAddress = contractAddress;
        this.hasBeenSent = hasBeenSent;
        this.chainType = chainType;
        this.recipient = recipient;
        this.status = status;
        this.coinName = coinName;
        this.coinType = coinType;
        this.gas = gas;

        this.tokenTicker = tokenTicker;
    }
}

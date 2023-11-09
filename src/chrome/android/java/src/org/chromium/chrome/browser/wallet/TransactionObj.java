package org.chromium.chrome.browser.wallet;

import android.graphics.drawable.Drawable;
import wallet.core.jni.CoinType;

public class TransactionObj {

    public String timestamp;
    public String amount;
    public String hash;
    public String recipientAddress;
    public String senderAddress;
    public String tokenSymbol;

    public String contractAddress;

    public boolean isSender;

    public String tokenIconUrl;
    public String chainType; // ERC20/BEP20
    public String tokenPriceUsd;
    public String gas;
    public String coinName;

    public TransactionObj(String timestamp, String amount, String hash, String recipientAddress, String senderAddress, String contractAddress, String tokenSymbol, String gas) {
        this.timestamp = timestamp;
        this.amount = amount;
        this.hash = hash;
        this.recipientAddress = recipientAddress;
        this.senderAddress = senderAddress;
        this.contractAddress = contractAddress;
        this.tokenSymbol = tokenSymbol;
        this.gas = gas;
    }

    public TransactionObj(String timestamp, String amount, String hash, String recipientAddress, String senderAddress, String contractAddress, String tokenSymbol,
                boolean isSender, String tokenIconUrl, String chainType, String tokenPriceUsd, String gas, String coinName) {
        this.timestamp = timestamp;
        this.amount = amount;
        this.hash = hash;
        this.recipientAddress = recipientAddress;
        this.senderAddress = senderAddress;
        this.contractAddress = contractAddress;
        this.tokenSymbol = tokenSymbol;

        this.isSender = isSender;

        this.tokenIconUrl = tokenIconUrl;
        this.chainType = chainType;
        this.tokenPriceUsd = tokenPriceUsd;
        this.gas = gas;
        this.coinName = coinName;
    }
}

package org.chromium.chrome.browser.wallet;

import android.graphics.drawable.Drawable;
import wallet.core.jni.CoinType;

public class TransactionObj {

    public String timestamp;
    public String amount;
    public String hash;
    public String recipientAddress;
    public String senderAddress;

    public String contractAddress;

    public TransactionObj(String timestamp, String amount, String hash, String recipientAddress, String senderAddress, String contractAddress) {
        this.timestamp = timestamp;
        this.amount = amount;
        this.hash = timestamp;
        this.recipientAddress = recipientAddress;
        this.senderAddress = senderAddress;
        this.contractAddress = contractAddress;
    }
}

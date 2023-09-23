package org.chromium.chrome.browser.wallet;

import java.util.ArrayList;

public interface WalletDatabaseInterface {

  void onTransactionsReceived();

  void onTokenInfoReceived();
}

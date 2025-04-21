package org.chromium.chrome.browser.wallet;

import java.util.ArrayList;

public interface WalletInterface {

  void onTabChanged(String tabtitle);

  void onNavigateImportWallet();

  void onNavigateCreateWallet();

  void onNavigateMain();

  void onNavigateReceive(TokenObj tokenObj);

  void onNavigateSend(TokenObj tokenObj);

  void onNavigateCreatePin();

  void onPinCreated(String pinCode);

  void onPinVerified(String pinCode);

  void onDeleteWallet();

  void onMnemonicImported(String mnemonic);

  void onNavigateVerifyTrx(String mCoinName, int mCoinIcon, String mCoinIconUrl, String mCoinTicker, String mCoinBalance, int mCoinType,
          String mRecipientAddress, String mAmount, String mGas, String mCoinUSDValue, String mContractAddress, String mChainType, String decimals);

  void onTrxVerified();

  void onUpdatedBalance(String tokenSymbol, String balance);

  void onReceivedCustomTokenPrice(String contractAddress, double price);

  void onReceiveTokenPrice(String tokenSymbol, double price);

  void sendTransaction(TransactionCallback callback);

  void checkGasPrice(String gas, ConfigureTrxCallback callback, int coinType);

  void onReceivedTransactions(ArrayList<TransactionObj> list);

  void onReceivedNonce(String tokenSymbol, String nonce);

  ArrayList<TokenObj> getTokenList();

  ArrayList<TransactionObj> getTrxList();

  String[] getMnemonic();

  void onNavigateTransactionView(TransactionObj trxObj);

  void onNavigatePreferences();

  String getWalletValue();

  void onNavigateCustomTokens();

  void onNavigateViewSeed();

  void onNavigateAddCustomToken();

  void exitWallet();

  void onReload();

  void getTokenGasEstimate(String amount, String tokenType, String recipientAddress, String contractAddress, ConfigureTrxCallback callback);
}

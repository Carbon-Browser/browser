package org.chromium.chrome.browser.wallet;

public interface ConfigureTrxCallback {

  void onCheckGasResult(String result);

  void onGasEstimateReceived(String result);
}

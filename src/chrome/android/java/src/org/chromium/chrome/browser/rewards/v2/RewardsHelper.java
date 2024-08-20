package org.chromium.chrome.browser.rewards.v2;

import org.chromium.base.ThreadUtils;

import carbonrewards.Carbonrewards;
import android.content.Context;
import android.content.SharedPreferences;
import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public final class RewardsHelper {

    private static RewardsHelper sInstance;
    private carbonrewards.Client client;
    private ExecutorService executorService = Executors.newCachedThreadPool();

    private volatile boolean hasSetWallet = false; // volatile to ensure visibility across threads

    public RewardsHelper(Context context) {
       client = Carbonrewards.carbonClient();
       // client.setDevMode();

       // Run the wallet setup asynchronously
       executorService.submit(() -> {
           try {
              client.walletFromPrivateKey(getRewardsKey(context));
              hasSetWallet = true;
           } catch (Exception ignore) {}
       });
    }

    public void bindInstance(RewardsHelper instance) {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = instance;
        }
    }

    public static RewardsHelper getInstance(Context context) {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new RewardsHelper(context);
        }
        return sInstance;
    }

    public CompletableFuture<String> getAllRewardsAsync() {
        return CompletableFuture.supplyAsync(() -> {
            try {
                return client.fetchRewardsString();
            } catch (Exception e) {
                return "";
            }
        }, executorService);
    }

    public CompletableFuture<String> getBalanceAsync() {
        return CompletableFuture.supplyAsync(() -> {
            long startTime = System.currentTimeMillis();
            long timeout = TimeUnit.SECONDS.toMillis(10); // 10 seconds timeout

            // Wait until hasSetWallet is true or timeout occurs
            while (!hasSetWallet && (System.currentTimeMillis() - startTime) < timeout) {
                try {
                    Thread.sleep(100); // Sleep for a short duration to avoid busy waiting
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return "0";
                }
            }

            if (!hasSetWallet) {
                return "0"; // Return "0" if wallet is not set within the timeout
            }

            try {
                if (client == null) return "0";
                return client.fetchBalance();
            } catch (Exception e) {
                return "0";
            }
        }, executorService);
    }

    public CompletableFuture<Void> logEventAsync(String event, String value) {
        return CompletableFuture.runAsync(() -> {
            try {
                client.sendSignedEvent(event, value);
            } catch (Exception ignore) {}
        }, executorService);
    }

    private String getRewardsKey(Context context) {
        SharedPreferences mSharedPreferences = new EncryptSharedPreferences(context).getSharedPreferences();
        String pkString = mSharedPreferences.getString("REWARDS_MNEMONIC_KEY", "");

        if (pkString.isEmpty()) {
          try {
            client.newWallet();
            mSharedPreferences.edit().putString("REWARDS_MNEMONIC_KEY", client.exportPrivateKey()).commit();
          } catch (Exception ignore) {
            ignore.printStackTrace();
          }
        }

        return pkString;
    }
}

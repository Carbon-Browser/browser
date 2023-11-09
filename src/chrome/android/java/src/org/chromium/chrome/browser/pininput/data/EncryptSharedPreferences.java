package org.chromium.chrome.browser.pininput.data;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.EncryptedSharedPreferences.PrefKeyEncryptionScheme;
import androidx.security.crypto.EncryptedSharedPreferences.PrefValueEncryptionScheme;
import androidx.security.crypto.MasterKey;
import androidx.security.crypto.MasterKey.KeyScheme;

public class EncryptSharedPreferences {

  public static SharedPreferences sharedPreferences;

  public SharedPreferences getSharedPreferences() {
      return sharedPreferences;
  }

  public EncryptSharedPreferences(Context context) {
      try {
        MasterKey masterKey = new MasterKey.Builder(context)
              .setKeyScheme(KeyScheme.AES256_GCM)
              .build();
          sharedPreferences = EncryptedSharedPreferences.create(
              context,
              "encrypted_data",
              masterKey,
              PrefKeyEncryptionScheme.AES256_SIV,
              PrefValueEncryptionScheme.AES256_GCM
          );
      } catch (Exception ignore ) {}
    }
}

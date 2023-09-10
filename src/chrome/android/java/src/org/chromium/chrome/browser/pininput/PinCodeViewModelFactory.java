package org.chromium.chrome.browser.pininput;

import android.content.SharedPreferences;
// import androidx.lifecycle.ViewModel;
// import androidx.lifecycle.ViewModelProvider;

public final class PinCodeViewModelFactory /*implements ViewModelProvider.Factory*/ {
   private final SharedPreferences sharedPreferences;

   // public ViewModel create(Class modelClass) {
   //    return (ViewModel)(new PinCodeViewModel(this.sharedPreferences));
   // }

   public PinCodeViewModelFactory(SharedPreferences sharedPreferences) {
      super();
      this.sharedPreferences = sharedPreferences;
   }
}

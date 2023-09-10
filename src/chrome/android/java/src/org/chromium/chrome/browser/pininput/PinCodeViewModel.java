package org.chromium.chrome.browser.pininput;

import android.content.SharedPreferences;

public class PinCodeViewModel {

   // private final MutableLiveData pinCode;
   // private final MutableLiveData securedPinCode;
   private final NumPadListener numPadListener;
   private final SharedPreferences sharedPreferences;
   public static final String PIN_CODE_KEY = "PIN_CODE_KEY";

   public final String getPinCode() {
      return "";
   }
   //
   // public final MutableLiveData getSecuredPinCode() {
   //    return this.securedPinCode;
   // }

   public final NumPadListener getNumPadListener() {
      return this.numPadListener;
   }

   public final SharedPreferences getSharedPreferences() {
      return this.sharedPreferences;
   }

   public PinCodeViewModel(SharedPreferences sharedPreferences) {
      super();
      this.sharedPreferences = sharedPreferences;
      // this.pinCode = new MutableLiveData();
      // this.securedPinCode = new MutableLiveData();
      this.numPadListener = new NumPadListener() {
         @Override
         public void onNumberClicked(char number) {
            // String var10000 = (String)PinCodeViewModel.this.getPinCode().getValue();
            // if (var10000 == null) {
            //    var10000 = "";
            // }
            //
            // String existingPinCode = var10000;
            // String newPassCode = existingPinCode + number;
            // PinCodeViewModel.this.getPinCode().postValue(newPassCode);
            // if (newPassCode.length() == 6) {
            //    SharedPreferences.Editor var4 = PinCodeViewModel.this.getSharedPreferences().edit();
            //    int var6 = false;
            //    var4.putString("PIN_CODE_KEY", newPassCode);
            //    var4.apply();
            //    String pinCodeInSharedPreference = PinCodeViewModel.this.getSharedPreferences().getString("PIN_CODE_KEY", "");
            //    PinCodeViewModel.this.getSecuredPinCode().postValue(pinCodeInSharedPreference);
            //    PinCodeViewModel.this.getPinCode().postValue("");
            // }

         }

         @Override
         public void onEraseClicked() {
            String var10000;
            label11: {
               var10000 = ""; //(String)PinCodeViewModel.this.getPinCode().getValue();
               if (var10000 != null) {
                  // var10000 = StringsKt.dropLast(var10000, 1);
                  if (var10000 != null) {
                     break label11;
                  }
               }

               var10000 = "";
            }

            String droppedLast = var10000;
            // PinCodeViewModel.this.getPinCode().postValue(droppedLast);
         }
      };
   }
}

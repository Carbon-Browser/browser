package org.chromium.chrome.browser.pininput;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;
import org.chromium.chrome.browser.wallet.PinCodeInterface;
import java.util.HashMap;
import org.chromium.chrome.R;
import android.os.Looper;
import android.os.Handler;
import org.chromium.chrome.browser.wallet.WalletActivity;
import org.chromium.chrome.browser.wallet.WalletInterface;
import org.chromium.ui.widget.Toast;

import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;

public final class PinCodeFragment extends Fragment implements PinCodeInterface {

   public PinCodeFragment() {}

   private static boolean isCreatePin = true;

   private static String mPinCode = "";

   private static String mCode = "";
   private static String mTempCode = "";

   private static View mDot1;
   private static View mDot2;
   private static View mDot3;
   private static View mDot4;
   private static View mDot5;
   private static View mDot6;

   private static boolean mIsVerifyTrx = false;

   @Override
   public void onPinCodePressed(String code) {
     if (mCode.length() == 6) return;

     mCode = mCode + code;
     onCodeChanged();

     doVibrate();
   }

   @Override
   public void onDeleteCodePressed() {
     if (mCode.length() == 0) return;

     mCode = mCode.substring(0, mCode.length() - 1);
     onCodeChanged();

     doVibrate();
   }

   private void doVibrate() {
      try {
        Vibrator vibrator = (Vibrator) getActivity().getSystemService(Context.VIBRATOR_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // For newer versions
            VibrationEffect effect = VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE); // 1000 milliseconds = 1 second
            vibrator.vibrate(effect);
        } else {
            // For older versions
            vibrator.vibrate(100); // 1000 milliseconds = 1 second
        }
      } catch ( Exception e ) {}
   }

   private void onCodeChanged() {
     try {
        mDot1.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 1 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
        mDot2.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 2 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
        mDot3.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 3 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
        mDot4.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 4 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
        mDot5.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 5 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
        mDot6.setBackground(mDot1.getContext().getDrawable(mCode.length() >= 6 ? R.drawable.oval_shape : R.drawable.oval_shape_transparent_stroke));
     } catch (Exception ignore) { }

     try {
        if (mCode.length() >=6) {
          final Handler handler = new Handler(Looper.getMainLooper());
          handler.postDelayed(new Runnable() {
              @Override
              public void run() {
                  try {
                    if (isCreatePin) {
                        if (mTempCode == "") {
                            mTempCode = mCode;
                            mCode = "";
                            onCodeChanged();
                            Toast.makeText(mDot1.getContext(), "Please confirm your pin.", Toast.LENGTH_SHORT).show();
                        } else {
                          if (mCode.equals(mTempCode)) {
                              SharedPreferences mSharedPreferences = new EncryptSharedPreferences(mDot1.getContext()).getSharedPreferences();
                              mSharedPreferences.edit().putString("PIN_CODE_KEY", mCode).commit();

                              // SEND PIN CREATED CALL
                              ((WalletInterface) ((WalletActivity)mDot1.getContext())).onPinCreated(mCode);

                              mTempCode = mCode;
                              mCode = "";
                              onCodeChanged();
                          } else {
                              mCode = "";
                              mTempCode = "";
                              onCodeChanged();
                              Toast.makeText(mDot1.getContext(), "Pin code did not match. Please try again.", Toast.LENGTH_SHORT).show();
                          }
                        }
                    } else {
                        final SharedPreferences mSharedPreferences = new EncryptSharedPreferences(mDot1.getContext()).getSharedPreferences();
                        int attempts = mSharedPreferences.getInt("PIN_CODE_ATTEMPTS", 0);
                        int timeLock = mSharedPreferences.getInt("PIN_CODE_LOCKED", 1);
                        long timeSinceLastAttempt = System.currentTimeMillis() - mSharedPreferences.getLong("PIN_CODE_TIME_TRACK", 0);

                        // lock for 30 mins, give 5 more attempts, double time, repeat
                        long timeToLock = timeLock * 1800000 - timeSinceLastAttempt;

                        boolean isLocked = attempts >= 5 && timeSinceLastAttempt <= timeToLock;

                        if (mPinCode.equals(mCode) && !isLocked) {
                            if (mIsVerifyTrx) {
                                ((WalletInterface) ((WalletActivity)mDot1.getContext())).onTrxVerified();
                            } else {
                                ((WalletInterface) ((WalletActivity)mDot1.getContext())).onPinVerified(mCode);
                            }

                            mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();
                            mSharedPreferences.edit().putLong("PIN_CODE_TIME_TRACK", 0).commit();
                            mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();

                            mCode = "";
                            mTempCode = "";
                            onCodeChanged();
                        } else {
                            if (timeSinceLastAttempt > timeToLock || attempts < 4) {
                                if (timeSinceLastAttempt > timeToLock && attempts > 4) {
                                  attempts = 0;
                                  mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", 0).commit();
                                }

                                mSharedPreferences.edit().putLong("PIN_CODE_TIME_TRACK", System.currentTimeMillis()).commit();

                                Toast.makeText(mDot1.getContext(), ("Incorrect pin, try again. " + (5 - (attempts + 1)) + " attempts remaining.") , Toast.LENGTH_SHORT).show();

                                mSharedPreferences.edit().putInt("PIN_CODE_ATTEMPTS", attempts + 1).commit();

                                mCode = "";
                                mTempCode = "";
                                onCodeChanged();
                            } else {
                                Toast.makeText(mDot1.getContext(), ("Wallet locked. Try again in " + timeConversion(timeToLock)) , Toast.LENGTH_SHORT).show();
                            }
                        }
                    }
                  } catch (Exception ignore) {}
              }
          }, 500);
        }
     } catch (Exception ignore) { }
   }

   public String timeConversion(Long millie) {
      if (millie != null) {
          long seconds = (millie / 1000);
          long sec = seconds % 60;
          long min = (seconds / 60) % 60;
          long hrs = (seconds / (60 * 60)) % 24;
          if (hrs > 0) {
              return String.format("%02d:%02d:%02d", hrs, min, sec);
          } else {
              return String.format("%02d:%02d", min, sec);
          }
      } else {
          return null;
      }
   }

   @Override
   public View onCreateView(LayoutInflater inflater,
            ViewGroup container, Bundle savedInstanceState) {
     return inflater.inflate(R.layout.fragment_pin_code, container, false);
   }

   @Override
   public void onViewCreated(View view, Bundle savedInstanceState) {
      if (getArguments() != null) {
          mPinCode = getArguments().getString("PIN_CODE_KEY", "");
          mIsVerifyTrx = getArguments().getBoolean("IS_VERIFY_TRX_KEY", false);
          if (mPinCode != "") isCreatePin = false;
      }

      mCode = "";
      mTempCode = "";

      mDot1 = view.findViewById(R.id.dot1);
      mDot2 = view.findViewById(R.id.dot2);
      mDot3 = view.findViewById(R.id.dot3);
      mDot4 = view.findViewById(R.id.dot4);
      mDot5 = view.findViewById(R.id.dot5);
      mDot6 = view.findViewById(R.id.dot6);
   }

   // @Override
   //  public void onDestroy() {
   //      mCode = null;
   //
   //      super.onDestroy();
   //  }
}

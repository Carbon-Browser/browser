package org.chromium.chrome.browser.wallet;

import android.os.Bundle;
import androidx.fragment.app.Fragment;
import org.chromium.chrome.R;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;

import android.graphics.Color;
import android.graphics.Shader;
import android.graphics.LinearGradient;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.text.TextWatcher;
import android.text.Editable;

import android.content.ClipboardManager;
import android.content.Context;
import android.content.ClipData;

import android.content.Intent;

import com.google.android.gms.vision.Frame;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;

import wallet.core.jni.CoinType;

import androidx.core.content.ContextCompat;
import androidx.core.app.ActivityCompat;
import android.content.pm.PackageManager;
import android.Manifest;

import android.widget.FrameLayout;

import java.math.BigInteger;
import java.math.BigDecimal;

import org.chromium.ui.widget.Toast;
import java.lang.Float;

import org.json.JSONObject;
import android.view.KeyEvent;

import com.google.zxing.Result;
import me.dm7.barcodescanner.zxing.ZXingScannerView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;

import android.widget.ProgressBar;

import java.math.RoundingMode;

public class WalletSend extends Fragment implements ZXingScannerView.ResultHandler, ConfigureTrxCallback {

    private FrameLayout mCameraView;
    private TextView mGasTimeEstimate;
    private ZXingScannerView mScannerView;

    private String mCoinName;
    private int mCoinIcon;
    private String mCoinIconUrl;
    private String mCoinTicker;
    private String mCoinBalance;
    private int mCoinType;
    private String mCoinUSDValue;
    private String mContractAddress;
    private String mChainType;

    private static String mRecipientAddress;
    private static String mAmount;
    private static String mGas;

    private boolean isValid = false;

    private boolean didOverrideAmountText = false;
    private boolean didOverrideGasAmount = false;

    private boolean isCameraShown = false;

    private EditText gasEditText;

    private ProgressBar gasLoader;
    private boolean isShowingAdvanced = false;
    private TextView autoGasButton;
    private Button nextButton;

    public WalletSend() {
        super(R.layout.wallet_fragment_send);
    }

    @Override
    public void onCheckGasResult(String result) {
        // eth result in seconds
        try {
            JSONObject jsonResult = new JSONObject(result);
            // float estimate = Float.parseFloat(jsonResult.getString("result"));

            gasEditText.setText(jsonResult.getString("result"));
        } catch (Exception ignore) { }
    }

    @Override
    public void onGasEstimateReceived(String result) {
        try {
            boolean isCustomType = mContractAddress != null && !"".equals(mContractAddress) && !"null".equals(mContractAddress);

            JSONObject jsonResult = new JSONObject(result);
            String result16 = jsonResult.getString("result");

            // Remove the '0x' prefix
            result16 = result16.startsWith("0x") ? result16.substring(2) : result16;

            // Convert the hex string to a BigInteger, then to BigDecimal
            BigDecimal amountDecimal = new BigDecimal(new BigInteger(result16, 16));

            // Convert from wei to ether
            BigDecimal wei = new BigDecimal("1000000000000000000");
            amountDecimal = amountDecimal.divide(wei);

            amountDecimal = removeZerosAfterDecimal(amountDecimal, 5);

            // Do something with amountDecimal
            gasEditText.setText(amountDecimal.toPlainString());

            BigDecimal tokenDollarValue = new BigDecimal(mCoinUSDValue);
            BigDecimal limitDefault = new BigDecimal("21000");
            BigDecimal limitCustom = new BigDecimal("180000");

            BigDecimal maxFeeToken = amountDecimal.multiply(isCustomType ? limitCustom : limitDefault);
            maxFeeToken = maxFeeToken.setScale(8, RoundingMode.HALF_UP);
            BigDecimal maxFeeDollar = maxFeeToken.multiply(tokenDollarValue);
            maxFeeDollar = maxFeeDollar.setScale(3, RoundingMode.HALF_UP);

            String fuel = mChainType.equals("BEP20") ? "BSC" : "ETH";
            mGasTimeEstimate.setText("Max Network Fee: $" + maxFeeDollar.toPlainString() + " / " + maxFeeToken.toPlainString() + " " + fuel);

            isValid = true;
            gasLoader.setVisibility(View.GONE);
            nextButton.setText("GO NEXT");
        } catch (Exception e) {
            Toast.makeText(getActivity(), "Please check that all the fields are valid.", Toast.LENGTH_SHORT).show();
            gasLoader.setVisibility(View.GONE);
        }
    }

    private BigDecimal removeZerosAfterDecimal(BigDecimal number, int zerosToRemove) {
        return number.multiply(BigDecimal.TEN.pow(zerosToRemove));
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        if (getArguments() != null) {
            mCoinName = getArguments().getString("COIN_NAME_KEY", "").toLowerCase();
            mCoinIconUrl = getArguments().getString("COIN_ICON_URL_KEY");
            mCoinTicker = getArguments().getString("COIN_TICKER_KEY", "");
            mCoinBalance = getArguments().getString("COIN_BALANCE_KEY", "");
            mCoinType = getArguments().getInt("COIN_TYPE_KEY", -1);
            mCoinUSDValue = getArguments().getString("COIN_USD_VALUE", "");
            mContractAddress = getArguments().getString("COIN_CONTRACT_ADDRESS_KEY", "");
            mChainType = getArguments().getString("COIN_CHAIN_TYPE_KEY", "");
        }

        gasLoader = view.findViewById(R.id.gas_loader);

        mScannerView = new ZXingScannerView(view.getContext());

        mCameraView = view.findViewById(R.id.camera_view);

        mGasTimeEstimate = view.findViewById(R.id.gas_time_estimate);

        View qrCodeButton = view.findViewById(R.id.qr_code_button);
        qrCodeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    if (ContextCompat.checkSelfPermission(view.getContext(), Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED)
                      ActivityCompat.requestPermissions(getActivity(), new String[] {Manifest.permission.CAMERA}, 3456);

                    if (isCameraShown) {
                        mScannerView.stopCamera();
                        mCameraView.removeAllViews();
                        mCameraView.setVisibility(View.GONE);
                    } else {
                        mCameraView.setVisibility(View.VISIBLE);
                        mCameraView.addView(mScannerView);
                        mScannerView.startCamera();
                    }

                    isCameraShown = !isCameraShown;
                } catch (Exception ignore) {}
            }
        });

        ImageView tokenIcon = view.findViewById(R.id.amount_token_icon);
        Glide.with(tokenIcon)
            .load(mCoinIconUrl)
            //.thumbnail(0.05f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .fitCenter()
            // .apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
            // .transform(new RoundedCorners(valueInDp))
            .into(new CustomTarget<Drawable>() {
                @Override
                public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                    if (tokenIcon.getDrawable() == null)
                        tokenIcon.setImageDrawable(resource);
                }

                @Override
                public void onLoadCleared(@Nullable Drawable placeholder) {

                }
            });

        TextView tokenTicker = view.findViewById(R.id.amount_token_name);
        tokenTicker.setText("$"+mCoinTicker);

        nextButton = view.findViewById(R.id.button_next);
        nextButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mRecipientAddress != null) mRecipientAddress = mRecipientAddress.replace(" ", "");


                if (!isValid && !isShowingAdvanced) {
                  validate();
                } else if (mRecipientAddress == null || mRecipientAddress.equals("")
                  || mAmount == null || mAmount.equals("")
                  || mGas == null || mGas.equals("")) {
                    Toast.makeText(getActivity(), "Please check that all the fields are valid.", Toast.LENGTH_SHORT).show();
                } else {
                    ((WalletInterface) getActivity()).onNavigateVerifyTrx(mCoinName, mCoinIcon, mCoinIconUrl, mCoinTicker,
                        mCoinBalance, mCoinType, mRecipientAddress, mAmount, mGas, mCoinUSDValue, mContractAddress, mChainType);
                }
            }
        });

        EditText recipientEditText = view.findViewById(R.id.trx_recipient_address);
        gasEditText = view.findViewById(R.id.trx_gas_amount);
        EditText amountEditText = view.findViewById(R.id.trx_amount);

        recipientEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                String text = editable.toString();

                mRecipientAddress = text;

                if (!isShowingAdvanced) {
                  isValid = false;
                  nextButton.setText("VERIFY");
                }
            }
        });


        gasEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
              if (!hasFocus) {
                 // checkGas();
              }
            }
         });

        gasEditText.setOnKeyListener(new EditText.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                // If the event is a key-down event on the "enter" button
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    // Perform action on key press
                    // checkGas();
                    return true;
                }
                return false;
            }
        });


        gasEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                String text = editable.toString();
                mGas = text;

                if (didOverrideGasAmount) {
                    didOverrideGasAmount = false;
                    return;
                }

                didOverrideGasAmount = true;

                try {
                    if (Float.parseFloat(text) > Float.parseFloat(mCoinBalance)) {
                        text = mCoinBalance;

                        gasEditText.setText(text);
                        gasEditText.setSelection(gasEditText.getText().length());
                    }

                    mGas = text;
                } catch (Exception ignore) { }
            }
        });

        amountEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                String text = editable.toString();
                mAmount = text;

                if (didOverrideAmountText) {
                    didOverrideAmountText = false;
                    return;
                }

                didOverrideAmountText = true;

                try {
                    if (Float.parseFloat(text) > Float.parseFloat(mCoinBalance)) {
                        text = mCoinBalance;

                        amountEditText.setText(text);
                        amountEditText.setSelection(amountEditText.getText().length());
                    }

                    mAmount = text;

                    if (!isShowingAdvanced) {
                      isValid = false;
                      nextButton.setText("VERIFY");
                    }
                } catch (Exception ignore) { }
            }
        });

        TextView maxAmountButton = view.findViewById(R.id.amount_max_button);
        Shader textShader = new LinearGradient(0, 0, 50, 0,
            new int[]{Color.parseColor("#FF2C07"),Color.parseColor("#FF872F")},
           null, Shader.TileMode.CLAMP);
        maxAmountButton.getPaint().setShader(textShader);

        maxAmountButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                amountEditText.setText(mCoinBalance);
            }
        });

        final Button advancedOptionsButton = view.findViewById(R.id.advanced_options_button);
        advancedOptionsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                isShowingAdvanced = !isShowingAdvanced;

                isValid = false;

                autoGasButton.setVisibility(isShowingAdvanced ? View.VISIBLE : View.GONE);

                gasEditText.setEnabled(isShowingAdvanced);

                nextButton.setText(isShowingAdvanced ? "GO NEXT" : "VERIFY");
            }
        });

        autoGasButton = view.findViewById(R.id.gas_auto_button);
        autoGasButton.getPaint().setShader(textShader);

        autoGasButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mRecipientAddress == null || mRecipientAddress.equals("")) {
                  Toast.makeText(getActivity(), "Please enter recipient address.", Toast.LENGTH_SHORT).show();
                  return;
                }
                ((WalletInterface) getActivity()).getTokenGasEstimate(mAmount, mChainType, mRecipientAddress, mContractAddress, WalletSend.this);
            }
        });
    }

    private void validate() {
        try {
            gasLoader.setVisibility(View.VISIBLE);
            ((WalletInterface) getActivity()).getTokenGasEstimate(mAmount, mChainType, mRecipientAddress, mContractAddress, WalletSend.this);
        } catch (Exception e) {}
    }

    @Override
    public void onResume() {
        super.onResume();
        // Register ourselves as a handler for scan results.
        mScannerView.setResultHandler(this);
        // Start camera on resume
        mScannerView.startCamera();
    }

    @Override
    public void onPause() {
        super.onPause();
        // Stop camera on pause
        mScannerView.stopCamera();
    }

    @Override
    public void handleResult(Result rawResult) {
        EditText recipientEditText = getActivity().findViewById(R.id.trx_recipient_address);
        recipientEditText.setText(rawResult.getText());

        try {
            mScannerView.stopCamera();
            mCameraView.removeAllViews();
            mCameraView.setVisibility(View.GONE);

            isCameraShown = !isCameraShown;
        } catch (Exception ignore) {}
    }
}

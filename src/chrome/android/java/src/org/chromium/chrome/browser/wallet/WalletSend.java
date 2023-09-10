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

    private boolean didOverrideAmountText = false;
    private boolean didOverrideGasAmount = false;

    private boolean isCameraShown = false;

    public WalletSend() {
        super(R.layout.wallet_fragment_send);
    }

    @Override
    public void onCheckGasResult(String result) {
        // eth result in seconds
        try {
            JSONObject jsonResult = new JSONObject(result);
            float seconds = Float.parseFloat(jsonResult.getString("result"));

            mGasTimeEstimate.setText("Estimated transaction time: " + (seconds/60) + " minutes");
        } catch (Exception ignore) {}
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        if (getArguments() != null) {
            mCoinName = getArguments().getString("COIN_NAME_KEY", "").toLowerCase();
            mCoinIcon = getArguments().getInt("COIN_ICON_KEY", -1);
            mCoinIconUrl = getArguments().getString("COIN_ICON_URL_KEY");
            mCoinTicker = "$" + getArguments().getString("COIN_TICKER_KEY", "");
            mCoinBalance = getArguments().getString("COIN_BALANCE_KEY", "");
            mCoinType = getArguments().getInt("COIN_TYPE_KEY", -1);
            mCoinUSDValue = getArguments().getString("COIN_BALANCE_KEY", "");
            mContractAddress = getArguments().getString("COIN_CONTRACT_ADDRESS_KEY", "");
            mChainType = getArguments().getString("COIN_CHAIN_TYPE_KEY", "");
        }

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

        View tokenIcon = view.findViewById(R.id.amount_token_icon);
        tokenIcon.setBackground(view.getContext().getResources().getDrawable(mCoinIcon));

        TextView tokenTicker = view.findViewById(R.id.amount_token_name);
        tokenTicker.setText(mCoinTicker);

        Button nextButton = view.findViewById(R.id.button_next);
        nextButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mRecipientAddress == null || mRecipientAddress.equals("")
                  || mRecipientAddress == null || mRecipientAddress.equals("")
                  || mRecipientAddress == null || mRecipientAddress.equals("")) {
                    Toast.makeText(getActivity(), "Please check that all the fields are valid.", Toast.LENGTH_SHORT).show();
                } else {
                    ((WalletInterface) getActivity()).onNavigateVerifyTrx(mCoinName, mCoinIcon, mCoinIconUrl, mCoinTicker,
                        mCoinBalance, mCoinType, mRecipientAddress, mAmount, mGas, mCoinUSDValue, mContractAddress, mChainType);
                }
            }
        });

        EditText recipientEditText = view.findViewById(R.id.trx_recipient_address);
        EditText gasEditText = view.findViewById(R.id.trx_gas_amount);
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
            }
        });


        gasEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
              if (!hasFocus) {
                 checkGas();
              }
            }
         });

        gasEditText.setOnKeyListener(new EditText.OnKeyListener() {
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                // If the event is a key-down event on the "enter" button
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    // Perform action on key press
                    checkGas();
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
    }

    private void checkGas() {
        if (mGas != null && !mGas.equals("")) {
           BigDecimal wei = new BigDecimal("1000000000000000000");

           BigDecimal gasDecimal = new BigDecimal(mGas);
           gasDecimal = gasDecimal.multiply(wei);
           ((WalletInterface) getActivity()).checkGasPrice(gasDecimal.toBigInteger().toString(), WalletSend.this, mCoinType);
        }
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

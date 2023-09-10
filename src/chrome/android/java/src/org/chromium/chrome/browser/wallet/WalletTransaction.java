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

import android.graphics.Bitmap;
import android.graphics.Color;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.qrcode.QRCodeWriter;

import org.json.JSONObject;

import android.widget.ProgressBar;

public class WalletTransaction extends Fragment implements TransactionCallback {

    private boolean isLoadingTrx = false;

    private int drawableIcon;
    private String iconUrl;
    private String tokenPriceUsd;
    private String tokenAmount;
    private String date;
    private String externalUrl;
    private String contractAddress;
    private boolean hasBeenSent;
    private String chainType;
    private String recipient;
    private String tokenTicker;
    private int status;
    private int coinType;
    private String coinName;
    private String gas;

    private boolean isTokenCustomType = false;

    public WalletTransaction() {
        super(R.layout.wallet_fragment_trx);
    }

    private ProgressBar loadingSpinner;
    private TextView errorTextView;
    private TextView statusTextView;

    @Override
    public void onTransactionResult(String result) {
        isLoadingTrx = false;
        hasBeenSent = true;
        if (loadingSpinner != null) loadingSpinner.setVisibility(View.GONE);

        try {
            JSONObject jsonResult = new JSONObject(result);

            // Handle error
            if (jsonResult.getJSONObject("error") != null) {
                status = 2;
                JSONObject errorObject = jsonResult.getJSONObject("error");
                errorTextView.setVisibility(View.VISIBLE);
                errorTextView.setText(errorObject.getString("message"));

                statusTextView.setText("Cancelled");
            } else {
                // handle success

            }
        } catch (Exception e) {}
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        if (getArguments() != null) {
            drawableIcon = getArguments().getInt("COIN_ICON_KEY", -1);
            iconUrl = getArguments().getString("COIN_ICON_URL_KEY", "");
            tokenPriceUsd = getArguments().getString("COIN_USD_VALUE", "");
            tokenAmount = getArguments().getString("TRX_AMOUNT", "");
            date = getArguments().getString("TRX_DATE", "");
            externalUrl = getArguments().getString("TRX_EXTERNAL_URL", "");
            contractAddress = getArguments().getString("COIN_CONTRACT_ADDRESS_KEY", "");
            hasBeenSent = getArguments().getBoolean("TRX_SENT_KEY", false);
            chainType = getArguments().getString("COIN_CHAIN_TYPE_KEY", "");
            recipient = getArguments().getString("TRX_RECIPIENT", "");
            tokenTicker = getArguments().getString("COIN_TICKER_KEY", "");
            coinType = getArguments().getInt("COIN_TYPE_KEY", -1);
            status = getArguments().getInt("TRX_STATUS", -1);
            coinName = getArguments().getString("COIN_NAME_KEY", "");
            gas = getArguments().getString("TRX_GAS", "");

            if (!hasBeenSent) isLoadingTrx = true;

            if (contractAddress.equals("")) isTokenCustomType = true;
        }

        if (!hasBeenSent) ((WalletInterface) getActivity()).sendTransaction(this);

        loadingSpinner = view.findViewById(R.id.progressBar);
        if (isLoadingTrx) loadingSpinner.setVisibility(View.VISIBLE);

        ImageView tokenIcon = view.findViewById(R.id.token_icon);
        if (drawableIcon != -1) {
            tokenIcon.setImageDrawable(view.getResources().getDrawable(drawableIcon));
        } else {
           // glide todo
        }

        errorTextView = view.findViewById(R.id.error_text);

        TextView tokenQuantityText = view.findViewById(R.id.header_token_quantity);
        tokenQuantityText.setText(tokenAmount + " " + tokenTicker);

        TextView tokenChainText = view.findViewById(R.id.header_token_chain_type);
        if (isTokenCustomType) tokenChainText.setText(chainType);

        Button externalButton = view.findViewById(R.id.header_external_button);
        externalButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // TODO
            }
        });

        TextView dateText = view.findViewById(R.id.trx_date);
        dateText.setText(date);

        String statusText = status == 1 ? "Pending" : status == 2 ? "Cancelled" : "Complete";
        statusTextView = view.findViewById(R.id.trx_status);
        statusTextView.setText(statusText);

        TextView recipientText = view.findViewById(R.id.trx_recipient);
        recipientText.setText(recipient);

        TextView gasText = view.findViewById(R.id.trx_gas);
        gasText.setText(gas);

        Button goBackButton = view.findViewById(R.id.button_back);
        goBackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateMain();
            }
        });
    }
}

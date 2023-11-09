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

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;

import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import java.util.Locale;
import 	java.sql.Date;

import java.text.SimpleDateFormat;
import java.math.BigInteger;
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
            if (jsonResult.has("error")) {
                status = 2;
                JSONObject errorObject = jsonResult.getJSONObject("error");
                errorTextView.setVisibility(View.VISIBLE);
                errorTextView.setText(errorObject.getString("message"));

                statusTextView.setText("Cancelled");
            } else {
                // handle success

                // add one to nonce
                String nonce =  TokenDatabase.getInstance(getActivity()).getTokenNonce(tokenTicker);
                boolean shouldStoreAsHex = false;
                BigInteger nonceBigInteger;
                if (nonce.length() == 1 && Character.isDigit(nonce.charAt(0))) {
                    nonceBigInteger = new BigInteger(nonce);
                    if (nonceBigInteger.compareTo(BigInteger.valueOf(9)) <= 0) {
                        shouldStoreAsHex = false;
                    } else {
                        shouldStoreAsHex = true;
                    }
                } else {
                    nonceBigInteger = new BigInteger(nonce, 16);
                    shouldStoreAsHex = true;
                }
                nonceBigInteger = nonceBigInteger.add(BigInteger.ONE);

                TokenDatabase. getInstance(getActivity()).setTokenNonce(tokenTicker, shouldStoreAsHex ? nonceBigInteger.toString(16) : nonceBigInteger.toString());

                JSONObject jsonObject = new JSONObject(result);
                externalUrl = chainType.equals("ERC20") ? "https://etherscan.io/tx/" : "https://bscscan.com/tx/";
                externalUrl = externalUrl + jsonObject.getString("result");
            }
        } catch (Exception e) { }
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        if (getArguments() != null) {
            iconUrl = getArguments().getString("COIN_ICON_URL_KEY", "");
            tokenPriceUsd = getArguments().getString("COIN_USD_VALUE", "");
            tokenAmount = getArguments().getString("TRX_AMOUNT", "");

            date = getArguments().getString("TRX_DATE", "");
            Date mDate = new Date(Long.parseLong(date)*1000l);
            SimpleDateFormat sdf = new SimpleDateFormat("dd/MM/yyyy", Locale.getDefault());
            date = sdf.format(mDate);

            externalUrl = getArguments().getString("TRX_EXTERNAL_URL", "");
            contractAddress = getArguments().getString("COIN_CONTRACT_ADDRESS_KEY", "");
            hasBeenSent = getArguments().getBoolean("TRX_SENT_KEY", false);
            chainType = getArguments().getString("COIN_CHAIN_TYPE_KEY", "");
            recipient = getArguments().getString("TRX_RECIPIENT", "");
            tokenTicker = getArguments().getString("COIN_TICKER_KEY", "");
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
        Glide.with(tokenIcon)
            .load(iconUrl)
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

        errorTextView = view.findViewById(R.id.error_text);

        TextView tokenQuantityText = view.findViewById(R.id.header_token_quantity);
        tokenQuantityText.setText(tokenAmount + " " + tokenTicker);

        TextView tokenChainText = view.findViewById(R.id.header_token_chain_type);
        if (isTokenCustomType) tokenChainText.setText(chainType);

        Button externalButton = view.findViewById(R.id.header_external_button);
        externalButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (externalUrl.equals("") || externalUrl == null) return;
                CustomTabActivity.showInfoPage(
                        getActivity(), externalUrl);
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

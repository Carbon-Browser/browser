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
import android.widget.LinearLayout;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.SocketTimeoutException;
import org.chromium.base.task.AsyncTask;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;

import android.view.LayoutInflater;
import java.util.ArrayList;

import android.text.TextWatcher;
import android.text.Editable;
import java.io.OutputStream;
import android.app.AlertDialog;
import android.widget.ProgressBar;
import org.chromium.ui.widget.Toast;

import org.json.JSONObject;
import org.json.JSONArray;
import android.icu.math.BigDecimal;
import java.lang.Integer;
import java.lang.Long;
import java.lang.Double;
import java.lang.Float;

public class WalletPreferencesAddCustomToken extends Fragment {

    private boolean isVerified = false;
    private ProgressBar mLoadingSpinner;
    private Button mVerifyContinueButton;

    private EditText mTokenNameEditText;
    private EditText mAddressEditText;
    private EditText mTokenSymbolEditText;

    private String tokenAddress;
    private String tokenNetwork = "BEP20";
    private String tokenName;
    private String tokenSymbol;
    private String tokenDenomination;

    private TokenObj mTokenObj;

    public WalletPreferencesAddCustomToken() {
        super(R.layout.wallet_fragment_preferences_add_custom_token);
    }

    private void sendAPIRequest(String url) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(url);

                    conn = (HttpURLConnection) mUrl.openConnection();
                    // conn.setDoOutput(true);
                    conn.setConnectTimeout(20000);
                    // conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("GET");
                    conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

                    // handle the response
                    int status = conn.getResponseCode();
                    if (status != 200) {
                        return "error";
                    } else {
                        BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                        String inputLine;
                        while ((inputLine = in.readLine()) != null) {
                            response.append(inputLine);
                        }
                        in.close();
                    }
                } catch (SocketTimeoutException timeout) {
                    // Time out, don't set a background - lazy
                    return "error";
                } catch (Exception e) {
                    return "error";
                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                if (result == null || result.equals("") || result.equals("error")) {
                    Toast.makeText(
                          getActivity(), "Could not verify token. Proceed with caution.", Toast.LENGTH_SHORT).show();

                    mTokenObj = new TokenObj(true, "", tokenName, "0.00", "0", tokenSymbol, tokenAddress, tokenNetwork/*, "1000000000000000000"*/);
                } else {
                    mTokenNameEditText.setEnabled(false);
                    mAddressEditText.setEnabled(false);
                    mTokenSymbolEditText.setEnabled(false);

                    try {
                        String chain = tokenNetwork.equals("BEP20") ? "smartchain" : "ethereum";
                        JSONObject jsonObj = new JSONObject(result);

                        mTokenNameEditText.setText(jsonObj.getString("name"));
                        mAddressEditText.setText(jsonObj.getString("id"));
                        mTokenSymbolEditText.setText(jsonObj.getString("symbol"));

                        mTokenObj = new TokenObj(true, "https://hydrisapps.com/carbon/android-resources/wallet/blockchains/" + chain + "/assets/" + jsonObj.getString("id") + "/logo.png",
                              jsonObj.getString("name"), "0.00", "0", jsonObj.getString("symbol"), jsonObj.getString("id"), tokenNetwork/*, jsonObj.getString("decimals")*/);
                    } catch (Exception ignore) { }
                }

                isVerified = true;
                mLoadingSpinner.setVisibility(View.GONE);
                mVerifyContinueButton.setEnabled(true);
                mVerifyContinueButton.setText("ADD CUSTOM TOKEN");
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mLoadingSpinner = view.findViewById(R.id.progressBar);

        mAddressEditText = view.findViewById(R.id.token_address_edittext);
        mAddressEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                tokenAddress = editable.toString();
            }
        });

        mTokenNameEditText = view.findViewById(R.id.token_name_edittext);
        mTokenNameEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                tokenName = editable.toString();
            }
        });

        mTokenSymbolEditText = view.findViewById(R.id.token_symbol_edittext);
        mTokenSymbolEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void afterTextChanged(Editable editable) {
                tokenSymbol = editable.toString();
            }
        });

        mVerifyContinueButton = view.findViewById(R.id.continue_button);
        mVerifyContinueButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if ((tokenAddress == null || tokenAddress.equals(""))
                    || (tokenName == null || tokenName.equals(""))
                    || (tokenSymbol == null || tokenSymbol.equals(""))) {

                      Toast.makeText(
                              view.getContext(), "Invalid input", Toast.LENGTH_SHORT).show();
                    return;
                }

                if (!isVerified) {
                    mVerifyContinueButton.setEnabled(false);
                    mLoadingSpinner.setVisibility(View.VISIBLE);
                    String chain = tokenNetwork.equals("BEP20") ? "smartchain" : "ethereum";
                    sendAPIRequest("https://hydrisapps.com/carbon/android-resources/wallet/blockchains/" + chain + "/assets/" + mAddressEditText.getText().toString() + "/info.json");
                } else {
                    if (mTokenObj == null) return;

                    TokenDatabase.getInstance(getActivity()).addToken(mTokenObj);

                    Toast.makeText(
                            view.getContext(), "Token added.", Toast.LENGTH_SHORT).show();

                    ((WalletInterface) getActivity()).onNavigateMain();
                }
            }
        });

        final TextView mTokenNetworkTextView = view.findViewById(R.id.token_network_text);
        View mTokenNetworkButton = view.findViewById(R.id.token_network_button);
        mTokenNetworkButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog tokenMenuDialog = new AlertDialog.Builder(v.getContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog).create();
                tokenMenuDialog.setCanceledOnTouchOutside(true);

                tokenMenuDialog.getWindow().setBackgroundDrawable(v.getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_black));

                View container = LayoutInflater.from(v.getContext()).inflate(R.layout.wallet_custom_token_chain_selector, null);

                View mEthereumButton = container.findViewById(R.id.ethereum_button);
                mEthereumButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        tokenNetwork = "ERC20";
                        mTokenNetworkTextView.setText(tokenNetwork);
                        tokenMenuDialog.dismiss();
                    }
                });

                View mSmartchainButton = container.findViewById(R.id.smartchain_button);
                mSmartchainButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        tokenNetwork = "BEP20";
                        mTokenNetworkTextView.setText(tokenNetwork);
                        tokenMenuDialog.dismiss();
                    }
                });

                tokenMenuDialog.show();

                tokenMenuDialog.setContentView(container);

                // tokenMenuDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            }
        });

    }
}

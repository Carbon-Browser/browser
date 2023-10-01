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

import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;
import android.content.SharedPreferences;

public class WalletPreferences extends Fragment {
    public WalletPreferences() {
        super(R.layout.wallet_fragment_preferences);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        View mCustomTokensButton = view.findViewById(R.id.custom_tokens_button);
        mCustomTokensButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateCustomTokens();
            }
        });

        View mLogoutButton = view.findViewById(R.id.logout_button);
        mLogoutButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SharedPreferences mSharedPreferences = new EncryptSharedPreferences(getActivity()).getSharedPreferences();
                mSharedPreferences.edit().putString("MNEMONIC_KEY", "").commit();
                mSharedPreferences.edit().putString("PIN_CODE_KEY", "").commit();

                TokenDatabase.getInstance(getActivity()).clearDatabase();

                ((WalletInterface) getActivity()).exitWallet();
            }
        });
    }
}

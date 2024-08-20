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

public class CreateWalletMenu extends Fragment {
    public CreateWalletMenu() {
        super(R.layout.wallet_fragment_create_menu);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        // Import button
        Button mImportButton = view.findViewById(R.id.button_import_wallet);
        mImportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateImportWallet();
            }
        });

        // Create button
        Button mCreateButton = view.findViewById(R.id.button_create_wallet);
        mCreateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateCreateWallet();
            }
        });
    }
}

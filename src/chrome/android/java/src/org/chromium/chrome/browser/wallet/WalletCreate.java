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

public class WalletCreate extends Fragment {
    public WalletCreate() {
        super(R.layout.wallet_fragment_create);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        LinearLayout mSeedContainer = view.findViewById(R.id.create_wallet_seed);

        String[] children = ((WalletInterface) getActivity()).getMnemonic();

        int childCounter = 0;
        for (int i = 0; i != 4; i++) {
            LinearLayout rowView = (LinearLayout)getActivity().getLayoutInflater().inflate(R.layout.create_wallet_seed_row, mSeedContainer, false);

            mSeedContainer.addView(rowView);

            TextView textView1 = rowView.findViewById(R.id.child1);
            textView1.setText(children[childCounter]);
            childCounter++;
            TextView textView2 = rowView.findViewById(R.id.child2);
            textView2.setText(children[childCounter]);
            childCounter++;
            TextView textView3 = rowView.findViewById(R.id.child3);
            textView3.setText(children[childCounter]);
            childCounter++;
        }

        Button mContinueButton = view.findViewById(R.id.button_continue);
        mContinueButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // ((WalletInterface) getActivity()).onNavigateMain();
                ((WalletInterface) getActivity()).onNavigateCreatePin();
            }
        });
    }
}

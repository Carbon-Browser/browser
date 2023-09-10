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
import java.util.ArrayList;
import java.lang.Integer;

import org.chromium.ui.widget.Toast;

public class WalletImport extends Fragment {
    public WalletImport() {
        super(R.layout.wallet_fragment_import);
    }

    private static ArrayList<String> mMnemonicList = new ArrayList<String>() {{
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
        add("");
    }};

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        LinearLayout mSeedContainer = view.findViewById(R.id.create_wallet_seed);

        String[] children = new String[12];

        int editTextCounter = 1;

        for (int i = 0; i != 4; i++) {
            LinearLayout rowView = (LinearLayout)getActivity().getLayoutInflater().inflate(R.layout.import_wallet_seed_row, mSeedContainer, false);

            mSeedContainer.addView(rowView);

            EditText editText1 = rowView.findViewById(R.id.child1);
            editText1.setTag("mnemomicEditText"+editTextCounter);
            editTextCounter++;

            editText1.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void afterTextChanged(Editable editable) {
                    String text = editable.toString().replaceAll(" ", "");

                    String editTextTag = (String) editText1.getTag();
                    int editTextIndex = Integer.parseInt(editTextTag.replaceAll("[^0-9]+", "")) - 1;

                    mMnemonicList.set(editTextIndex, text);

                    if (!text.isEmpty()) {
                        editText1.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_send_button));
                    } else {
                        editText1.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_black));
                    }
                }
            });

            EditText editText2 = rowView.findViewById(R.id.child2);
            editText2.setTag("mnemomicEditText"+editTextCounter);
            editTextCounter++;

            editText2.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void afterTextChanged(Editable editable) {
                    String text = editable.toString().replaceAll(" ", "");

                    String editTextTag = (String) editText2.getTag();
                    int editTextIndex = Integer.parseInt(editTextTag.replaceAll("[^0-9]+", "")) - 1;

                    mMnemonicList.set(editTextIndex, text);

                    if (!text.isEmpty()) {
                        editText2.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_send_button));
                    } else {
                        editText2.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_black));
                    }
                }
            });

            EditText editText3 = rowView.findViewById(R.id.child3);
            editText3.setTag("mnemomicEditText"+editTextCounter);
            editTextCounter++;

            editText3.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                }

                @Override
                public void afterTextChanged(Editable editable) {
                    String text = editable.toString().replaceAll(" ", "");

                    String editTextTag = (String) editText3.getTag();
                    int editTextIndex = Integer.parseInt(editTextTag.replaceAll("[^0-9]+", "")) - 1;

                    mMnemonicList.set(editTextIndex, text);

                    if (!text.isEmpty()) {
                        editText3.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_send_button));
                    } else {
                        editText3.setBackground(getContext().getResources().getDrawable(R.drawable.rounded_background_wallet_black));
                    }
                }
            });
        }

        Button mContinueButton = view.findViewById(R.id.button_continue);
        mContinueButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                validateMnemonic();
            }
        });
    }

    private void validateMnemonic() {
        String mnemonicString = "";
        for (int i = 0; i != mMnemonicList.size(); i++) {
          mnemonicString = mnemonicString.equals("") ? mMnemonicList.get(i) : mnemonicString + " " + mMnemonicList.get(i);
          if (mMnemonicList.get(i).equals("")) {
            Toast.makeText(getActivity(), "Please enter your full mnemonic.", Toast.LENGTH_SHORT).show();
            return;
          }
        }

        ((WalletInterface) getActivity()).onMnemonicImported(mnemonicString);
    }
}

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

import android.widget.Toast;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.ClipData;

import android.content.Intent;

import android.graphics.Bitmap;
import android.graphics.Color;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.qrcode.QRCodeWriter;

public class WalletReceive extends Fragment {
    public WalletReceive() {
        super(R.layout.wallet_fragment_receive);
    }

    private String mAddress;
    private String mCoinName;

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        if (getArguments() != null) {
            mAddress = getArguments().getString("COIN_ADDRESS_KEY", "");
            mCoinName = getArguments().getString("COIN_NAME_KEY", "").toLowerCase();
            mCoinName = mCoinName.substring(0, 1).toUpperCase() + mCoinName.substring(1);
        }

        ImageView qrCodeView = view.findViewById(R.id.qr_code_view);

        final String warningText = "Send only " + mCoinName.toLowerCase() + " to this address. Sending any other token may result in permanent loss.";
        TextView mWarningTextView = view.findViewById(R.id.scam_warning_message);
        mWarningTextView.setText(warningText);

        TextView mAddressTextView = view.findViewById(R.id.address_textview);
        mAddressTextView.setText(mAddress);

        Button mButtonCopy = view.findViewById(R.id.button_copy);
        mButtonCopy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ClipboardManager clipboard = (ClipboardManager)
                    getActivity().getSystemService(Context.CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText("Wallet Address", mAddress);
                clipboard.setPrimaryClip(clip);
                Toast.makeText(getActivity(), "Address copied to clipboard", Toast.LENGTH_SHORT).show();
            }
        });

        Button mButtonShare = view.findViewById(R.id.button_share);
        mButtonShare.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(Intent.ACTION_SEND);
                String shareBody = mAddress;
                intent.setType("text/plain");
                intent.putExtra(Intent.EXTRA_TEXT, shareBody);

                getActivity().startActivity(Intent.createChooser(intent, "Wallet Address"));
            }
        });


        QRCodeWriter writer = new QRCodeWriter();
        try {
            BitMatrix bitMatrix = writer.encode(mAddress, BarcodeFormat.QR_CODE, 512, 512);
            int width = bitMatrix.getWidth();
            int height = bitMatrix.getHeight();
            Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
            for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {
                    bmp.setPixel(x, y, bitMatrix.get(x, y) ? Color.BLACK : Color.WHITE);
                }
            }
            qrCodeView.setImageBitmap(bmp);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

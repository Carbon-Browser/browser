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
import android.app.AlertDialog;
import android.widget.LinearLayout;
import android.view.LayoutInflater;
import android.content.Context;
import java.util.ArrayList;
import android.widget.FrameLayout;
import android.view.ViewGroup;
import android.view.Gravity;
import android.content.DialogInterface;
import org.chromium.base.ApiCompatibilityUtils;

import android.graphics.drawable.ColorDrawable;
import java.lang.Integer;
import wallet.core.jni.CoinType;

public class MainTabFragment extends Fragment {
    public MainTabFragment() {
        super(R.layout.wallet_fragment_main);
    }

    private static View mMainView;

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mMainView = view;

        TextView mBalanceTextView = view.findViewById(R.id.balance_textview);
        Shader textShader = new LinearGradient(0, 0, 80, 0,
            new int[]{Color.parseColor("#FF320A"),Color.parseColor("#FF9133")},
           null, Shader.TileMode.CLAMP);
        mBalanceTextView.getPaint().setShader(textShader);

        Button mReceiveButton = view.findViewById(R.id.button_receive);
        mReceiveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openTokenSelector("receive", view.getContext());
            }
        });

        Button mSendButton = view.findViewById(R.id.button_send);
        mSendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openTokenSelector("send", view.getContext());
            }
        });

        View expandTokens = view.findViewById(R.id.expand_button_token);
        expandTokens.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openAllTokensView(view.getContext());
            }
        });

        View expandTrx = view.findViewById(R.id.expand_button_trx);
        expandTrx.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                openAllTrxView(view.getContext());
            }
        });

        drawTrxView();
        drawTokenView();
    }

    private void drawTokenView() {
        if (mMainView == null) return;

        ArrayList<TransactionObj> mTrxArray = ((WalletInterface) getActivity()).getTrxList();

        // add tokens to container
        LinearLayout mTrxContainer = mMainView.findViewById(R.id.container_tokens);
        for (int i = 0; i != mTrxArray.size(); i++) {
            TransactionObj trxObj = mTrxArray.get(i);

            View v = LayoutInflater.from(mMainView.getContext()).inflate(R.layout.wallet_transaction_item, null);
            mTrxContainer.addView(v);

            TextView actionTitleTextView = v.findViewById(R.id.action_title);
            actionTitleTextView.setText(tokenObj.name);

            TextView addressTextView = v.findViewById(R.id.address);
            addressTextView.setText(tokenObj.chainName);

            TextView trxValueTextView = v.findViewById(R.id.token_value_token);
            trxValueTextView.setText(tokenObj.balance);

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            tokenIcon.setImageResource(tokenObj.icon);

            v.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    // TODO go to trx view
                }
            });
        }
    }

    private void drawTokenView() {
        if (mMainView == null) return;

        ArrayList<TokenObj> mTokenArray = ((WalletInterface) getActivity()).getTokenList();

        // add tokens to container
        LinearLayout mTokenContainer = mMainView.findViewById(R.id.container_tokens);
        for (int i = 0; i != mTokenArray.size(); i++) {
            TokenObj tokenObj = mTokenArray.get(i);

            View v = LayoutInflater.from(mMainView.getContext()).inflate(R.layout.wallet_token_item, null);
            mTokenContainer.addView(v);

            TextView tokenNameTextView = v.findViewById(R.id.token_name);
            tokenNameTextView.setText(tokenObj.name);

            TextView tokenChainTextView = v.findViewById(R.id.token_chain);
            tokenChainTextView.setText(tokenObj.chainName);

            TextView tokenBalanceTextView = v.findViewById(R.id.token_value_token);
            tokenBalanceTextView.setText(tokenObj.balance);

            TextView tokenValueUSDTextView = v.findViewById(R.id.token_value_usd);
            tokenValueUSDTextView.setText("$"+String.format("%.2f", (Float.parseFloat(tokenObj.usdValue)*Float.parseFloat(tokenObj.balance))));

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            tokenIcon.setImageResource(tokenObj.icon);

            v.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    onTokenClicked(tokenObj, view.getContext());
                }
            });
        }
    }

    private void openAllTrxView(Context context) {
      final AlertDialog expandViewDialog = new AlertDialog.Builder(context, R.style.FullscreenDialogAnimation).create();
      expandViewDialog.setCanceledOnTouchOutside(true);

      expandViewDialog.getWindow().setBackgroundDrawable(context.getResources().getDrawable(R.drawable.rounded_background_wallet_black));

      LinearLayout containerView = (LinearLayout) LayoutInflater.from(context).inflate(R.layout.wallet_fullscreen_popup, null);
      expandViewDialog.getWindow().setGravity(Gravity.BOTTOM);
      expandViewDialog.show();

      TextView mTitle = containerView.findViewById(R.id.view_all_title);
      mTitle.setText("All Transactions");

      View mCloseButton = containerView.findViewById(R.id.exit_fullscreen_button);
      mCloseButton.setOnClickListener(new View.OnClickListener() {
          @Override
          public void onClick(View view) {
              expandViewDialog.dismiss();
          }
      });

      expandViewDialog.setContentView(containerView);

      expandViewDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
    }

    private void openAllTokensView(Context context) {
        ArrayList<TokenObj> mTokenArray = ((WalletInterface) getActivity()).getTokenList();

        final AlertDialog expandViewDialog = new AlertDialog.Builder(context, R.style.FullscreenDialogAnimation).create();
        expandViewDialog.setCanceledOnTouchOutside(true);

        expandViewDialog.getWindow().setBackgroundDrawable(context.getResources().getDrawable(R.drawable.rounded_background_wallet_black));

        LinearLayout containerView = (LinearLayout) LayoutInflater.from(context).inflate(R.layout.wallet_fullscreen_popup, null);
        expandViewDialog.getWindow().setGravity(Gravity.BOTTOM);
        expandViewDialog.show();

        TextView mTitle = containerView.findViewById(R.id.view_all_title);
        mTitle.setText("All Tokens");

        View mCloseButton = containerView.findViewById(R.id.exit_fullscreen_button);
        mCloseButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                expandViewDialog.dismiss();
            }
        });

        expandViewDialog.setContentView(containerView);

        for (int i = 0; i != mTokenArray.size(); i++) {
            TokenObj tokenObj = mTokenArray.get(i);

            View v = LayoutInflater.from(context).inflate(R.layout.wallet_token_item, null);
            containerView.addView(v);

            TextView tokenNameTextView = v.findViewById(R.id.token_name);
            tokenNameTextView.setText(tokenObj.name);

            TextView tokenChainTextView = v.findViewById(R.id.token_chain);
            tokenChainTextView.setText(tokenObj.chainName);

            TextView tokenBalanceTextView = v.findViewById(R.id.token_value_token);
            tokenBalanceTextView.setText(tokenObj.balance);

            TextView tokenValueUSDTextView = v.findViewById(R.id.token_value_usd);
            tokenValueUSDTextView.setText("$"+String.format("%.2f", (Float.parseFloat(tokenObj.usdValue)*Float.parseFloat(tokenObj.balance))));

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            tokenIcon.setImageResource(tokenObj.icon);

            v.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    onTokenClicked(tokenObj, view.getContext());
                    expandViewDialog.dismiss();
                }
            });
        }

        expandViewDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
    }

    private void onTokenClicked(TokenObj tokenObj, Context context) {
        AlertDialog tokenMenuDialog = new AlertDialog.Builder(context, R.style.SpeedDialDialogAnimation).create();
        tokenMenuDialog.setCanceledOnTouchOutside(true);

        tokenMenuDialog.getWindow().setBackgroundDrawable(context.getResources().getDrawable(R.drawable.rounded_background_wallet_black));

        int width = context.getResources().getDisplayMetrics().widthPixels;
        int height = tokenMenuDialog.getWindow().getAttributes().height;
        tokenMenuDialog.getWindow().setLayout(width, height);

        FrameLayout container = new FrameLayout(context);
        tokenMenuDialog.getWindow().setGravity(Gravity.BOTTOM);
        tokenMenuDialog.show();

        tokenMenuDialog.setContentView(container);

        tokenMenuDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        container.setMinimumHeight(context.getResources().getDimensionPixelSize(R.dimen.min_popup_dialog_height));

        // center
        View optionsLayout = LayoutInflater.from(context).inflate(R.layout.wallet_popup_menu, null);
        container.addView(optionsLayout);
        View tokenIconPopup = optionsLayout.findViewById(R.id.token_icon);
        tokenIconPopup.setBackground(context.getResources().getDrawable(tokenObj.icon));

        View mSendButton = container.findViewById(R.id.token_option_send);
        mSendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateSend(tokenObj);
                tokenMenuDialog.dismiss();
            }
        });
        View mReceiveButton = container.findViewById(R.id.token_option_receive);
        mReceiveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateReceive(tokenObj);
                tokenMenuDialog.dismiss();
            }
        });

        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)optionsLayout.getLayoutParams();
        lp.gravity = Gravity.CENTER;
        optionsLayout.setLayoutParams(lp);
    }

    private void openTokenSelector(String type, Context context) {
        ArrayList<TokenObj> mTokenArray = ((WalletInterface) getActivity()).getTokenList();

        AlertDialog tokenMenuDialog = new AlertDialog.Builder(context, R.style.SpeedDialDialogAnimation).create();
        tokenMenuDialog.setCanceledOnTouchOutside(true);

        tokenMenuDialog.getWindow().setBackgroundDrawable(context.getResources().getDrawable(R.drawable.rounded_background_wallet_black));

        LinearLayout container = new LinearLayout(context);
        container.setOrientation(LinearLayout.VERTICAL);
        tokenMenuDialog.getWindow().setGravity(Gravity.BOTTOM);
        tokenMenuDialog.show();

        tokenMenuDialog.setContentView(container);

        tokenMenuDialog.getWindow().setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        container.setMinimumHeight(context.getResources().getDimensionPixelSize(R.dimen.min_popup_dialog_height));

        for (int i = 0; i != mTokenArray.size(); i++) {
            View tokenRow = LayoutInflater.from(context).inflate(R.layout.wallet_token_selector_row, null);
            container.addView(tokenRow);

            final TokenObj tokenObj = mTokenArray.get(i);

            ImageView tokenIcon = tokenRow.findViewById(R.id.token_icon);
            tokenIcon.setImageResource(tokenObj.icon);

            TextView tokenName = tokenRow.findViewById(R.id.token_name);
            tokenName.setText(tokenObj.name);

            TextView tokenChain = tokenRow.findViewById(R.id.token_chain);
            tokenChain.setText(tokenObj.chainName);

            TextView tokenBalance = tokenRow.findViewById(R.id.token_balance);
            tokenBalance.setText(tokenObj.balance);

            tokenRow.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (type.equals("send")) {
                        ((WalletInterface) getActivity()).onNavigateSend(tokenObj);
                    } else {
                        ((WalletInterface) getActivity()).onNavigateReceive(tokenObj);
                    }
                    tokenMenuDialog.dismiss();
                }
            });
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mMainView = null;
    }
}

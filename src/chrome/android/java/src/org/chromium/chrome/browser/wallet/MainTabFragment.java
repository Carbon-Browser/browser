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

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;

import android.graphics.drawable.ColorDrawable;
import java.lang.Integer;
import wallet.core.jni.CoinType;

import android.widget.ProgressBar;
import android.os.Handler;
import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;
import android.content.SharedPreferences;
import android.widget.ScrollView;

public class MainTabFragment extends Fragment implements WalletDatabaseInterface {
    public MainTabFragment() {
        super(R.layout.wallet_fragment_main);
    }

    private float density;
    private int valueInDp;

    private static View mMainView;

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mMainView = view;

        density = view.getContext().getResources().getDisplayMetrics().density;
        valueInDp = (int)(20 * density);

        View walletReloadButton = view.findViewById(R.id.wallet_reload);
        final ProgressBar progressBar = view.findViewById(R.id.loading_progress);
        final ScrollView scrollView = view.findViewById(R.id.main_scrollview);
        walletReloadButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              final SharedPreferences mSharedPreferences = new EncryptSharedPreferences(getActivity()).getSharedPreferences();

              // Get the current system time
              long currentTime = System.currentTimeMillis();

              // Retrieve the last reload time from SharedPreferences
              long lastReloadTime = mSharedPreferences.getLong("RELOAD_TIME_KEY", 0);

              // Calculate the difference in minutes
              long differenceInMinutes = (currentTime - lastReloadTime) / (60 * 1000);

              // Check if the difference is greater than or equal to 3 minutes
              if (differenceInMinutes >= 0.35) {
                  // It's been at least 3 minutes since the last reload, proceed with the reload
                  mSharedPreferences.edit().putLong("RELOAD_TIME_KEY", currentTime).apply(); // Store the current time as the last reload time

                  progressBar.setVisibility(View.VISIBLE); // Show the progress bar#

                  ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) scrollView.getLayoutParams();
                  params.topMargin = 0;
                  scrollView.setLayoutParams(params);

                  ((WalletInterface) getActivity()).onReload();

                  // Create a Handler to run code in the future
                  new Handler().postDelayed(new Runnable() {
                      @Override
                      public void run() {
                          if (getContext() == null) return;

                          progressBar.setVisibility(View.GONE); // Hide the progress bar after 4 seconds

                          // Reset the top margin of the scrollView to 5dp after the delay
                          int topMarginInDp = 5;
                          final float scale = getContext().getResources().getDisplayMetrics().density;
                          int topMarginInPx = (int) (topMarginInDp * scale + 0.5f);
                          ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) scrollView.getLayoutParams();
                          params.topMargin = topMarginInPx;
                          scrollView.setLayoutParams(params);
                      }
                  }, 5000); // 4000 milliseconds delay
              }
            }
        });

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

        View mPreferencesButton = view.findViewById(R.id.wallet_preferences);
        mPreferencesButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigatePreferences();
            }
        });

        calculateTotal();
        drawTrxView();
        drawTokenView();
    }

    @Override
    public void onTransactionsReceived() {
        try {
            drawTrxView();
        } catch (Exception ignore) {}
    }

    @Override
    public void onTokenInfoReceived() {
        try {
            drawTokenView();
            calculateTotal();
        } catch (Exception ignore) {}
    }

    private void calculateTotal() {
        String walletValue = ((WalletInterface) getActivity()).getWalletValue();
        TextView mBalanceTextView = mMainView.findViewById(R.id.balance_textview);
        mBalanceTextView.setText("$"+walletValue);
    }

    private void drawTrxView() {
        if (mMainView == null) return;

        ArrayList<TransactionObj> mTrxArray = ((WalletInterface) getActivity()).getTrxList();

        // add tokens to container
        LinearLayout mTrxContainer = mMainView.findViewById(R.id.container_trx);

        for (int i = 0; i != mTrxArray.size() && i < 3; i++) {
            final TransactionObj trxObj = mTrxArray.get(i);

            View v;
            if (mTrxContainer.getChildAt(i + 1) == null) {
                v = LayoutInflater.from(mMainView.getContext()).inflate(R.layout.wallet_transaction_item, null);
                mTrxContainer.addView(v);
            } else {
                v = (View)mTrxContainer.getChildAt(i + 1);
            }

            TextView actionTitleTextView = v.findViewById(R.id.action_title);
            actionTitleTextView.setText(trxObj.isSender ? "Sent" : "Received");

            TextView addressTextView = v.findViewById(R.id.address);
            addressTextView.setText(trxObj.isSender ? trxObj.recipientAddress : trxObj.senderAddress);

            TextView trxValueTextView = v.findViewById(R.id.token_value_token);
            trxValueTextView.setText(trxObj.amount);

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            Glide.with(tokenIcon)
                .load(trxObj.tokenIconUrl)
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

            v.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    ((WalletInterface) getActivity()).onNavigateTransactionView(trxObj);
                }
            });
        }
    }

    private void drawTokenView() { // carbon = 0, bsc = 1, eth = 2
        if (mMainView == null) return;

        ArrayList<TokenObj> mTokenArray = ((WalletInterface) getActivity()).getTokenList();

        LinearLayout mTokenContainer = mMainView.findViewById(R.id.container_tokens);

        boolean doTokensExist = mTokenContainer.getChildCount() > 1;

        // add tokens to container
        for (int i = 0; i != mTokenArray.size(); i++) {
            TokenObj tokenObj = mTokenArray.get(i);

            View v;

            if (doTokensExist) {
                v = (View)mTokenContainer.getChildAt(i + 1);
            } else {
                v = LayoutInflater.from(mMainView.getContext()).inflate(R.layout.wallet_token_item, null);
                mTokenContainer.addView(v);
            }

            TextView tokenNameTextView = v.findViewById(R.id.token_name);
            tokenNameTextView.setText(tokenObj.name);

            TextView tokenChainTextView = v.findViewById(R.id.token_chain);
            tokenChainTextView.setText(tokenObj.chainName);

            TextView tokenBalanceTextView = v.findViewById(R.id.token_value_token);
            tokenBalanceTextView.setText(tokenObj.balance);

            TextView tokenValueUSDTextView = v.findViewById(R.id.token_value_usd);
            tokenValueUSDTextView.setText("$"+String.format("%.2f", (Float.parseFloat(tokenObj.usdValue)*Float.parseFloat(tokenObj.balance))));

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            Glide.with(tokenIcon)
                .load(tokenObj.iconUrl)
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

      LinearLayout mainContainer = containerView.findViewById(R.id.main_container);

      ArrayList<TransactionObj> mTrxArray = ((WalletInterface) getActivity()).getTrxList();

      for (int i = 0; i != mTrxArray.size(); i++) {
          final TransactionObj trxObj = mTrxArray.get(i);

          View v = LayoutInflater.from(mMainView.getContext()).inflate(R.layout.wallet_transaction_item, null);
          mainContainer.addView(v);

          TextView actionTitleTextView = v.findViewById(R.id.action_title);
          actionTitleTextView.setText(trxObj.isSender ? "Sent" : "Received");

          TextView addressTextView = v.findViewById(R.id.address);
          addressTextView.setText(trxObj.isSender ? trxObj.recipientAddress : trxObj.senderAddress);

          TextView trxValueTextView = v.findViewById(R.id.token_value_token);
          trxValueTextView.setText(trxObj.amount);

          ImageView tokenIcon = v.findViewById(R.id.token_icon);
          Glide.with(tokenIcon)
              .load(trxObj.tokenIconUrl)
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

          v.setOnClickListener(new View.OnClickListener() {
              @Override
              public void onClick(View view) {
                  ((WalletInterface) getActivity()).onNavigateTransactionView(trxObj);
                  expandViewDialog.dismiss();
              }
          });
      }

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

        LinearLayout mainContainer = containerView.findViewById(R.id.main_container);

        for (int i = 0; i != mTokenArray.size(); i++) {
            TokenObj tokenObj = mTokenArray.get(i);

            View v = LayoutInflater.from(context).inflate(R.layout.wallet_token_item, null);
            mainContainer.addView(v);

            TextView tokenNameTextView = v.findViewById(R.id.token_name);
            tokenNameTextView.setText(tokenObj.name);

            TextView tokenChainTextView = v.findViewById(R.id.token_chain);
            tokenChainTextView.setText(tokenObj.chainName);

            TextView tokenBalanceTextView = v.findViewById(R.id.token_value_token);
            tokenBalanceTextView.setText(tokenObj.balance);

            TextView tokenValueUSDTextView = v.findViewById(R.id.token_value_usd);
            tokenValueUSDTextView.setText("$"+String.format("%.2f", (Float.parseFloat(tokenObj.usdValue)*Float.parseFloat(tokenObj.balance))));

            ImageView tokenIcon = v.findViewById(R.id.token_icon);
            Glide.with(tokenIcon)
                .load(tokenObj.iconUrl)
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
        ImageView tokenIconPopup = optionsLayout.findViewById(R.id.token_icon);
        Glide.with(tokenIconPopup)
            .load(tokenObj.iconUrl)
            //.thumbnail(0.05f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .fitCenter()
            // .apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
            // .transform(new RoundedCorners(valueInDp))
            .into(new CustomTarget<Drawable>() {
                @Override
                public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                    if (tokenIconPopup.getDrawable() == null)
                        tokenIconPopup.setImageDrawable(resource);
                }

                @Override
                public void onLoadCleared(@Nullable Drawable placeholder) {

                }
            });

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
            Glide.with(tokenIcon)
                .load(tokenObj.iconUrl)
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

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

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;

import android.view.LayoutInflater;
import java.util.ArrayList;

import android.view.MenuItem;
import android.widget.PopupMenu;
import android.view.Menu;

public class WalletPreferencesCustomTokens extends Fragment {
    public WalletPreferencesCustomTokens() {
        super(R.layout.wallet_fragment_preferences_custom_tokens);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        ArrayList<TokenObj> mTokenArray = ((WalletInterface) getActivity()).getTokenList();

        LinearLayout tokenContainer = view.findViewById(R.id.token_container);

        View addButton = view.findViewById(R.id.add_button);
        addButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ((WalletInterface) getActivity()).onNavigateAddCustomToken();
            }
        });

        for (int i = 0; i != mTokenArray.size(); i++) {
            final TokenObj tokenObj = mTokenArray.get(i);

            // skip bsc, eth, and csix
            if (!tokenObj.isCustomType) continue;
            if (tokenObj.chain != null && tokenObj.chain.equals("0x04756126f044634c9a0f0e985e60c88a51acc206")) continue;

            final View tokenRow = LayoutInflater.from(view.getContext()).inflate(R.layout.wallet_custom_token_item_row, null);
            tokenContainer.addView(tokenRow);

            TextView tokenTicker = tokenRow.findViewById(R.id.token_ticker);
            tokenTicker.setText(tokenObj.name + " (" + tokenObj.ticker + ")");

            TextView tokenNameChain = tokenRow.findViewById(R.id.token_name_chain);
            tokenNameChain.setText(tokenObj.chainName);

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

            View tokenOptionsButton = tokenRow.findViewById(R.id.token_control_button);
            tokenOptionsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    //  open popup with option to delete
                    PopupMenu popupMenu = new PopupMenu(view.getContext(), tokenOptionsButton);

                    // Programmatically add menu items
                    popupMenu.getMenu().add(Menu.NONE, Menu.NONE, Menu.NONE, "Remove Item");

                    // Set menu item click listener
                    popupMenu.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            //if (item.getItemId() == R.id.remove_item) {
                                tokenContainer.removeView(tokenRow);
                                TokenDatabase.getInstance(getActivity()).removeToken(tokenObj.ticker);
                                return true;
                            //}
                            //return false;
                        }
                    });

                    // Show the menu
                    popupMenu.show();
                }
            });
        }
    }
}

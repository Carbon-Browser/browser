package org.chromium.chrome.browser.rewards;

import org.chromium.chrome.R;
import android.view.View;
import androidx.recyclerview.widget.RecyclerView;
import android.content.Context;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import java.util.ArrayList;
import org.chromium.base.task.AsyncTask;
import java.io.BufferedReader;
import java.net.URL;
import java.net.URLConnection;
import java.io.InputStreamReader;
import org.w3c.dom.Document;
import org.xml.sax.InputSource;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import java.io.StringReader;
import org.w3c.dom.NodeList;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import android.widget.TextView;
import android.graphics.drawable.Drawable;

import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.browser.app.ChromeActivity;
import android.widget.ImageView;
import org.chromium.content_public.browser.LoadUrlParams;
import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import com.bumptech.glide.request.RequestOptions;
import org.chromium.chrome.browser.rewards.RewardObject;
import org.chromium.chrome.browser.rewards.RewardsAPIBridge;
import android.widget.LinearLayout;
import android.widget.Button;
import android.content.Intent;
import org.chromium.base.IntentUtils;
import android.widget.PopupWindow;
import android.os.Handler;
import android.os.Looper;
import java.lang.Runnable;
import androidx.appcompat.widget.AppCompatImageButton;
import android.graphics.Color;

public class RewardsRecyclerAdapter extends RecyclerView.Adapter<RewardsRecyclerAdapter.ViewHolder> implements RewardsAPIBridge.RewardsAPIBridgeCommunicator {

    private ArrayList<RewardObject> mData = new ArrayList<>();
    private LayoutInflater mInflater;

    private SharedPreferences mPrefs;

    private TextView mErrorMessageTextView;
    private LinearLayout mLoadingIndicator;
    private PopupWindow mPopup;
    private RewardsCommunicator mRewardsCommunicator;

    // data is passed into the constructor
    public RewardsRecyclerAdapter(Context context, LinearLayout loadingIndicator, TextView errorMessageTextView, PopupWindow popup, RewardsCommunicator communicator) {
        this.mInflater = LayoutInflater.from(context);

        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        mRewardsCommunicator = communicator;

        mLoadingIndicator = loadingIndicator;
        mErrorMessageTextView = errorMessageTextView;
        mPopup = popup;

        RewardsAPIBridge.getInstance().getAllRewards(this);
    }

    @Override
    public void onRewardsReceived(ArrayList<RewardObject> rewards) {
        this.mData = rewards;

        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                mLoadingIndicator.setVisibility(View.GONE);
                notifyDataSetChanged();
            }
        });
    }

    @Override
    public void onReceiveRewardsError() {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                mLoadingIndicator.setVisibility(View.GONE);

                mErrorMessageTextView.setVisibility(View.VISIBLE);
            }
        });
    }

    // inflates the row layout from xml when needed
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.rewards_item_view, parent, false);

        return new ViewHolder(view);
    }

    // binds the data to the TextView in each row
    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        View view = holder.itemView;

        final RewardObject mRewardObject = mData.get(position);

        ImageView mRewardImage = view.findViewById(R.id.reward_imageview);
        final float density = view.getContext().getResources().getDisplayMetrics().density;
        final int valueInDp = (int)(10 * density);
        final String imageUrl = mRewardObject.imageUrl.replace("\\", "").replace("\"", "").replace("[", "").replace("]", "");
        Glide.with(mRewardImage)
            .load(imageUrl)
            .thumbnail(0.05f)
            .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
            .fitCenter()
            //.apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
            .transform(new RoundedCorners(valueInDp))
            .into(new CustomTarget<Drawable>() {
                @Override
                public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                    if (mRewardImage.getDrawable() == null)
                        mRewardImage.setImageDrawable(resource);
                }

                @Override
                public void onLoadCleared(@Nullable Drawable placeholder) {

                }
            });

        TextView mRewardMonetary = view.findViewById(R.id.reward_monetary_value);

        String rewardPointsString = mRewardObject.valueDollar + " $CSIX";
        if (mRewardObject.valueDollar < mRewardObject.valuePoints) {
            rewardPointsString = "$" + mRewardObject.valueDollar + " in $CSIX";
        }

        mRewardMonetary.setText(rewardPointsString);

        TextView mRewardName = view.findViewById(R.id.reward_name);
        mRewardName.setText(mRewardObject.name);

        if (mRewardObject.name.equals("Carbon PRO (6mo)")) {
            AppCompatImageButton mProHelpBtn = view.findViewById(R.id.pro_help_btn);
            mProHelpBtn.setVisibility(View.VISIBLE);

            mProHelpBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mRewardsCommunicator.loadRewardsUrl("https://trycarbon.io/#pro", view);

                    mPopup.dismiss();
                }
            });
        }

        LinearLayout mRedeemButton = view.findViewById(R.id.reward_redeem_button);
        if (RewardsAPIBridge.getInstance().getTotalCreditBalance() < mRewardObject.valuePoints) {
            mRedeemButton.setEnabled(false);
        }
        mRedeemButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent();
                intent.setClass(view.getContext(), RewardsRedeemActivity.class);

                intent.putExtra("rewardImageUrl", imageUrl);
                intent.putExtra("rewardValueDollar", mRewardObject.valueDollar+ " $CSIX");
                intent.putExtra("rewardValuePoints", mRewardObject.valuePoints);
                intent.putExtra("rewardName", mRewardObject.name);
                intent.putExtra("rewardId", mRewardObject.id);

                IntentUtils.safeStartActivity(view.getContext(), intent);

                mPopup.dismiss();
            }
        });
    }

    // total number of rows
    @Override
    public int getItemCount() {
        return mData.size();
    }

    // stores and recycles views as they are scrolled off screen
    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        ViewHolder(View itemView) {
            super(itemView);
        }

        @Override
        public void onClick(View view) {

        }
    }
}

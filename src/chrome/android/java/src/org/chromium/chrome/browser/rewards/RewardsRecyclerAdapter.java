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
import android.os.Handler;
import android.os.Looper;
import java.lang.Runnable;
import androidx.appcompat.widget.AppCompatImageButton;
import org.chromium.chrome.browser.rewards.RewardsCommunicator;
import android.graphics.Color;

import java.lang.Integer;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.List;
import org.chromium.chrome.browser.rewards.v2.RewardsHelper;
import org.chromium.chrome.browser.rewards.v2.RewardObjectV2;

public class RewardsRecyclerAdapter extends RecyclerView.Adapter<RewardsRecyclerAdapter.ViewHolder> implements RewardsAPIBridge.RewardsAPIBridgeCommunicator {

    private ArrayList<RewardObjectV2> mData = new ArrayList<>();
    private LayoutInflater mInflater;

    private SharedPreferences mPrefs;

    private TextView mErrorMessageTextView;
    private LinearLayout mLoadingIndicator;
    private RewardsCommunicator mRewardsCommunicator;

    private boolean mIsDarkMode;

    private int balance;

    // data is passed into the constructor
    public RewardsRecyclerAdapter(Context context, LinearLayout loadingIndicator, TextView errorMessageTextView, RewardsCommunicator communicator, boolean isDarkMode) {
        this.mInflater = LayoutInflater.from(context);

        mIsDarkMode = isDarkMode;

        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        mRewardsCommunicator = communicator;

        mLoadingIndicator = loadingIndicator;
        mErrorMessageTextView = errorMessageTextView;

        RewardsHelper.getInstance(context).getAllRewardsAsync().thenAccept(rewards -> {
            this.mData = parseRewards(rewards);

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    mLoadingIndicator.setVisibility(View.GONE);
                    notifyDataSetChanged();
                }
            });
        });

        RewardsHelper.getInstance(context).getBalanceAsync().thenAccept(balance -> {
          this.balance = Integer.parseInt(balance);
        });
    }

    public static ArrayList<RewardObjectV2> parseRewards(String jsonResponse) {
        ArrayList<RewardObjectV2> rewardsList = new ArrayList<>();

        try {
            JSONArray jsonArray = new JSONArray(jsonResponse);

            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);

                // Extracting values from JSON object
                String id = jsonObject.getString("token_id");
                String name = jsonObject.getString("name");
                String cost = String.valueOf(jsonObject.getInt("cost"));
                String amount = String.valueOf(jsonObject.getInt("amount"));
                String uri = jsonObject.getString("uri");
                int inventory = jsonObject.getInt("inventory");

                // Extracting image URL from metadata
                JSONObject metadata = jsonObject.getJSONObject("metadata");
                String imageUrl = metadata.getString("image");

                // Creating RewardObjectV2 object and adding to list
                RewardObjectV2 reward = new RewardObjectV2(id, name, cost, amount, uri, imageUrl, inventory);
                rewardsList.add(reward);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return rewardsList;
    }

    // UNUSED IN v2 --
    @Override
    public void onRewardsReceived(ArrayList<RewardObject> rewards) {
        // this.mData = rewards;
        //
        // new Handler(Looper.getMainLooper()).post(new Runnable() {
        //     @Override
        //     public void run() {
        //         mLoadingIndicator.setVisibility(View.GONE);
        //         notifyDataSetChanged();
        //     }
        // });
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
        String textColor = mIsDarkMode ? "#ffffff" : "#000000";

        View view = holder.itemView;

        final RewardObjectV2 mRewardObject = mData.get(position);

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
        mRewardMonetary.setTextColor(Color.parseColor(textColor));
        TextView mRewardMonetary2 = view.findViewById(R.id.reward_monetary_value2);

        String rewardPointsString = mRewardObject.cost + " CRT";
        mRewardMonetary2.setText("Claim");

        mRewardMonetary.setText(rewardPointsString);

        TextView mRewardName = view.findViewById(R.id.reward_name);
        mRewardName.setText(mRewardObject.name);
        mRewardName.setTextColor(Color.parseColor(textColor));

        LinearLayout mRedeemButton = view.findViewById(R.id.reward_redeem_button);
        // if (balance < Integer.parseInt(mRewardObject.cost)) { DISABLE CLAIMING UNTIL BACKEND SUPPORTED
            mRedeemButton.setEnabled(false);
        // }
        mRedeemButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent();
                intent.setClass(view.getContext(), RewardsRedeemActivity.class);

                intent.putExtra("rewardImageUrl", imageUrl);
                intent.putExtra("rewardCost", mRewardObject.cost+ " CRT");
                intent.putExtra("rewardInventory",mRewardObject.inventory);
                intent.putExtra("rewardName", mRewardObject.name);
                intent.putExtra("rewardId", mRewardObject.id);

                IntentUtils.safeStartActivity(view.getContext(), intent);
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

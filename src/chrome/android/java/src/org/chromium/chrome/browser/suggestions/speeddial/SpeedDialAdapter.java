package org.chromium.chrome.browser.suggestions.speeddial;

import android.content.Context;
import android.graphics.Color;
import androidx.core.view.MotionEventCompat;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.FrameLayout;
import android.graphics.drawable.Drawable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.chromium.chrome.browser.suggestions.speeddial.helper.ItemTouchHelperAdapter;
import org.chromium.chrome.browser.suggestions.speeddial.helper.ItemTouchHelperViewHolder;
import org.chromium.chrome.browser.suggestions.speeddial.helper.OnStartDragListener;
import org.chromium.chrome.R;

import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.favicon.IconType;
import org.chromium.chrome.browser.profiles.Profile;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.content.res.Resources;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.url.GURL;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.chrome.browser.app.ChromeActivity;

import android.app.AlertDialog;
import android.webkit.URLUtil;
import android.widget.EditText;
import android.widget.Button;

import android.graphics.PorterDuff;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.vectordrawable.graphics.drawable.VectorDrawableCompat;

import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import java.net.URL;

import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialInteraction;

import java.util.List;
import java.util.UUID;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.util.Set;

import android.telephony.TelephonyManager;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import android.util.TypedValue;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.amplitude.api.Amplitude;

/**
 * Simple RecyclerView.Adapter that implements {@link ItemTouchHelperAdapter} to respond to move and
 * dismiss events from a {@link androidx.recyclerview.widget.ItemTouchHelper}.
 */
public class SpeedDialAdapter extends RecyclerView.Adapter<SpeedDialAdapter.SpeedDialViewHolder>
        implements ItemTouchHelperAdapter {

    private final List<SpeedDialDataItem> mItems;

    private final OnStartDragListener mDragStartListener;

    private RecyclerView mRecyclerView;

    private boolean isInEditMode;

    private final LargeIconBridge mIconBridge;

    private RoundedIconGenerator mIconGenerator;

    private final SpeedDialDatabase mDatabase;

    private final SpeedDialInteraction speedDialInteraction;

    private boolean hasBackground;

    private String deviceID;

    private boolean mIsDarkEnabled;

    public SpeedDialAdapter(Context context, OnStartDragListener dragStartListener,
            SpeedDialInteraction speedDialInteraction, boolean isPopup) {
        mDragStartListener = dragStartListener;

        mDatabase = new SpeedDialDatabase();

        mItems = mDatabase.getSpeedDials();

        mIconBridge = new LargeIconBridge(Profile.getLastUsedRegularProfile());

        this.speedDialInteraction = speedDialInteraction;

        // Make unique identifier for site plug
        final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();
        deviceID = mPrefs.getString("unique_id", null);
        if (deviceID == null) {
            deviceID = UUID.randomUUID().toString().replace("-", "").substring(0, 16);
            mPrefs.edit().putString("unique_id", deviceID).apply();
        }

        // download, cache, and save sponsored speed dials..
        fetchSponsoredSpeedDials(isPopup);
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        super.onAttachedToRecyclerView(recyclerView);

        mRecyclerView = recyclerView;

        mIconGenerator = FaviconUtils.createCircularIconGenerator(mRecyclerView.getContext());
    }

    @Override
    public SpeedDialViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.speed_dial_tile_view, parent, false);
        SpeedDialViewHolder sdViewHolder = new SpeedDialViewHolder(view);
        return sdViewHolder;
    }

    @Override
    public void onBindViewHolder(final SpeedDialViewHolder holder, int position) {
        if (position == mItems.size()) {
            holder.setAddBtn();
        }

        Resources res = mRecyclerView.getContext().getResources();

        if (holder.isAddBtn) {
            // implement add button

            // set the add btn tint depending on theme color..
            int themeColor = -1;
            try {
                TypedValue typedValue = new TypedValue();
                mRecyclerView.getContext().getTheme().resolveAttribute(R.attr.colorPrimary, typedValue, true);
                themeColor = typedValue.data;
            } catch (Exception e) {}

            Drawable addDrawable = res.getDrawable(R.drawable.ic_add);
            addDrawable.setColorFilter(Color.parseColor("#50515B")/*res.getColor((themeColor == -1)
                    ? android.R.color.black : android.R.color.black)*/, android.graphics.PorterDuff.Mode.SRC_IN);

            final float density = res.getDisplayMetrics().density;
            final int valueInDp = (int)(15 * density);

            holder.imageView.getLayoutParams().height -= valueInDp;
            holder.imageView.getLayoutParams().width -= valueInDp;
            holder.imageView.setImageDrawable(addDrawable);
            //FrameLayout.MarginLayoutParams layoutParams = (FrameLayout.MarginLayoutParams)holder.imageView.getLayoutParams();
            //layoutParams.setMargins(0, res.getDimensionPixelSize(R.dimen.speed_dial_icon_margin_top), 0, 0);
            //holder.imageView.setLayoutParams(layoutParams);
            //holder.textView.setText(mRecyclerView.getContext().getResources().getString(R.string.app_banner_add));

            // handle click
            holder.view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    // add dialog
                    AlertDialog alertDialog = new AlertDialog.Builder(view.getContext()).create();//, R.style.SpeedDialDialogAnimation).create();
                    alertDialog.setCanceledOnTouchOutside(true);

                    alertDialog.show();
                    alertDialog.setContentView(LayoutInflater.from(view.getContext()).inflate(R.layout.speed_dial_add_dialog, null, false));

                    alertDialog.getWindow().clearFlags(
                        android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                                android.view.WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);

                    Button okBtn = alertDialog.findViewById(R.id.speed_dial_add_dialog_ok);
                    okBtn.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            // process adding the speed dial
                            // need to add this hacky focus listener so the keyboard shows..................
                            EditText titleET = alertDialog.findViewById(R.id.speed_dial_add_dialog_et_title);
                            EditText addressET = alertDialog.findViewById(R.id.speed_dial_add_dialog_et);

                            String url = addressET.getText().toString();
                            String title = titleET.getText().toString();

                            // validate the url
                            if (!URLUtil.isValidUrl(url)) {
                                url = URLUtil.guessUrl(url);
                            }

                            // validate the title
                            if (title.isEmpty()) title = url;

                            // add it
                            mDatabase.addSpeedDial(title, url);
                            mItems.add(new SpeedDialDataItem(title, url, mItems.size(), false, false, null, null));
                            notifyItemInserted(mItems.size() - 1);

                            // dismiss the dialog to finish off
                            alertDialog.cancel();
                        }
                    });

                    Button cancelBtn = alertDialog.findViewById(R.id.speed_dial_add_dialog_cancel);
                    cancelBtn.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            alertDialog.cancel();
                        }
                    });
                }
            });
        } else {
            SpeedDialDataItem tile = mItems.get(position);

            // set the title
            holder.textView.setText(tile.title);
            // set text colour white if item was added by the user and ntp has a background
            if (mIsDarkEnabled) {
                holder.textView.setTextColor(Color.WHITE);
            }

            // set the background
            // use glide for default tiles and sponsored tiles, large icon bridge for custom ones
            String url = tile.url;
            if (!tile.isSponsored) {
                GURL gurl = new GURL(url);
                gurl = null;

                Drawable iconDrawable = FaviconUtils.getIconDrawableWithoutFilter(
                        null, url, Color.TRANSPARENT, mIconGenerator, res, res.getDimensionPixelSize(R.dimen.speed_dial_tile_view_icon_size));
                holder.imageView.setBackground(iconDrawable);

                final float density = res.getDisplayMetrics().density;
                final int valueInDp = (int)(9 * density);
                Glide.with(holder.imageView)
                    .load(imageUrl)
                    .thumbnail(0.1f)
                    .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                    .fitCenter()
                    .transform(new RoundedCorners(valueInDp))
                    .placeholder(iconDrawable)
                    .into(new CustomTarget<Drawable>() {
                        @Override
                        public void onResourceReady(@NonNull Drawable resource, @Nullable Transition<? super Drawable> transition) {
                            if (resource.getIntrinsicHeight() <= 32) {
                                // TODO
                                int halfWidth = res.getDimensionPixelSize(R.dimen.speed_dial_tile_view_icon_size) / 3;
                                holder.imageView.getLayoutParams().height = halfWidth;
                                holder.imageView.getLayoutParams().width = halfWidth;
                            }

                            holder.imageView.setBackground(resource);
                        }

                        @Override
                        public void onLoadCleared(@Nullable Drawable placeholder) {

                        }
                    });
            } else if (tile.isSponsored) {
                Drawable iconDrawable = FaviconUtils.getIconDrawableWithoutFilter(
                        null, url, Color.TRANSPARENT, mIconGenerator, res, 120);
                holder.imageView.setBackground(iconDrawable);

                final float density = res.getDisplayMetrics().density;
                final int valueInDp = (int)(9 * density);
                Glide.with(holder.imageView)
                    .load(tile.imageUrl)
                    .thumbnail(0.1f)
                    .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                    .fitCenter()
                    .placeholder(iconDrawable)
                    .transform(new RoundedCorners(valueInDp))
                    .into(new CustomTarget<Drawable>() {
                        @Override
                        public void onResourceReady(@NonNull Drawable resource, @Nullable Transition<? super Drawable> transition) {
                            holder.imageView.setBackground(resource);
                        }

                        @Override
                        public void onLoadCleared(@Nullable Drawable placeholder) {

                        }
                    });

                logSponsoredTileImpression(tile.impressionUrl);
            } /*else {
                LargeIconCallback callbackWrapper = new LargeIconCallback() {
                    @Override
                    public void onLargeIconAvailable(Bitmap icon, int fallbackColor,
                            boolean isFallbackColorDefault, @IconType int iconType) {
                        Drawable iconDrawable = FaviconUtils.getIconDrawableWithoutFilter(
                                icon, url, fallbackColor, mIconGenerator, res, 96);

                        if (holder != null) holder.imageView.setBackground(iconDrawable);
                    }
                };
                mIconBridge.getLargeIconForStringUrl(url, 96, callbackWrapper);
            } */

            // handle click
            holder.view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (isInEditMode) {
                        // Escape edit mode
                        isInEditMode = false;
                        updateEditMode(false);
                    } else {
                        // Load the speed dial
                        if (mRecyclerView.getContext() instanceof ChromeActivity) {
                            LoadUrlParams loadUrlParams = new LoadUrlParams(tile.url);
                            ChromeActivity activity = (ChromeActivity)mRecyclerView.getContext();
                            activity.getActivityTab().loadUrl(loadUrlParams);
                        }

                        if (speedDialInteraction != null) {
                            speedDialInteraction.onSpeedDialClicked();
                        }
                        Amplitude.getInstance().logEvent("speed_dial_click_event");
                    }
                }
            });

            // delete button click
            holder.deleteBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    FrameLayout deleteBtnParent = (FrameLayout)view.getParent();
                    final SpeedDialViewHolder holder = (SpeedDialViewHolder)mRecyclerView.getChildViewHolder(deleteBtnParent);
                    onItemDismiss(holder.getAdapterPosition());
                }
            });
        }

        // Start a drag whenever the handle view it touched
        holder.view.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                if (!holder.isAddBtn) mDragStartListener.onStartDrag(holder);

                isInEditMode = true;

                updateEditMode(true);

                return false;
            }
        });
    }

    @Override
    public void onItemDismiss(int position) {
        mItems.remove(position);
        notifyItemRemoved(position);

        if (mDatabase != null) {
            mDatabase.removeSpeedDial(position);
        }
    }

    @Override
    public boolean onItemMove(int fromPosition, int toPosition) {
        // account for the add button
        if (toPosition == mItems.size()) toPosition--;

        // apply the change in the database
        if (mDatabase != null) {
            mDatabase.moveSpeedDial(fromPosition, toPosition);
        }

        Collections.swap(mItems, fromPosition, toPosition);
        notifyItemMoved(fromPosition, toPosition);

        return true;
    }

    @Override
    public int getItemCount() {
        return mItems.size() + 1;
    }

    private void updateEditMode(boolean isVisibile) {
        for (int childCount = mRecyclerView.getChildCount(), i = 0; i < childCount; ++i) {
            final SpeedDialViewHolder holder = (SpeedDialViewHolder)mRecyclerView.getChildViewHolder(mRecyclerView.getChildAt(i));

            // show delete buttons in edit mode, but hide add button
            if (holder.isAddBtn) {
                holder.view.setVisibility(isVisibile ? View.GONE : View.VISIBLE);
            } else {
                holder.deleteBtn.setVisibility(isVisibile ? View.VISIBLE : View.GONE);
            }
        }
    }

    // set the tile titles text to white when a background is found - black by default
    public void updateTileTextTint() {
        hasBackground = true;

        for (int childCount = mRecyclerView.getChildCount(), i = 0; i < childCount; ++i) {
            final SpeedDialViewHolder holder = (SpeedDialViewHolder)mRecyclerView.getChildViewHolder(mRecyclerView.getChildAt(i));

            // show delete buttons in edit mode, but hide add button
            if (!holder.isAddBtn) {
                holder.textView.setTextColor(Color.WHITE);
                holder.imageView.setColorFilter(Color.BLACK);
            } else {
                holder.imageView.setColorFilter(Color.WHITE);
            }
        }
    }

    public void setDark() {
        mIsDarkEnabled = true;
    }

    /**
    * parse json result from fetchSponsoredSpeedDials()
    * display them in the adapter - applying title, url, and imageurl
    *
    */
    private void processSponsoredTiles(JSONArray quickLinks, ArrayList<SpeedDialDataItem> mSponsoredTiles) {
        try {
            for (int i = 0; i != mSponsoredTiles.size(); i++) {
                JSONObject speedDial = (JSONObject) quickLinks.get(i);

                int position = mSponsoredTiles.get(i).position;
                //mItems.indexOf(mSponsoredTiles.get(i));

                if (mSponsoredTiles.get(i).imageUrl == null || mSponsoredTiles.get(i).imageUrl.isEmpty()) {
                    // remove the old tile, add the new one, log the impression...
                    mItems.remove(position);
                    notifyItemRemoved(position);
                    mItems.add(position, new SpeedDialDataItem(speedDial.getString("brand"),
                            speedDial.getString("rurl"), position, true, false,
                            speedDial.getString("iurl"), speedDial.getString("impurl")));
                    notifyItemInserted(position);
                }

                // save the data to the database
                mDatabase.setSpeedDial(speedDial.getString("brand"), speedDial.getString("rurl"),
                    position, true, false, speedDial.getString("iurl"),
                    speedDial.getString("impurl"));
            }
        } catch (Exception e) { }
    }

    /**
    * get sponsored tiles from siteplug, save them to a sponsored tile
    */
    private void fetchSponsoredSpeedDials(final boolean isPopup) {
        // get number of sponsored tiles
        final ArrayList<SpeedDialDataItem> mSponsoredTiles = new ArrayList<>();
        for (int i = 0; i != mItems.size(); i++) {
            if (mItems.get(i).isSponsored) {
                mSponsoredTiles.add(mItems.get(i));
            }
        }
        // dont proceed if there arent any sponsored ones..
        if (mSponsoredTiles.size() == 0) return;

        final SharedPreferences mPrefs = ContextUtils.getAppSharedPreferences();

        boolean mSitePlugMaster = mPrefs.getBoolean("speed_dial_master", true);
        Set<String> mEnabledGeos;
        if (!mSitePlugMaster) {
            // Get user geo, if not in geo list stop and jump to getting default speed dials and from database
            mEnabledGeos = mPrefs.getStringSet("speed_dial_enabled_geos", null);

            String locale = detectSIMCountry(ContextUtils.getApplicationContext());
            if (locale.isEmpty() || !mEnabledGeos.contains(locale)) {
                // TODO TODODO getNonSponsoredSpeedDials();
                return;
            }
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                URL url;
                StringBuffer response = new StringBuffer();
                try {
                    //if (isPopup) {
                        // Quick Links for pop-up
                    //    url = new URL("http://poa39.siteplug.com/qlapi?o=poa39&s=61303&u=com.browser.tssomas&f=json&n=" + mSponsoredTiles.size() + "&i=1&is=96x96&di=" + deviceID + "&af=0");
                    //} else {
                        // Quick Links for homepage
                        url = new URL("http://poa39.siteplug.com/qlapi?o=poa39&s=46401&u=com.browser.tssomas&f=json&n=" + mSponsoredTiles.size() + "&i=1&is=96x96&di=" + deviceID + "&af=0");
                    //}
                } catch (MalformedURLException e) {
                    throw new IllegalArgumentException("invalid url");
                }

                HttpURLConnection conn = null;
                try {
                    conn = (HttpURLConnection) url.openConnection();
                    conn.setDoOutput(false);
                    conn.setConnectTimeout(4000);
                    conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("GET");
                    conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

                    // handle the response
                    int status = conn.getResponseCode();
                    if (status != 200) {
                        throw new IOException("Post failed with error code " + status);
                    } else {
                        BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                        String inputLine;
                        while ((inputLine = in.readLine()) != null) {
                            response.append(inputLine);
                        }
                        in.close();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    if (conn != null) {
                        conn.disconnect();
                    }

                    if (!(mRecyclerView.getContext() instanceof ChromeActivity)) return;
                    ((ChromeActivity)mRecyclerView.getContext()).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            JSONObject jsonObject = null;
                            try {
                               jsonObject = new JSONObject(response.toString());

                               JSONArray quickLinks = jsonObject.getJSONArray("data");

                               processSponsoredTiles(quickLinks, mSponsoredTiles);
                            } catch (JSONException e) {
                                e.printStackTrace();
                            }
                        }
                    });
                }
            }
        }).start();
    }

    private void logSponsoredTileImpression(final String impressionUrl) {
        if (impressionUrl != null && !impressionUrl.equals("")) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    URL url;
                    StringBuffer response = new StringBuffer();
                    try {
                        url = new URL(impressionUrl + "&di=" + deviceID);
                    } catch (MalformedURLException e) {
                        throw new IllegalArgumentException("invalid url");
                    }

                    HttpURLConnection conn = null;
                    try {
                        conn = (HttpURLConnection) url.openConnection();
                        conn.setDoOutput(false);
                        conn.setConnectTimeout(3000);
                        conn.setDoInput(true);
                        conn.setUseCaches(false);
                        conn.setRequestMethod("PUT");
                        conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

                        // handle the response
                        int status = conn.getResponseCode();
                        if (status != 200) {
                            throw new IOException("Post failed with error code " + status);
                        }
                    } catch (Exception e) {
                      e.printStackTrace();
                    } finally {
                        if (conn != null) {
                            conn.disconnect();
                        }
                    }
                }
            }).start();
        }
    }

    private String detectSIMCountry(Context context) {
        try {
            TelephonyManager telephonyManager = (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
            return telephonyManager.getSimCountryIso();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return detectNetworkCountry(context);
    }

    private String detectNetworkCountry(Context context) {
        try {
            TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            return telephonyManager.getNetworkCountryIso();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Simple example of a view holder that implements {@link ItemTouchHelperViewHolder} and has a
     * "handle" view that initiates a drag event when touched.
     */
    public static class SpeedDialViewHolder extends RecyclerView.ViewHolder implements
            ItemTouchHelperViewHolder {

        public final View view;
        public final TextView textView;
        public final ImageView imageView;
        public final ImageView deleteBtn;

        public boolean isAddBtn = false;

        public SpeedDialViewHolder(View itemView) {
            super(itemView);
            view = itemView;
            textView = (TextView) itemView.findViewById(R.id.speed_dial_tile_textview);
            imageView = (ImageView) itemView.findViewById(R.id.speed_dial_tile_view_icon);
            deleteBtn = (ImageView) itemView.findViewById(R.id.speed_dial_delete);
        }

        public void setAddBtn() {
            isAddBtn = true;
        }

        @Override
        public void onItemSelected() {
            //itemView.setBackgroundColor(Color.LTGRAY);
        }

        @Override
        public void onItemClear() {
            itemView.setBackgroundColor(0);
        }
    }
}

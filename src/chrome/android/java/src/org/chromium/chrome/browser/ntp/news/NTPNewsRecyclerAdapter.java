package org.chromium.chrome.browser.ntp.news;

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

import android.widget.FrameLayout;
import android.view.Gravity;
import java.lang.Runnable;
import android.os.Handler;
import org.json.JSONObject;

import android.widget.LinearLayout;
import android.util.TypedValue;

import org.json.JSONArray;

import org.chromium.chrome.browser.monetization.VeveBridge;
import org.chromium.chrome.browser.monetization.VeveUniversalObj;

import android.graphics.Color;
import java.lang.Character;

public class NTPNewsRecyclerAdapter extends RecyclerView.Adapter<NTPNewsRecyclerAdapter.ViewHolder> implements VeveBridge.VeveUniversalCommunicator {

    private ArrayList<Object> mData = new ArrayList<>();
    private ArrayList<NewsDataObject> mDataTemp = new ArrayList<>();
    private ArrayList<VeveUniversalObj> mUniversalData = new ArrayList<>();
    private LayoutInflater mInflater;

    private String mCountryCode;

    private static final String mCoindarToken = "70277:34tJkQia1LEWXFUTf5N";

    private static final String ARTICLE_IMAGE_URL_BASE = "https://hydrisapps.com/carbon/android-resources/article-image-getter/?key=ergnpiqg95pbwfewnfqewfk42939&url=";

    private SharedPreferences mPrefs;

    private boolean blockImageRefresh = false;

    private boolean mIsDarkMode;

    // data is passed into the constructor
    public NTPNewsRecyclerAdapter(Context context, boolean isDarkMode) {
        this.mInflater = LayoutInflater.from(context);

        this.mIsDarkMode = isDarkMode;

        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        long mTimeCached = mPrefs.getLong("time_news_cached", 0);
        long mCurrentTime = System.currentTimeMillis();
        if (mCurrentTime - mTimeCached >= 3600000) {
            // been cached for more than an hour, load new
            getCountryCode();
        } else {
            final String mCachedNews = mPrefs.getString("cached_news", "");
            if (mCachedNews.equals("")) {
                getCountryCode();
            } else {
                try {
                    DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
                    InputSource src = new InputSource();
                    src.setCharacterStream(new StringReader(mCachedNews));

                    Document doc = builder.parse(src);
                    NodeList nodes = doc.getElementsByTagName("item");

                    for (int i = 0; i < nodes.getLength(); i++) {
                        NodeList childNodes = nodes.item(i).getChildNodes();
                        // if (childNodes.getNodeType() == Node.ELEMENT_NODE) {
                            String imageUrl = "";
                            for (int z = 0; z < childNodes.getLength(); z++) {
                              Element el = (Element)childNodes.item(z);
                              if (el.hasAttribute("url")) {
                                  imageUrl = el.getAttribute("url");
                              }
                            }

                            Element element = (Element) nodes.item(i);

                            String title = getValue("title", element);
                            String url = getValue("link", element);
                            // String imageUrl = getValue("media:content", element);
                            // String imageGetterUrl = ARTICLE_IMAGE_URL_BASE + url;
                            String publisher = getValue("source", element);
                            String date = getValue("pubDate", element);
                            if (date != null) {
                              date = date.replaceAll("\\+0000", "");
                            }

                            NewsDataObject dataObj = new NewsDataObject(url, imageUrl, title, publisher, date, null);

                            mData.add(dataObj);

                            // fetchImagesFromGetter(i, imageGetterUrl);
                            notifyItemInserted(mData.size() - 1);
                        // }
                    }
                } catch (Exception ignore) {
                    getCountryCode();
                }
            }
        }

        VeveBridge.getInstance().getUniversalAds((VeveBridge.VeveUniversalCommunicator)this);
    }

    @Override
    public void onUniversalAdsReceived(ArrayList<VeveUniversalObj> ads) {
        blockImageRefresh = true;
        for (int i = 0; i != ads.size(); i++) {

            if (i == 0) {
                if (mData.size() == 0) break;

                mData.add(1, ads.get(i));

                notifyItemInserted(1);
            } else if (i == 1 && mData.size() >= 8) {
                if (mData.size() <= 8) break;

                mData.add(8, ads.get(i));

                notifyItemInserted(8);
            } else if (i == 2 && mData.size() >= 16) {
                if (mData.size() <= 16) break;

                mData.add(16, ads.get(i));

                notifyItemInserted(16);
            } else if (i == 3 && mData.size() >= 24) {
                if (mData.size() <= 24) break;

                mData.add(24, ads.get(i));

                notifyItemInserted(24);
            } else if (i == 4 && mData.size() >= 32) {
                if (mData.size() <= 32) break;

                mData.add(32, ads.get(i));

                notifyItemInserted(32);
            }
        }

        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                //notifyDataSetChanged();
            }
        }, 1000);
    }

    private void getCountryCode() {
        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

        long mCurrentTime = System.currentTimeMillis();
        long mTimeCached = mPrefs.getLong("time_geo_cached", 0);
        if (mCurrentTime - mTimeCached < 604800000) {
              mCountryCode = mPrefs.getString("geoplugin_countryCode", null);
              if (mCountryCode != null) {
                  getNewsData();
                  return;
              }
        }

        final String countryEndpointUrl = "http://www.geoplugin.net/json.gp";
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                String response = null;

                try {
                    URL connectURL = new URL(countryEndpointUrl);
                    BufferedReader in = new BufferedReader(
                    new InputStreamReader(connectURL.openStream()));

                    String inputLine;
                    StringBuilder sb = new StringBuilder();
                    while ((inputLine = in.readLine()) != null){
                        sb.append(inputLine);
                    }
                    response = sb.toString();

                    in.close();
                } catch (Exception ignore) { }

                return response;
            }

            @Override
            protected void onPostExecute(String result) {
                if (result != null) {
                    try {
                        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

                        JSONObject jsonObject = new JSONObject(result);
                        mCountryCode = jsonObject.getString("geoplugin_countryCode");

                        long mCurrentTime = System.currentTimeMillis();
                        mPrefs.edit().putLong("time_geo_cached", mCurrentTime).apply();
                        mPrefs.edit().putString("geoplugin_countryCode", mCountryCode).apply();

                        getNewsData();
                    } catch (Exception ignore) {}
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void getNewsData() {
        final String countryEndpointUrl = "https://rss.app/feeds/_J7ORk9Ca44w7rT6f.xml";
        // "https://coindar.org/api/v2/events?access_token=" + mCoindarToken + "&page=1&page_size=30&sort_by=date_added&order_by=1";
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                String response = null;

                try {
                    URL connectURL = new URL(countryEndpointUrl);
                    BufferedReader in = new BufferedReader(
                    new InputStreamReader(connectURL.openStream()));

                    String inputLine;
                    StringBuilder sb = new StringBuilder();
                    while ((inputLine = in.readLine()) != null){
                        sb.append(inputLine);
                    }
                    response = sb.toString();
                    in.close();
                } catch (Exception ignore) {}

                return response;
            }

            @Override
            protected void onPostExecute(String result) {
                if (result != null) {
                    try {
                        DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
                        InputSource src = new InputSource();
                        src.setCharacterStream(new StringReader(result));

                        Document doc = builder.parse(src);
                        NodeList nodes = doc.getElementsByTagName("item");

                        if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();

                        long mCurrentTime = System.currentTimeMillis();
                        mPrefs.edit().putLong("time_news_cached", mCurrentTime).apply();
                        mPrefs.edit().putString("cached_news", result).apply();

                        for (int i = 0; i < nodes.getLength(); i++) {
                            NodeList childNodes = nodes.item(i).getChildNodes();
                            // if (childNodes.getNodeType() == Node.ELEMENT_NODE) {
                                String imageUrl = "";
                                for (int z = 0; z < childNodes.getLength(); z++) {
                                  Element el = (Element)childNodes.item(z);
                                  if (el.hasAttribute("url")) {
                                      imageUrl = el.getAttribute("url");
                                  }
                                }

                                Element element = (Element) nodes.item(i);

                                String title = getValue("title", element);
                                String url = getValue("link", element);
                                // String imageUrl = getValue("media:content", element);
                                // String imageGetterUrl = ARTICLE_IMAGE_URL_BASE + url;
                                String publisher = getValue("source", element);
                                String date = getValue("pubDate", element);
                                if (date != null) {
                                  date = date.replaceAll("\\+0000", "");
                                }

                                NewsDataObject dataObj = new NewsDataObject(url, imageUrl, title, publisher, date, null);

                                mData.add(dataObj);

                                // fetchImagesFromGetter(i, imageGetterUrl);
                                notifyItemInserted(mData.size() - 1);
                            // }
                        }

                        // JSONArray mJsonArray = new JSONArray(result);
                        // if (mPrefs == null) mPrefs = ContextUtils.getAppSharedPreferences();
                        //
                        // long mCurrentTime = System.currentTimeMillis();
                        // mPrefs.edit().putLong("time_news_cached", mCurrentTime).apply();
                        // mPrefs.edit().putString("cached_news", result).apply();
                        //
                        // for (int i = 0; i < mJsonArray.length(); i++) {
                        //     JSONObject mItem = mJsonArray.getJSONObject(i);
                        //
                        //     String title = mItem.getString("caption");
                        //     String url = mItem.getString("source");
                        //
                        //     String imageGetterUrl = ARTICLE_IMAGE_URL_BASE + url;
                        //     String publisher = "";//getValue("source", element);
                        //     String date = mItem.getString("date_public");
                        //
                        //     // get Image Url
                        //     String imageUrl = "";
                        //     for (int j = url.length() - 1; j != 0; j--) {
                        //       if (Character.toString(url.charAt(j)).equals("-")) {
                        //         imageUrl = "https://coindar.org/images/events/" + url.substring(j + 1) + "_en.jpg";
                        //         break;
                        //       }
                        //     }
                        //
                        //     NewsDataObject dataObj = new NewsDataObject(url, imageUrl, title, publisher, date, imageGetterUrl);
                        //
                        //     mData.add(dataObj);
                        //     // mDataTemp.add(dataObj);
                        //
                        //     notifyItemInserted(mDataTemp.size() - 1);
                        //
                        //     // disable images, coindar doesnt do them
                        //     // fetchImagesFromGetter(i, imageGetterUrl);
                        // }
                    } catch(Exception ignore) { }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void fetchImagesFromGetter(final int index, final String url) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                String response = null;

                try {
                    URL connectURL = new URL(url);
                    BufferedReader in = new BufferedReader(
                    new InputStreamReader(connectURL.openStream()));

                    String inputLine;
                    StringBuilder sb = new StringBuilder();
                    while ((inputLine = in.readLine()) != null){
                        sb.append(inputLine);
                    }
                    response = sb.toString();

                    in.close();
                } catch (Exception ignore) { }

                return response;
            }

            @Override
            protected void onPostExecute(String result) {
                if (result != null) {
                    NewsDataObject dataObj = mDataTemp.get(index);
                    dataObj.imageUrl = result;
                    mData.add(dataObj);

                    notifyItemInserted(mData.size() - 1);
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private String getValue(String tag, Element element) {
        try {
          NodeList nodes = element.getElementsByTagName(tag).item(0).getChildNodes();
          Node node = (Node) nodes.item(0);
          return node.getTextContent();
        } catch (Exception ignore) {
          return "";
        }
    }

    // inflates the row layout from xml when needed
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        if (viewType == 0) {
            View view = mInflater.inflate(R.layout.ntp_news_universal_ad_item, parent, false);
            return new ViewHolder(view);
        } else {
            View view = mInflater.inflate(R.layout.ntp_news_row_item, parent, false);
            return new ViewHolder(view);
        }
    }

    // binds the data to the TextView in each row
    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        String textColor = mIsDarkMode ? "#ffffff" : "#000000";

        try {
            View view = holder.itemView;

            if (getItemViewType(position) == 0) {
                LinearLayout container = view.findViewById(R.id.universal_ad_container);

                int height = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 140, view.getResources().getDisplayMetrics());

                ViewGroup.LayoutParams params = container.getLayoutParams();
                params.height = height;
                container.setLayoutParams(params);

                view.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                      try {
                        if (view.getContext() instanceof ChromeActivity) {
                            LoadUrlParams loadUrlParams = new LoadUrlParams(((VeveUniversalObj) mData.get(holder.getAdapterPosition())).url);
                            ChromeActivity activity = (ChromeActivity) view.getContext();
                            activity.getActivityTab().loadUrl(loadUrlParams);
                        }
                      } catch (Exception ignore) {}
                    }
                });

                TextView titleTextView = view.findViewById(R.id.ntp_universal_ad_item_title);
                titleTextView.setText(((VeveUniversalObj) mData.get(position)).title);
                titleTextView.setTextColor(Color.parseColor(textColor));

                TextView descTextView = view.findViewById(R.id.ntp_universal_ad_item_description);
                descTextView.setText(((VeveUniversalObj) mData.get(position)).description);
                descTextView.setTextColor(Color.parseColor(textColor));

                String imageUrl = ((VeveUniversalObj) mData.get(position)).imgUrl;
                if (imageUrl != null) {
                    final ImageView imageView = view.findViewById(R.id.ntp_universal_ad_item_image);
                    final float density = view.getContext().getResources().getDisplayMetrics().density;
                    final int valueInDp = (int)(20 * density);
                    Glide.with(imageView)
                        .load(imageUrl)
                        //.thumbnail(0.05f)
                        .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                        .fitCenter()
                        .apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
                        .transform(new RoundedCorners(valueInDp))
                        .into(new CustomTarget<Drawable>() {
                            @Override
                            public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                                if (imageView.getDrawable() == null)
                                    imageView.setImageDrawable(resource);
                            }

                            @Override
                            public void onLoadCleared(@Nullable Drawable placeholder) {

                            }
                        });
                }

                VeveBridge.getInstance().logImpression(((VeveUniversalObj) mData.get(position)).impurl);

                return;
            }

            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                  try {
                      if (view.getContext() instanceof ChromeActivity) {
                          LoadUrlParams loadUrlParams = new LoadUrlParams(((NewsDataObject) mData.get(holder.getAdapterPosition())).articleUrl);
                          ChromeActivity activity = (ChromeActivity) view.getContext();
                          activity.getActivityTab().loadUrl(loadUrlParams);
                      }
                  } catch (Exception ignore) {}
                }
            });

            TextView titleTextView = view.findViewById(R.id.ntp_news_item_title);
            titleTextView.setText(((NewsDataObject) mData.get(position)).articleTitle);
            titleTextView.setTextColor(Color.parseColor(textColor));

            String imageUrl = ((NewsDataObject) mData.get(position)).imageUrl;
            if (imageUrl != null) {
                final ImageView imageView = view.findViewById(R.id.ntp_news_item_image);
                final float density = view.getContext().getResources().getDisplayMetrics().density;
                final int valueInDp = (int)(20 * density);
                Glide.with(imageView)
                    .load(imageUrl)
                    //.thumbnail(0.05f)
                    .diskCacheStrategy(DiskCacheStrategy.AUTOMATIC)
                    .fitCenter()
                    .apply(new RequestOptions().override((int)(125 * density), (int)(100 * density)))
                    .transform(new RoundedCorners(valueInDp))
                    .into(new CustomTarget<Drawable>() {
                        @Override
                        public void onResourceReady(Drawable resource, @Nullable Transition<? super Drawable> transition) {
                            if (imageView.getDrawable() == null)
                                imageView.setImageDrawable(resource);
                        }

                        @Override
                        public void onLoadCleared(@Nullable Drawable placeholder) {

                        }
                    });
            }

            TextView publisherTextView = view.findViewById(R.id.ntp_news_item_publisher);
            publisherTextView.setText(((NewsDataObject) mData.get(position)).publisher);
            publisherTextView.setTextColor(Color.parseColor(textColor));

            TextView publishDateTextView = view.findViewById(R.id.ntp_news_item_publishdate);
            String articleDate = ((NewsDataObject) mData.get(position)).articleDate;
            publishDateTextView.setText(articleDate);
            publishDateTextView.setTextColor(Color.parseColor(textColor));

        } catch (Exception ignore) { }
    }

    // total number of rows
    @Override
    public int getItemCount() {
        // if (mData.size() >= 15) {
        //     return 15;
        // }
        return mData.size();
    }

    @Override
    public int getItemViewType(int position) {
        boolean isAd = mData.get(position) instanceof VeveUniversalObj;
        if (isAd) {
            return 0;
        }
        return 1;
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

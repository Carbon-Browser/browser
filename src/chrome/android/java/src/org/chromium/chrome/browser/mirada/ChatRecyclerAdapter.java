package org.chromium.chrome.browser.mirada;

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
import android.content.res.ColorStateList;
import android.app.Activity;
// import com.amplitude.api.Amplitude;
import android.os.Looper;

public class ChatRecyclerAdapter extends RecyclerView.Adapter<ChatRecyclerAdapter.ViewHolder> implements ChatInterface {

    private ArrayList<ChatDataObj> mData = new ArrayList<>();

    private boolean isLightTheme;

    private MiradaActivityInterface mCommunicator;

    private LayoutInflater mInflater;

    private StreamingService mStreamingService = new StreamingService();

    private Activity mActivity;

    // data is passed into the constructor
    public ChatRecyclerAdapter(Activity activity, boolean isLightTheme, MiradaActivityInterface communicator) {
        this.mInflater = LayoutInflater.from(activity);

        this.isLightTheme = isLightTheme;

        this.mCommunicator = communicator;
    }

    // inflates the row layout from xml when needed
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = mInflater.inflate(R.layout.mirada_chat_row_item, parent, false);

        return new ViewHolder(view, viewType);
    }

    // binds the data to the TextView in each row
    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        holder.onBindViewHolder(position);
    }

    // total number of rows
    @Override
    public int getItemCount() {
        return mData.size();
    }

    @Override
    public int getItemViewType(int position) {
        if (mData.get(position).isUserData) {
            return 0;
        }
        return 1;
    }

    @Override
    public void onNewChatClicked() {
        mData.clear();
        notifyDataSetChanged();
    }

    @Override
    public void onReceivedPrompt(String prompt) {
        try {
            SharedPreferences pref = ContextUtils.getAppSharedPreferences();

            // Amplitude.getInstance().logEventAsync("mirada_ai_request_event");

            mData.add(new ChatDataObj("id", true, prompt, false, true));

            notifyDataSetChanged();

            JSONArray messages = new JSONArray();

            for (int i = 0; i != mData.size(); i++) {
                JSONObject messageItem = new JSONObject();
                messageItem.put("role", mData.get(i).isUserData ? "user" : "assistant");
                messageItem.put("content", mData.get(i).text);
                messages.put(messageItem);
            }

            // add the newest message
            JSONObject messageItem = new JSONObject();
            messageItem.put("role", "user");
            messageItem.put("content", prompt);
            messages.put(messageItem);

            // Create the root object and add properties to it
            JSONObject root = new JSONObject();
            root.put("messages", messages);
            root.put("stream", true);

            String data = root.toString();

            // Convert the root object to a JSON string
            String jsonString = root.toString();
            mStreamingService.postStreamRequest("https://app.mirada.ai/api/mirada/external", data, null, new StreamingService.StreamHandler() {
                StringBuilder characterBuffer = new StringBuilder();
                Handler uiHandler = new Handler(Looper.getMainLooper());
                Runnable characterProcessor;
                boolean streamEnded = false;
                int currentDataIndex = -1; // Tracks the current index of mData being updated

                @Override
                public void onChunkReceived(String chunk) {
                    try {
                        String chunkString = chunk.replaceAll("data: ", "");
                        if (chunkString.isEmpty()) return;

                        synchronized (this) { // Ensure thread-safety
                            int index = currentDataIndex;//findDataObjById("id");
                            if (index != -1) {
                                currentDataIndex = index;
                            } else {
                                ChatDataObj newDataObj = new ChatDataObj("id", false, "", true, true);
                                mData.add(newDataObj);
                                currentDataIndex = mData.size() - 1;
                            }

                            characterBuffer.append(chunkString);
                        }
                        processBuffer();
                        mCommunicator.onReceivedPromptChunk();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }

                private int findDataObjById(String id) {
                    for (int i = 0; i < mData.size(); i++) {
                        if (mData.get(i).id.equals(id)) {
                            return i;
                        }
                    }
                    return -1;
                }

                private void processBuffer() {
                    if (characterProcessor == null) {
                        characterProcessor = new Runnable() {
                            @Override
                            public void run() {
                                if (characterBuffer.length() > 0) {
                                    char nextChar = characterBuffer.charAt(0);
                                    characterBuffer.deleteCharAt(0);
                                    updateUIWithChar(nextChar);
                                }
                                // Continue processing or complete the process
                                if (!streamEnded || characterBuffer.length() > 0) {
                                    uiHandler.postDelayed(this, 5);
                                } else {
                                    checkCompletion();
                                }
                            }
                        };
                        uiHandler.post(characterProcessor);
                    }
                }

                private void updateUIWithChar(char ch) {
                    synchronized (this) {
                        if (currentDataIndex != -1) {
                            ChatDataObj dataObj = mData.get(currentDataIndex);
                            dataObj.text += ch;
                            notifyItemChanged(currentDataIndex);
                        }
                    }
                }

                @Override
                public void onComplete() {
                    streamEnded = true;
                }

                private void checkCompletion() {
                    if (characterBuffer.length() == 0) {
                        synchronized (this) {
                            if (streamEnded && characterProcessor != null) {
                                uiHandler.removeCallbacks(characterProcessor);
                                characterProcessor = null;
                                mCommunicator.onFinishLoading();
                            }
                        }
                    }
                }

                @Override
                public void onError(Exception e) {
                    streamEnded = true;
                    checkCompletion();
                    // Log and handle the error
                    mCommunicator.onFinishLoading();
                }
            });
        } catch (Exception ignore) {
          // TODO proper error handling
          ignore.printStackTrace();
        }
    }

    // stores and recycles views as they are scrolled off screen
    public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        int viewType;

        ViewHolder(View itemView, int viewType) {
            super(itemView);

            this.viewType = viewType;
        }

        public void onBindViewHolder(int position) {
            String textColor = isLightTheme ? "#000000" : "#ffffff";

            try {
                View view = itemView;

                ChatDataObj dataObj = mData.get(position);

                TextView text = view.findViewById(R.id.text);
                text.setTextColor(Color.parseColor(textColor));
                text.setText(dataObj.text);

                View icon = view.findViewById(R.id.logo_view);
                icon.setBackground(view.getContext().getResources().getDrawable(viewType == 0 ? R.drawable.ic_user_mirada : R.drawable.ic_mirada_logo_round));

                if (viewType == 0) {
                  int iconColor = Color.parseColor(isLightTheme ? "#262626" : "#ffffff");
                  icon.setBackgroundTintList(ColorStateList.valueOf(iconColor));

                  String backgroundColor = isLightTheme ? "#F9F9F9" : "#3E3E3E";
                  LinearLayout container = view.findViewById(R.id.container);
                  container.setBackgroundColor(Color.parseColor(backgroundColor));
                }
            } catch (Exception ignore) { }
        }

        @Override
        public void onClick(View view) {

        }
    }
}

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

    private String id = "";
    private String tk = "";
    private String instr = "";
    private String model = "";

    // data is passed into the constructor
    public ChatRecyclerAdapter(Activity activity, boolean isLightTheme, MiradaActivityInterface communicator) {
        this.mInflater = LayoutInflater.from(activity);

        this.isLightTheme = isLightTheme;

        this.mCommunicator = communicator;

        SharedPreferences pref = ContextUtils.getAppSharedPreferences();
        id = pref.getString("miradarc_id", "vllm-98r21ddkytvdh6");
        model = pref.getString("mirada_model", "casperhansen/llama-3-70b-instruct-awq");
        tk = pref.getString("miradarc_tk", "B13LSOMXXUQ095YEIJF5EAAH6SLEHDYSO3GAHJ51");
        instr = pref.getString("miradarc_instr", "The following is a conversation between a highly knowledgeable and intelligent AI assistant, called Mirada AI, and a human user, called User. Your name is \"Mirada AI\". In the following interactions, User and Mirada AI will converse in natural language, and Mirada AI will answer User's questions. Mirada AI was built to be respectful, polite and truthful. Mirada AI was built by Mirada Labs, a decentralized team of specialists. Mirada AI will never decline to answer a question, and always attempts to give an answer that User would be satisfied with. It knows a lot, and always tells the truth. Mirada AI is a decentralized AI platform with the goal of truth-seeking and honesty, powered by the crypto token $MIRX. Continue the chat dialogue below.");
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

            // add the system message
            JSONObject systemMessage = new JSONObject();
            systemMessage.put("role", "system");
            systemMessage.put("content", instr);
            messages.put(systemMessage);

            String appendString = pref.getString("miradarc_append", null);
            if (appendString != null) {
                try {
                  JSONArray appendArray = new JSONArray(appendString);
                  for (int i = 0; i != appendArray.length(); i++) {
                      JSONObject messageItem = appendArray.getJSONObject(i);
                      messageItem.put("role", messageItem.getString("role"));
                      messageItem.put("content", messageItem.getString("content"));
                      messages.put(messageItem);
                  }
                } catch (Exception ignore) {}
            }

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
            root.put("max_tokens", 400);
            root.put("stream", true);
            // root.put("user", "");
            // root.put("name1", "");
            // root.put("name2", "Mirada AI");
            // root.put("user_bio", "");

            JSONArray stopTokenIds = new JSONArray();
            stopTokenIds.put(128009);  // Add the integer to the JSONArray
            root.put("stop_token_ids", stopTokenIds);

            // root.put("mode", "chat-instruct");
            // root.put("instruction_template", "Mistral");
            root.put("repetition_penalty", 1.3);
            // root.put("negative_prompt", "wokeness, wokism, generalizations");
            // root.put("length_penalty", 1.2);
            root.put("temperature", 0.5);
            root.put("model", model);

            String data = root.toString();

            // Convert the root object to a JSON string
            String jsonString = root.toString();
            mStreamingService.postStreamRequest("https://api.runpod.ai/v2/" + id + "/openai/v1/chat/completions", data, tk, new StreamingService.StreamHandler() {
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

                        JSONObject jsonResult = new JSONObject(chunkString);
                        String id = jsonResult.getString("id");
                        JSONArray arr = jsonResult.getJSONArray("choices");
                        JSONObject obj = arr.getJSONObject(0);
                        JSONObject choices = obj.getJSONObject("delta");
                        String content = choices.getString("content");

                        synchronized (this) { // Ensure thread-safety
                            int index = findDataObjById(id);
                            if (index != -1) {
                                currentDataIndex = index;
                            } else {
                                ChatDataObj newDataObj = new ChatDataObj(id, false, "", true, true);
                                mData.add(newDataObj);
                                currentDataIndex = mData.size() - 1;
                            }

                            characterBuffer.append(content);
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

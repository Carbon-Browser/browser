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
            mData.add(new ChatDataObj("id", true, prompt, false));

            notifyDataSetChanged();

            JSONArray messages = new JSONArray();

            // add the system message
            JSONObject systemMessage = new JSONObject();
            systemMessage.put("role", "system");
            systemMessage.put("content", "The following is a conversation between a highly knowledgeable and intelligent AI assistant, called Mirada AI, and a human user, called User. Your name is \"Mirada AI\". In the following interactions, User and Mirada AI will converse in natural language, and Mirada AI will answer User's questions. Mirada AI was built to be respectful, polite and truthful. Mirada AI was built by Mirada Labs, a decentralized team of specialists. Mirada AI will never decline to answer a question, and always attempts to give an answer that User would be satisfied with. It knows a lot, and always tells the truth. Mirada AI is a decentralized AI platform with the goal of truth-seeking and honesty, powered by the crypto token $MIRX. Carbon Browser is the fastest web3 browser powered by its crypto token $CSIX. Carbon Browser was built by Carbon X Labs. Continue the chat dialogue below.");
            messages.put(systemMessage);

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
            root.put("max_tokens", 250);
            root.put("stream", true);
            root.put("user", "");
            root.put("name1", "");
            root.put("name2", "Mirada AI");
            root.put("user_bio", "");
            root.put("mode", "chat-instruct");
            root.put("instruction_template", "Mistral");
            root.put("repetition_penalty", 1);
            root.put("negative_prompt", "wokeness, wokism, generalizations");
            root.put("length_penalty", 1.2);

            String data = root.toString();


            // Convert the root object to a JSON string
            String jsonString = root.toString();
            mStreamingService.postStreamRequest("https://eikrx51v929qmu-5000.proxy.runpod.net/v1/chat/completions?token=6gc6ql5und28xq8yb8a8", data, new StreamingService.StreamHandler() {
              @Override
              public void onChunkReceived(String chunk) {
                  try {
                      String chunkString = chunk.replaceAll("data: ", "");

                      if (chunkString == null || chunkString.isEmpty()) return;

                      JSONObject jsonResult = new JSONObject(chunkString);
                      String id = jsonResult.getString("id");
                      JSONArray arr = jsonResult.getJSONArray("choices");
                      JSONObject obj = arr.getJSONObject(0);
                      JSONObject choices = obj.getJSONObject("delta");
                      String role = choices.getString("role");
                      String content = choices.getString("content");

                      int index = mData.size() - 1;
                      ChatDataObj newDataObj = mData.get(index);

                      if (newDataObj.id.equals(id)) {
                          // stream already exists, continue
                          String oldText = newDataObj.text;
                          newDataObj.text = oldText + content;
                          mData.set(index, newDataObj);
                      } else {
                          // make new data obj
                          mData.add(new ChatDataObj(id, false, content, true));
                      }
                  } catch (Exception e) {
                      e.printStackTrace();
                  }

                  mCommunicator.onReceivedPromptChunk();
              }

              @Override
              public void onComplete() {
                  try {
                      int index = mData.size() - 1;
                      ChatDataObj finishLoadingObj = mData.get(index);
                      finishLoadingObj.isLoading = false;
                      mData.set(index, finishLoadingObj);
                      notifyDataSetChanged();
                  } catch (Exception ignore) { }

                  mCommunicator.onFinishLoading();
              }

              @Override
              public void onError(Exception e) {
                  mCommunicator.onFinishLoading();
                  e.printStackTrace();
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

package org.chromium.chrome.browser.mirada;

import org.chromium.chrome.browser.ChromeBaseAppCompatActivity;
import android.os.Bundle;
import org.chromium.chrome.R;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.night_mode.ThemeType;
import androidx.recyclerview.widget.RecyclerView;
import android.app.Activity;
import android.graphics.Color;
import android.widget.LinearLayout;
import android.graphics.PorterDuff;
import android.content.res.ColorStateList;
import android.widget.PopupMenu;
import android.view.MenuItem;
import android.view.KeyEvent;
import androidx.recyclerview.widget.LinearLayoutManager;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;
import android.content.Intent;

public class MiradaActivity extends ChromeBaseAppCompatActivity implements MiradaActivityInterface {

    private boolean isLightTheme = false;

    private static final String colorDarkText = "#ffffff";
    private static final String colorLightText = "#000000";

    private static final String colorDark = "#262626";
    private static final String colorLight = "#ffffff";

    private boolean isLoading = false;

    private ChatInterface mChatInterface;

    private RecyclerView recyclerView;
    private ChatRecyclerAdapter chatRecyclerAdapter;

    public MiradaActivity() { }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        isLightTheme = ChromeSharedPreferences.getInstance().readInt("ui_theme_setting", ThemeType.LIGHT) == ThemeType.LIGHT;
        setContentView(R.layout.mirada_activity);

        final String textColor = isLightTheme ? colorLightText : colorDarkText;
        final String backgroundColor = isLightTheme ? colorLight : colorDark;
        final int buttonColor = Color.parseColor(isLightTheme ? colorDark : colorLight);

        LinearLayout mainContainer = findViewById(R.id.container);
        mainContainer.setBackgroundColor(Color.parseColor(backgroundColor));

        ImageView logoView = findViewById(R.id.logo);
        logoView.setImageResource(isLightTheme ? R.drawable.ic_mirada_logo_long_light : R.drawable.ic_mirada_logo_long);

        chatRecyclerAdapter = new ChatRecyclerAdapter(MiradaActivity.this, isLightTheme, this);
        mChatInterface = (ChatInterface)chatRecyclerAdapter;

        Button menuButton = findViewById(R.id.overflow_menu);
        menuButton.setBackgroundTintList(ColorStateList.valueOf(buttonColor));
        menuButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
              PopupMenu popup = new PopupMenu(MiradaActivity.this, view);
              popup.getMenuInflater().inflate(R.menu.mirada_chat, popup.getMenu());
              popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                  @Override
                  public boolean onMenuItemClick(MenuItem item) {
                      int id = item.getItemId();
                      if (id == R.id.new_chat_id) {
                          mChatInterface.onNewChatClicked();
                          isLoading = false;
                      }
                      if (id == R.id.follow_on_x_id) {
                          Intent returnIntent = new Intent();
                          returnIntent.putExtra("miradaResponse", "follow_on_x_id");
                          setResult(Activity.RESULT_OK, returnIntent);
                          finish();
                      }
                      if (id == R.id.visit_website_id) {
                          Intent returnIntent = new Intent();
                          returnIntent.putExtra("miradaResponse", "visit_website_id");
                          setResult(Activity.RESULT_OK, returnIntent);
                          finish();
                      }
                      if (id == R.id.image_generation_id) {
                          Intent returnIntent = new Intent();
                          returnIntent.putExtra("miradaResponse", "image_generation_id");
                          setResult(Activity.RESULT_OK, returnIntent);
                          finish();
                      }
                      return true;
                  }
              });
              popup.show();
            }
        });

        LinearLayout inputContainer = findViewById(R.id.input_container);
        int backgroundResource = isLightTheme ? R.drawable.mirada_input_light : R.drawable.mirada_input_dark;
        inputContainer.setBackground(getResources().getDrawable(backgroundResource));

        EditText input = findViewById(R.id.input);
        input.setTextColor(Color.parseColor(textColor));
        input.setHintTextColor(Color.parseColor("#ABABAB"));
        input.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                // Check if the event is a key-down event on the "Enter" button
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                    (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    // Perform your action on key press here
                    submitPrompt(input);
                    return true; // Return true because you handled the event
                }
                return false; // Return false if you haven't handled the event
            }
        });

        Button sendButton = findViewById(R.id.button_send);
        sendButton.setBackgroundTintList(ColorStateList.valueOf(buttonColor));
        sendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                submitPrompt(input);
            }
        });

        View backButton = findViewById(R.id.mirada_back_button);
        backButton.setBackgroundTintList(ColorStateList.valueOf(buttonColor));
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setResult(Activity.RESULT_OK, null);
                finish();
            }
        });

        recyclerView = findViewById(R.id.recycler_view);
        recyclerView.setLayoutManager(new LinearLayoutManager(MiradaActivity.this));
        recyclerView.setAdapter(chatRecyclerAdapter);

        try {
            Intent intent = getIntent();
            String searchQuery = intent.getStringExtra("search_query");

            if (searchQuery != null && !searchQuery.isEmpty()) {
                isLoading = true;
                mChatInterface.onReceivedPrompt(searchQuery);
            }
        } catch (Exception ignore) {}
    }

    private void submitPrompt(EditText input) {
        if (isLoading) return;

        try {
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(input.getWindowToken(), 0);

            String text = input.getText().toString();
            input.setText("");

            isLoading = true;

            mChatInterface.onReceivedPrompt(text);
        } catch (Exception ingore) { }
    }

    @Override
    public void onReceivedPromptChunk() {
      runOnUiThread(new Runnable() {
          @Override
          public void run() {
              recyclerView.requestLayout();
              chatRecyclerAdapter.notifyDataSetChanged();

              try {
                  if (recyclerView.getAdapter() != null) {
                      recyclerView.smoothScrollToPosition(recyclerView.getAdapter().getItemCount() - 1);
                  }
              } catch (Exception ignore) {}
          }
      });
    }

    @Override
    public void onFinishLoading() {
        isLoading = false;
        recyclerView.invalidate();
    }
}

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

public class MiradaActivity extends ChromeBaseAppCompatActivity {

    private boolean isLightTheme = true;

    private static final String colorDarkText = "#ffffff";
    private static final String colorLightText = "#000000";

    private static final String colorDark = "#262626";
    private static final String colorLight = "#ffffff";

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
                          // TODO clear chat
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

        RecyclerView recyclerView = findViewById(R.id.recycler_view);
    }

    private void submitPrompt(EditText input) {
        String text = input.getText().toString();
        input.setText("");
        // TODO send to api etc 
    }
}

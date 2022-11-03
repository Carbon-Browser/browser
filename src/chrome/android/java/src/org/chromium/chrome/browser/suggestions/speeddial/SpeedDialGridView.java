package org.chromium.chrome.browser.suggestions.speeddial;

import android.os.Bundle;
import android.support.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.ItemTouchHelper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.content.Context;

import org.chromium.chrome.browser.suggestions.speeddial.helper.OnStartDragListener;
import org.chromium.chrome.browser.suggestions.speeddial.helper.SimpleItemTouchHelperCallback;

import org.chromium.chrome.browser.suggestions.speeddial.SpeedDialInteraction;
import org.chromium.chrome.R;

/**
 * @author Paul Burke (ipaulpro)
 */
public class SpeedDialGridView extends RecyclerView implements OnStartDragListener {

    private ItemTouchHelper mItemTouchHelper;

    public SpeedDialGridView(Context context, int nColumns, SpeedDialInteraction speedDialInteraction,
            boolean isPopup) {
        super(context);
        final SpeedDialAdapter adapter = new SpeedDialAdapter(context, this, speedDialInteraction,
                isPopup);

        this.setHasFixedSize(true);
        this.setAdapter(adapter);

        //this.setBackground(context.getResources().getDrawable(R.drawable.ntp_rounded_dark_background_opacity));

        // float scale = getResources().getDisplayMetrics().density;
        // int dpAsPixels = (int) (8*scale + 0.5f);
        // this.setPadding(0, dpAsPixels, 0, dpAsPixels);

        final GridLayoutManager layoutManager = new GridLayoutManager(context, nColumns);
        this.setLayoutManager(layoutManager);

        ItemTouchHelper.Callback callback = new SimpleItemTouchHelperCallback(adapter);
        mItemTouchHelper = new ItemTouchHelper(callback);
        mItemTouchHelper.attachToRecyclerView(this);
    }

    @Override
    public void onStartDrag(RecyclerView.ViewHolder viewHolder) {
        mItemTouchHelper.startDrag(viewHolder);
    }

    public void updateTileTextTint() {
        SpeedDialAdapter adapter = (SpeedDialAdapter) getAdapter();
        adapter.updateTileTextTint();
    }

    public void setDark() {
        SpeedDialAdapter adapter = (SpeedDialAdapter) getAdapter();
        adapter.setDark();
    }
}

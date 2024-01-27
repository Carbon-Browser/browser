package org.chromium.chrome.browser.ntp;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import org.chromium.chrome.browser.ntp.TokenTrackerObj;
import java.util.List;
import android.widget.Filter;
import android.widget.Filterable;
import java.util.ArrayList;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import org.chromium.chrome.R;

public class TokenTrackerAdapter extends RecyclerView.Adapter<TokenTrackerAdapter.ViewHolder> implements Filterable {
    public interface OnItemClickListener {
      void onItemClick(int position, String info);
    }

    private List<TokenTrackerObj> originalData;
    private List<TokenTrackerObj> filteredData;

    private OnItemClickListener listener;

    // ViewHolder class (replace with your implementation)
    public static class ViewHolder extends RecyclerView.ViewHolder {
        TextView textView;

        public ViewHolder(View itemView) {
            super(itemView);

            textView = itemView.findViewById(android.R.id.text1);
        }
    }

    // Adapter constructor
    public TokenTrackerAdapter(List<TokenTrackerObj> data, OnItemClickListener listener) {
        this.originalData = data;
        this.filteredData = new ArrayList<>(data);

        this.listener = listener;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        // Inflate your item layout and return the ViewHolder
        View view = LayoutInflater.from(parent.getContext())
                .inflate(android.R.layout.simple_list_item_1, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        TokenTrackerObj item = filteredData.get(position);
        holder.textView.setText(item.symbol + " (" + item.name + ")");
        final String info = item.id + "," + (item.symbol.toUpperCase());
        holder.itemView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                listener.onItemClick(position, info);
            }
        });
    }

    @Override
    public int getItemCount() {
        return filteredData.size();
    }

    @Override
    public Filter getFilter() {
        return new Filter() {
            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                List<TokenTrackerObj> filteredList = new ArrayList<>();

                if (constraint == null || constraint.length() == 0) {
                    filteredList.addAll(originalData);
                } else {
                    String filterPattern = constraint.toString().toLowerCase().trim();

                    Pattern pattern = Pattern.compile(filterPattern);

                    for (TokenTrackerObj item : originalData) {
                        Matcher nameMatcher = pattern.matcher(item.name);
                        Matcher symbolMatcher = pattern.matcher(item.symbol);

                        // Check if either the name or symbol matches the pattern
                        if (nameMatcher.find() || symbolMatcher.find()) {
                            filteredList.add(item);
                        }
                    }
                }

                FilterResults results = new FilterResults();
                results.values = filteredList;
                return results;
            }

            @Override
            protected void publishResults(CharSequence constraint, FilterResults results) {
                filteredData.clear();
                filteredData.addAll((List) results.values);
                notifyDataSetChanged();
            }
        };
    }
}

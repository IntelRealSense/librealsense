package com.intel.realsense.camera;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

public class CheckableTwoLinesListCellAdapter extends ArrayAdapter<CheckableTwoLinesListCell> {
    private static final int mLayoutResourceId = R.layout.checkable_list_view;
    private final CheckableTwoLinesListCell mCheckableTwoLinesListCells[];
    private final LayoutInflater mLayoutInflater;
    private final Listener mListener;

    public CheckableTwoLinesListCellAdapter(Context context, CheckableTwoLinesListCell[] data, Listener listener){
        super(context, mLayoutResourceId, data);
        mLayoutInflater = ((Activity) context).getLayoutInflater();
        mCheckableTwoLinesListCells = data;
        mListener = listener;
    }

    public class Holder {
        TextView first;
        TextView second;
        CheckBox checkBox;
    }

    @Override
    public View getView(int position, View rawView, final ViewGroup parent){
        rawView = mLayoutInflater.inflate(mLayoutResourceId, parent, false);
        CheckableTwoLinesListCell settingsListViewLine = mCheckableTwoLinesListCells[position];

        final Holder holder;
        holder = new Holder();
        holder.first = rawView.findViewById(R.id.stream_device);
        holder.second = rawView.findViewById(R.id.stream_type);
        holder.checkBox = rawView.findViewById(R.id.stream_enable);
        holder.first.setText(settingsListViewLine.getMainString());
        holder.second.setText(settingsListViewLine.getSecondaryString());
        holder.checkBox.setTag(position);
        holder.checkBox.setChecked(settingsListViewLine.isEnabled());
        holder.checkBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                int position = (int) buttonView.getTag();
                CheckableTwoLinesListCell curr = mCheckableTwoLinesListCells[position];
                curr.setEnabled(isChecked);
                mListener.onCheckedChanged(curr);
            }
        });
        return rawView;
    }

    interface Listener{
        void onCheckedChanged(CheckableTwoLinesListCell holder);
    }
}

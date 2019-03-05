package com.intel.realsense.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.ListView;
import android.widget.TextView;

import com.intel.realsense.librealsense.StreamType;

import java.util.ArrayList;
import java.util.List;

public class SettingsActivity extends AppCompatActivity {
    private static final String TAG = "librs camera settings";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_view);
    }

    @Override
    protected void onResume() {
        super.onResume();

        TextView message = findViewById(R.id.list_view_title);
        message.setText("Streams:\n (disable all for default)");

        loadStreamList(createSettingList());
    }

    private void loadStreamList(CheckableTwoLinesListCell[] lines){
        if(lines == null)
            return;
        final CheckableTwoLinesListCellAdapter adapter = new CheckableTwoLinesListCellAdapter(this, lines, new CheckableTwoLinesListCellAdapter.Listener() {
            @Override
            public void onCheckedChanged(CheckableTwoLinesListCell holder) {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean(holder.getMainString(), holder.isEnabled());
                editor.commit();
            }
        });

        ListView streamListView = findViewById(R.id.list_view);
        streamListView.setAdapter(adapter);
        adapter.notifyDataSetChanged();
    }

    private CheckableTwoLinesListCell[] createSettingList(){

        List<CheckableTwoLinesListCell> settingsLines = new ArrayList<>();
        settingsLines.add(createLine(StreamType.COLOR));
        settingsLines.add(createLine(StreamType.DEPTH));
        settingsLines.add(createLine(StreamType.INFRARED));

        return settingsLines.toArray(new CheckableTwoLinesListCell[settingsLines.size()]);
    }

    private CheckableTwoLinesListCell createLine(StreamType streamType){
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);

        return new CheckableTwoLinesListCell(streamType.name(), "", sharedPref.getBoolean(streamType.name(), false));
    }
}

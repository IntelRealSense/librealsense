package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;

public class FileBrowserActivity extends AppCompatActivity {
    private static final String TAG = "librs camera detached";

    private File mFolder = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_view);
        String folderName = getIntent().getStringExtra(getString(R.string.browse_folder));
        mFolder = new File(folderName);
    }

    @Override
    protected void onResume() {
        super.onResume();

        TextView message = findViewById(R.id.list_view_title);
        if(!mFolder.exists()) {
            message.setText("No RealSense files found");
            return;
        }
        File[] files = mFolder.listFiles();
        if(files == null) {
            message.setText("Error reading RealSense files");
            Log.e(TAG, "Error reading files from " + mFolder.getAbsolutePath());
            return;
        }

        if(files.length == 0) {
            message.setText("No RealSense files found");
            return;
        }

        message.setText("Select a file to play from:");

        String[] filesNames = new String[files.length];
        final ListView listview = findViewById(R.id.list_view);

        for (int i = 0; i < files.length; ++i) {
            filesNames[i] = files[i].getName(); // Use getName() to extract the file name
        }

        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, filesNames);
        listview.setAdapter(adapter);

        listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
                final String item = files[position].getAbsolutePath();
                Intent intent = new Intent();
                intent.putExtra(getString(R.string.intent_extra_file_path), item);
                setResult(RESULT_OK, intent);
                finish();
            }
        });
    }
}
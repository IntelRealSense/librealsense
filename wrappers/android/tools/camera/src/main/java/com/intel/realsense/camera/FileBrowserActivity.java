package com.intel.realsense.camera;

import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;

public class FileBrowserActivity extends AppCompatActivity {

    private String mFolder = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_view);
        mFolder = getIntent().getStringExtra(getString(R.string.browse_folder));
    }

    @Override
    protected void onResume() {
        super.onResume();

        TextView message = findViewById(R.id.list_view_title);

        File folder = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + mFolder);
        if(!folder.exists()) {
            message.setText("No RealSense files found");
            return;
        }
        final File[] files = folder.listFiles();

        if(files.length == 0) {
            message.setText("No RealSense files found");
            return;
        }

        message.setText("Select a file to play from:");

        String[] filesNames = new String[files.length];
        final ListView listview = findViewById(R.id.list_view);

        for (int i = 0; i < files.length; ++i) {
            String path = files[i].getAbsolutePath();
            String[] split = path.split("/");
            filesNames[i] = split[split.length - 1];
        }

        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, filesNames);
        listview.setAdapter(adapter);

        listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, final View view,
                                    int position, long id) {
                final String item = files[position].getAbsolutePath();
                Intent intent = new Intent();
                intent.putExtra(getString(R.string.intent_extra_file_path), item);
                setResult(RESULT_OK, intent);
                finish();
            }

        });
    }
}

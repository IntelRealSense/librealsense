package com.intel.realsense.camera;

import android.content.res.Resources;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class PresetsActivity extends AppCompatActivity {
    private static final String TAG = "librs camera presets";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_view);
    }

    @Override
    protected void onResume() {
        super.onResume();

        TextView message = findViewById(R.id.list_view_title);
        final Resources resources = getResources();
        String[] presets;
        try {
            presets = resources.getAssets().list("presets");
        } catch (IOException e) {
            message.setText("No presets found");
            return;
        }

        if(presets.length == 0) {
            message.setText("No presets found");
            return;
        }
        message.setText("Select a preset:");

        final ListView listview = findViewById(R.id.list_view);

        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, presets);
        listview.setAdapter(adapter);

        final String[] finalPresets = presets;
        listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, final View view,
                                    int position, long id) {
                RsContext ctx = new RsContext();
                try(DeviceList devices = ctx.queryDevices()) {
                    if(devices.getDeviceCount() == 0){
                        Log.e(TAG, "failed to set preset, no device found");
                        finish();
                    }
                    try(Device device = devices.createDevice(0)){
                        if(device == null || !device.isInAdvancedMode()){
                            Log.e(TAG, "failed to set preset, device not in advanced mode");
                            finish();
                        }
                        final String item = finalPresets[position];
                        try {
                            InputStream is = resources.getAssets().open("presets/" + item);
                            byte[] buffer = new byte[is.available()];
                            is.read(buffer);
                            ByteArrayOutputStream baos = new ByteArrayOutputStream();
                            baos.write(buffer);
                            baos.close();
                            is.close();
                            device.loadPresetFromJson(buffer);
                        } catch (IOException e) {
                            Log.e(TAG, "failed to set preset, failed to open preset file, error: " + e.getMessage());
                        }
                        catch (Exception e) {
                            Log.e(TAG, "failed to set preset, error: " + e.getMessage());
                        }finally {
                            finish();
                        }
                    }
                }
            }
        });
    }
}

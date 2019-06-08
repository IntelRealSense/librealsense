package com.intel.realsense.camera;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;

import java.util.Map;
import java.util.TreeMap;

public class InfoActivity extends AppCompatActivity {
    private static final String TAG = "librs camera info";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list_view);
    }

    @Override
    protected void onResume() {
        super.onResume();

        TextView message = findViewById(R.id.list_view_title);

        RsContext ctx = new RsContext();
        DeviceList devices = ctx.queryDevices();
        if(devices.getDeviceCount() == 0){
            finish();
        }

        message.setText("Device info:");

        Map<CameraInfo,String> infoMap = new TreeMap<>();

        final Device device = ctx.queryDevices().createDevice(0);
        for(CameraInfo ci : CameraInfo.values()){
            if(device.supportsInfo(ci))
                infoMap.put(ci, device.getInfo(ci));
        }

        final String[] info = new String[infoMap.size()];
        int i = 0;
        for(Map.Entry e : infoMap.entrySet()){
            info[i++] = e.getKey().toString() + ": " + e.getValue();
        }


        final ListView listview = findViewById(R.id.list_view);

        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, info);
        listview.setAdapter(adapter);;
    }
}

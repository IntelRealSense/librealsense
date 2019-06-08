package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class SettingsActivity extends AppCompatActivity {
    private static final String TAG = "librs camera settings";
    private static final int INDEX_DEVICE_INFO = 0;
    private static final int INDEX_ADVANCE_MODE = 1;
    private static final int INDEX_PRESETS = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
    }

    @Override
    protected void onResume() {
        super.onResume();

        RsContext ctx = new RsContext();
        try(DeviceList devices = ctx.queryDevices()) {
            if (devices.getDeviceCount() == 0) {
                return;
            }
            Device device = ctx.queryDevices().createDevice(0);
            loadSettingsList(device);
            StreamProfileSelector[] profilesList = createSettingList(device);
            loadStreamList(device, profilesList);
        } catch(Exception e){
            Log.e(TAG, "failed to load settings, error: " + e.getMessage());
            Toast.makeText(this, "Failed to load settings", Toast.LENGTH_LONG).show();
            finish();
        }
    }

    private void loadSettingsList(final Device device){
        final ListView listview = findViewById(R.id.settings_list_view);

        Map<Integer,String> settingsMap = new TreeMap<>();
        settingsMap.put(INDEX_DEVICE_INFO,"Device info");
        settingsMap.put(INDEX_ADVANCE_MODE,"Enable advanced mode");

        if(device.supportsInfo(CameraInfo.ADVANCED_MODE) && device.isInAdvancedMode()){
            settingsMap.put(INDEX_ADVANCE_MODE,"Disable advanced mode");
            settingsMap.put(INDEX_PRESETS,"Presets");
        }
        final String[] settings = new String[settingsMap.values().size()];
        settingsMap.values().toArray(settings);
        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, settings);
        listview.setAdapter(adapter);

        listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, final View view,
                                    int position, long id) {
                switch (position){
                    case INDEX_DEVICE_INFO: {
                        Intent intent = new Intent(SettingsActivity.this, InfoActivity.class);
                        startActivity(intent);
                        break;
                    }
                    case INDEX_ADVANCE_MODE: device.toggleAdvancedMode(!device.isInAdvancedMode());
                        break;
                    case INDEX_PRESETS: {
                        Intent intent = new Intent(SettingsActivity.this, PresetsActivity.class);
                        startActivity(intent);
                        break;
                    }
                        default:
                            break;
                }
            }
        });
    }

    private static String getDeviceConfig(String pid, StreamType streamType, int streamIndex){
        return pid + "_" + streamType.name() + "_" + streamIndex;
    }

    public static String getEnabledDeviceConfigString(String pid, StreamType streamType, int streamIndex){
        return getDeviceConfig(pid, streamType, streamIndex) + "_enabled";
    }

    public static String getIndexdDeviceConfigString(String pid, StreamType streamType, int streamIndex){
        return getDeviceConfig(pid, streamType, streamIndex) + "_index";
    }

    public static Map<Integer, List<VideoStreamProfile>> createProfilesMap(Device device){
        Map<Integer, List<VideoStreamProfile>> rv = new HashMap<>();
        List<Sensor> sensors = device.querySensors();
        for (Sensor s : sensors){
            List<StreamProfile> profiles = s.getStreamProfiles();
            for (StreamProfile p : profiles){
                Pair<StreamType, Integer> pair = new Pair<>(p.getType(), p.getIndex());
                if(!rv.containsKey(pair.hashCode()))
                    rv.put(pair.hashCode(), new ArrayList<VideoStreamProfile>());
                VideoStreamProfile vsp = p.as(VideoStreamProfile.class);
                rv.get(pair.hashCode()).add(vsp);
            }
        }
        return rv;
    }

    private void loadStreamList(Device device, StreamProfileSelector[] lines){
        if(lines == null)
            return;
        final String pid = device.getInfo(CameraInfo.PRODUCT_ID);
        final StreamProfileAdapter adapter = new StreamProfileAdapter(this, lines, new StreamProfileAdapter.Listener() {
            @Override
            public void onCheckedChanged(StreamProfileSelector holder) {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                VideoStreamProfile p = holder.getProfile();
                editor.putBoolean(getEnabledDeviceConfigString(pid, p.getType(), p.getIndex()), holder.isEnabled());
                editor.putInt(getIndexdDeviceConfigString(pid, p.getType(), p.getIndex()), holder.getIndex());
                editor.commit();
            }
        });

        ListView streamListView = findViewById(R.id.configuration_list_view);
        streamListView.setAdapter(adapter);
        adapter.notifyDataSetChanged();
    }

    private StreamProfileSelector[] createSettingList(final Device device){
        Map<Integer, List<VideoStreamProfile>> profilesMap = createProfilesMap(device);

        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        String pid = device.getInfo(CameraInfo.PRODUCT_ID);

        List<StreamProfileSelector> lines = new ArrayList<>();
        for(Map.Entry e : profilesMap.entrySet()){
            List<VideoStreamProfile> list = (List<VideoStreamProfile>) e.getValue();
            VideoStreamProfile p = list.get(0);
            boolean enabled = sharedPref.getBoolean(getEnabledDeviceConfigString(pid, p.getType(), p.getIndex()), false);
            int index = sharedPref.getInt(getIndexdDeviceConfigString(pid, p.getType(), p.getIndex()), 0);
            lines.add(new StreamProfileSelector(enabled, index, list));
        }

        Collections.sort(lines);

        return lines.toArray(new StreamProfileSelector[lines.size()]);
    }
}

package com.intel.realsense.camera;

import android.Manifest;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
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
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.FwLogger;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Updatable;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class SettingsActivity extends AppCompatActivity {
    private static final String TAG = "librs camera settings";

    AppCompatActivity mContext;

    private static final int OPEN_FILE_REQUEST_CODE = 0;
    private static final int OPEN_FW_FILE_REQUEST_CODE = 1;

    private static final int INDEX_DEVICE_INFO = 0;
    private static final int INDEX_ADVANCE_MODE = 1;
    private static final int INDEX_PRESETS = 2;
    private static final int INDEX_UPDATE = 3;
    private static final int INDEX_UPDATE_UNSIGNED = 4;
    private static final int INDEX_TERMINAL = 5;
    private static final int INDEX_FW_LOG = 6;
    private static final int INDEX_CREATE_FLASH_BACKUP = 7;

    private Device _device;

    private boolean areAdvancedFeaturesEnabled = false; // advanced features (fw logs, terminal etc.)

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        mContext = this;

        // Advanced features are enabled if xml files exists in the device.
        String advancedFeaturesPath = FileUtilities.getExternalStorageDir() +
                File.separator +
                getString(R.string.realsense_folder) +
                File.separator +
                "hw";
        areAdvancedFeaturesEnabled = !FileUtilities.isPathEmpty(advancedFeaturesPath);
    }

    @Override
    protected void onResume() {
        super.onResume();

        int tries = 3;
        for(int i = 0; i < tries; i++){
            RsContext ctx = new RsContext();
            try(DeviceList devices = ctx.queryDevices()) {
                if (devices.getDeviceCount() == 0) {
                    Thread.sleep(500);
                    continue;
                }
                _device = devices.createDevice(0);
                loadInfosList();
                loadSettingsList(_device);
                StreamProfileSelector[] profilesList = createSettingList(_device);
                loadStreamList(_device, profilesList);
                return;
            } catch(Exception e){
                Log.e(TAG, "failed to load settings, error: " + e.getMessage());
            }
        }
        Log.e(TAG, "failed to load settings");
        Toast.makeText(this, "Failed to load settings", Toast.LENGTH_LONG).show();
        Intent intent = new Intent(this, DetachedActivity.class);
        startActivity(intent);
        finish();
    }
    @Override
    protected void onPause() {
        super.onPause();
        if (_device != null)
            _device.close();
    }

    private void loadInfosList() {
        final ListView listview = findViewById(R.id.info_list_view);
        String appVersion = "Camera App Version: " + BuildConfig.VERSION_NAME;
        String lrsVersion = "LibRealSense Version: " + RsContext.getVersion();

        final String[] info = { lrsVersion, appVersion};
        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, info);
        listview.setAdapter(adapter);
        adapter.notifyDataSetChanged();
    }

    private void loadSettingsList(final Device device){
        final ListView listview = findViewById(R.id.settings_list_view);

        final Map<Integer,String> settingsMap = new TreeMap<>();
        settingsMap.put(INDEX_DEVICE_INFO,"Device info");
        settingsMap.put(INDEX_ADVANCE_MODE,"Enable advanced mode");
        if(device.is(Extension.UPDATABLE)){
            settingsMap.put(INDEX_UPDATE,"Firmware update");
            try(Updatable fwud = device.as(Extension.UPDATABLE)){
                if(fwud != null && fwud.supportsInfo(CameraInfo.CAMERA_LOCKED) && fwud.getInfo(CameraInfo.CAMERA_LOCKED).equals("NO"))
                    settingsMap.put(INDEX_UPDATE_UNSIGNED,"Firmware update (unsigned)");
            }
        }

        if(device.supportsInfo(CameraInfo.ADVANCED_MODE) && device.isInAdvancedMode()){
            settingsMap.put(INDEX_ADVANCE_MODE,"Disable advanced mode");
            settingsMap.put(INDEX_PRESETS,"Presets");
        }

        if (areAdvancedFeaturesEnabled) {
            SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
            boolean fw_logging_enabled = sharedPref.getBoolean(getString(R.string.fw_logging), false);
            settingsMap.put(INDEX_FW_LOG, fw_logging_enabled ? "Stop FW logging" : "Start FW logging");

            settingsMap.put(INDEX_TERMINAL,"Terminal");
        }

        settingsMap.put(INDEX_CREATE_FLASH_BACKUP, "Create FW backup");

        final String[] settings = new String[settingsMap.values().size()];
        settingsMap.values().toArray(settings);
        final ArrayAdapter adapter = new ArrayAdapter<>(this, R.layout.files_list_view, settings);
        listview.setAdapter(adapter);

        listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, final View view,
                                    int position, long id) {
                Object[] keys = settingsMap.keySet().toArray();

                switch ((int)keys[position]){
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
                    case INDEX_UPDATE: {
                        FirmwareUpdateDialog fud = new FirmwareUpdateDialog();
                        Bundle bundle = new Bundle();
                        bundle.putBoolean(getString(R.string.firmware_update_request), true);
                        fud.setArguments(bundle);
                        fud.show(getFragmentManager(), "fw_update_dialog");
                        break;
                    }
                    case INDEX_UPDATE_UNSIGNED: {
                        Intent intent = new Intent(SettingsActivity.this, FileBrowserActivity.class);
                        intent.putExtra(getString(R.string.browse_folder), getString(R.string.realsense_folder) + File.separator +  "firmware");
                        startActivityForResult(intent, OPEN_FILE_REQUEST_CODE);
                        break;
                    }
                    case INDEX_TERMINAL: {
                        Intent intent = new Intent(SettingsActivity.this, TerminalActivity.class);
                        startActivity(intent);
                        break;
                    }
                    case INDEX_FW_LOG: {
                        toggleFwLogging();
                        recreate();
                        break;
                    }
                    case INDEX_CREATE_FLASH_BACKUP: {
                        new FlashBackupTask(device).execute();
                        break;
                    }
                    default:
                        break;
                }
            }
        });
    }

    private class FlashBackupTask extends AsyncTask<Void, Void, Void> {

        private ProgressDialog mProgressDialog;
        private Device mDevice;
        String mBackupFileName = "fwdump.bin";

        public FlashBackupTask(Device mDevice) {
            this.mDevice = mDevice;
        }

        @Override
        protected Void doInBackground(Void... voids) {
            try(final Updatable upd = mDevice.as(Extension.UPDATABLE)){
                FileUtilities.saveFileToExternalDir(mBackupFileName, upd.createFlashBackup());
                return null;
            }
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();

            mProgressDialog = ProgressDialog.show(mContext, "Saving Firmware Backup", "Please wait, this can take a few minutes");
            mProgressDialog.setCanceledOnTouchOutside(false);
            mProgressDialog.setCancelable(false);
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);

            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
            }

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    new AlertDialog.Builder(mContext)
                            .setTitle("Firmware Backup Success")
                            .setMessage("Saved into: " + FileUtilities.getExternalStorageDir() + File.separator + mBackupFileName)
                            .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            })
                            .show();

                }
            });
        }
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

    public static Map<Integer, List<StreamProfile>> createProfilesMap(Device device){
        Map<Integer, List<StreamProfile>> rv = new HashMap<>();
        List<Sensor> sensors = device.querySensors();
        for (Sensor s : sensors){
            List<StreamProfile> profiles = s.getStreamProfiles();
            for (StreamProfile p : profiles){
                Pair<StreamType, Integer> pair = new Pair<>(p.getType(), p.getIndex());
                if(!rv.containsKey(pair.hashCode()))
                    rv.put(pair.hashCode(), new ArrayList<StreamProfile>());
                rv.get(pair.hashCode()).add(p);
                p.close();
            }
        }
        return rv;
    }

    private void loadStreamList(Device device, StreamProfileSelector[] lines){
        if(device == null || lines == null)
            return;
        if(!device.supportsInfo(CameraInfo.PRODUCT_ID))
            throw new RuntimeException("try to config unknown device");
        final String pid = device.getInfo(CameraInfo.PRODUCT_ID);
        final StreamProfileAdapter adapter = new StreamProfileAdapter(this, lines, new StreamProfileAdapter.Listener() {
            @Override
            public void onCheckedChanged(StreamProfileSelector holder) {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                StreamProfile p = holder.getProfile();
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
        Map<Integer, List<StreamProfile>> profilesMap = createProfilesMap(device);

        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        if(!device.supportsInfo(CameraInfo.PRODUCT_ID))
            throw new RuntimeException("try to config unknown device");
        String pid = device.getInfo(CameraInfo.PRODUCT_ID);
        List<StreamProfileSelector> lines = new ArrayList<>();
        for(Map.Entry e : profilesMap.entrySet()){
            List<StreamProfile> list = (List<StreamProfile>) e.getValue();
            StreamProfile p = list.get(0);
            boolean enabled = sharedPref.getBoolean(getEnabledDeviceConfigString(pid, p.getType(), p.getIndex()), false);
            int index = sharedPref.getInt(getIndexdDeviceConfigString(pid, p.getType(), p.getIndex()), 0);
            lines.add(new StreamProfileSelector(enabled, index, list));
        }

        Collections.sort(lines);

        return lines.toArray(new StreamProfileSelector[lines.size()]);
    }

    void toggleFwLogging(){
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        boolean fw_logging_enabled = sharedPref.getBoolean(getString(R.string.fw_logging), false);
        String fw_logging_file_path = sharedPref.getString(getString(R.string.fw_logging_file_path), "");
        if(fw_logging_file_path.equals("")){
            Intent intent = new Intent(SettingsActivity.this, FileBrowserActivity.class);
            intent.putExtra(getString(R.string.browse_folder), getString(R.string.realsense_folder) + File.separator +  "hw");
            startActivityForResult(intent, OPEN_FW_FILE_REQUEST_CODE);
            return;
        }
        if(fw_logging_enabled)
            FwLogger.stopFwLogging();
        else
            FwLogger.startFwLogging(fw_logging_file_path);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean(getString(R.string.fw_logging), !fw_logging_enabled);
        editor.commit();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null)
            return;
        String filePath = data.getStringExtra(getString(R.string.intent_extra_file_path));
        switch (requestCode){
            case OPEN_FILE_REQUEST_CODE:{
                FirmwareUpdateProgressDialog fud = new FirmwareUpdateProgressDialog();
                Bundle bundle = new Bundle();
                bundle.putString(getString(R.string.firmware_update_file_path), filePath);
                fud.setArguments(bundle);
                fud.setCancelable(false);
                fud.show(getFragmentManager(), null);
                break;
            }
            case OPEN_FW_FILE_REQUEST_CODE: {
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putString(getString(R.string.fw_logging_file_path), filePath);
                editor.commit();
                toggleFwLogging();
                break;
            }
        }
    }
}

package com.intel.realsense.camera;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Environment;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ExpandableListView;
import android.widget.ListView;
import android.widget.Toast;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.FwLogger;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Updatable;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
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
        String advancedFeaturesPath = FileUtilities.getExternalStorageDir(mContext) +
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
                List<StreamProfileSelector> profilesList = createSettingList(_device);
                RemoveUnsupportedProfiles(profilesList);
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
        String appVersion = "Camera App Version: " + BuildConfig.VERSION_NAME;
        String lrsVersion = "LibRealSense Version: " + RsContext.getVersion();

        HashMap<String, List<String>> expandableListDetail = new HashMap<String, List<String>>();

        List<String> info = new ArrayList<String>();
        info.add(appVersion);
        info.add(lrsVersion);

        expandableListDetail.put("Software information", info);

        ExpandableListView expandableListView = findViewById(R.id.info_ex_list_view);
        List<String> expandableListTitle = new ArrayList<String>(expandableListDetail.keySet());
        SettingsViewAdapter infoViewAdapter = new SettingsViewAdapter(this, expandableListTitle, expandableListDetail);
        expandableListView.setAdapter(infoViewAdapter);
        // Expand Software information by default
        expandableListView.expandGroup(0);
    }

    private void loadSettingsList(final Device device){

        final Map<Integer,String> settingsMap = new TreeMap<>();

        settingsMap.put(INDEX_DEVICE_INFO,"Device info");

        if(device.supportsInfo(CameraInfo.ADVANCED_MODE)) {
            if (device.isInAdvancedMode()) {
                settingsMap.put(INDEX_ADVANCE_MODE, "Disable advanced mode");
                settingsMap.put(INDEX_PRESETS, "Presets");
            }
            else {
                settingsMap.put(INDEX_ADVANCE_MODE, "Enable advanced mode");
            }
        }

        if(device.is(Extension.UPDATABLE)){
            settingsMap.put(INDEX_UPDATE,"Firmware update");
            try(Updatable fwud = device.as(Extension.UPDATABLE)){
                if(fwud != null && fwud.supportsInfo(CameraInfo.CAMERA_LOCKED) && fwud.getInfo(CameraInfo.CAMERA_LOCKED).equals("NO"))
                    settingsMap.put(INDEX_UPDATE_UNSIGNED,"Firmware update (unsigned)");
            }
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

        // Create expandable list view
        ExpandableListView expandableListView = findViewById(R.id.settings_ex_list_view);
        HashMap<String, List<String>> expandableListDetail = new HashMap<String, List<String>>();
        List<String> settings_group = Arrays.asList(settings);
        expandableListDetail.put("Device Settings",settings_group);
        List<String> expandableListTitle = new ArrayList<String>(expandableListDetail.keySet());
        SettingsViewAdapter deviceSettingsViewAdapter = new SettingsViewAdapter(this, expandableListTitle, expandableListDetail);
        expandableListView.setAdapter(deviceSettingsViewAdapter);
        expandableListView.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {

            @Override
            public boolean  onChildClick(ExpandableListView parent, View v,
                                     int groupPosition, int childPosition, long id) {
                Object[] keys = settingsMap.keySet().toArray();

                switch ((int)keys[childPosition]){
                    case INDEX_DEVICE_INFO: {
                        Intent intent = new Intent(SettingsActivity.this, InfoActivity.class);
                        startActivity(intent);
                        break;
                    }
                    case INDEX_ADVANCE_MODE: device.toggleAdvancedMode(!device.isInAdvancedMode());
                        break;
                    case INDEX_PRESETS: {
                        PresetsDialog cd = new PresetsDialog();
                        cd.setCancelable(true);
                        cd.show(getFragmentManager(), null);
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
                        Intent intent = new Intent();
                        intent.setType("*/*");
                        intent.setAction(Intent.ACTION_OPEN_DOCUMENT);

                        if (intent.resolveActivity(getPackageManager()) != null) {
                            startActivityForResult(Intent.createChooser(intent, "Choose unsigned firmware file"), OPEN_FILE_REQUEST_CODE);
                        } else {
                            Log.d(TAG, "Unable to resolve Intent.ACTION_OPEN_DOCUMENT");
                        }
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
                        new FlashBackupTask(device, mContext).execute();
                        break;
                    }
                    default:
                        break;
                }
                return true;
            }
        });
    }

    private class FlashBackupTask extends AsyncTask<Void, Void, Void> {

        private ProgressDialog mProgressDialog;
        private Device mDevice;
        String mBackupFileName = "fwdump.bin";
        private Context mContext;

        public FlashBackupTask(Device mDevice, Context context) {
            this.mDevice = mDevice;
            this.mContext = context;
        }

        @Override
        protected Void doInBackground(Void... voids) {
            try(final Updatable upd = mDevice.as(Extension.UPDATABLE)){
                FileUtilities.saveFileToExternalDir(mContext, mBackupFileName, upd.createFlashBackup());
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
                            .setMessage("Saved into: " + FileUtilities.getExternalStorageDir(mContext) + File.separator + mBackupFileName)
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

    // set the stream profile from the device on/off in settings
    public static void setProfileSetting(SharedPreferences sharedPref, Device device, StreamProfile p, boolean enabled)
    {
        Map<Integer, List<StreamProfile>> profilesMap = SettingsActivity.createProfilesMap(device);
        SharedPreferences.Editor editor = sharedPref.edit();

        String pid = device.getInfo(CameraInfo.PRODUCT_ID);

        StreamType pType = p.getType();
        int pIndex = p.getIndex();
        StreamFormat pFormat = p.getFormat();
        int pFps = p.getFrameRate();

        int index = -1;

        // find the profile settings index
        for(Map.Entry e : profilesMap.entrySet()){

            // found the profile index
            if (index != -1) break;

            List<StreamProfile> profiles = (List<StreamProfile>) e.getValue();
            int i = 0;

            for (StreamProfile sp : profiles) {
                StreamType spType = sp.getType();
                int spIndex = sp.getIndex();
                StreamFormat spFormat = sp.getFormat();
                int spFps = sp.getFrameRate();

                if (p.is(Extension.VIDEO_PROFILE) && sp.is(Extension.VIDEO_PROFILE)) {
                    VideoStreamProfile vp = p.as(Extension.VIDEO_PROFILE);
                    VideoStreamProfile vsp = sp.as(Extension.VIDEO_PROFILE);

                    // found the profile
                    if (pType == spType && pIndex == spIndex && pFormat == spFormat && pFps == spFps &&
                            vp.getWidth() == vsp.getWidth() && vp.getHeight() == vsp.getHeight()) {
                        index = i;
                        break;
                    }
                }
                else if (p.is(Extension.MOTION_PROFILE) && sp.is(Extension.MOTION_PROFILE))
                {
                    // found the profile
                    if (pType == spType && pIndex == spIndex && pFormat == spFormat && pFps == spFps) {
                        index = i;
                        break;
                    }
                }

                i++;
            }
        }

        // turn the profile on/off in settings
        if (index != -1) {
            editor.putBoolean(SettingsActivity.getEnabledDeviceConfigString(pid, p.getType(), p.getIndex()), enabled);
            editor.putInt(SettingsActivity.getIndexdDeviceConfigString(pid, p.getType(), p.getIndex()), index);
            editor.commit();
        }
    }

    private void loadStreamList(Device device, List<StreamProfileSelector> streamProfiles){
        if(device == null || streamProfiles.size() == 0)
            return;
        if(!device.supportsInfo(CameraInfo.PRODUCT_ID))
            throw new RuntimeException("try to config unknown device");

        StreamProfileSelector[] streamProfilesArray = streamProfiles.toArray(new StreamProfileSelector[streamProfiles.size()]);
        List<String> settings_group = new ArrayList<String>();
        for (int i = 0; i < streamProfilesArray.length ; i++) {
            settings_group.add(streamProfilesArray[i].getName());
        }
        // Create expandable list view
        ExpandableListView streamListView = findViewById(R.id.configuration_ex_list_view);
        HashMap<String, List<String>> expandableListDetail = new HashMap<String, List<String>>();

        expandableListDetail.put("Configuration:(turn off all to reset to default streams)",settings_group);
        List<String> expandableListTitle = new ArrayList<String>(expandableListDetail.keySet());

        final String pid = device.getInfo(CameraInfo.PRODUCT_ID);
        final StreamProfileAdapter adapter = new StreamProfileAdapter(this, expandableListTitle, expandableListDetail, streamProfilesArray, new StreamProfileAdapter.Listener() {
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

        streamListView.setAdapter(adapter);
        adapter.notifyDataSetChanged();
    }

    private List<StreamProfileSelector> createSettingList(final Device device){
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

        return lines;
    }

    private void RemoveUnsupportedProfiles(List<StreamProfileSelector> streamProfiles){
        StreamProfileSelector confidenceProfile = null;
        for (StreamProfileSelector streamProfile : streamProfiles){
            if (streamProfile.getProfile().getType() == StreamType.CONFIDENCE){
                confidenceProfile = streamProfile;
                break;
            }
        }

    // Confidence stream format is RAW8, and it is not supported for display.
    // Its removal is necessary until format RAW8 display is enabled.
        if (confidenceProfile != null)
            streamProfiles.remove(confidenceProfile);
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

                // firmware file source location
                Uri uri = data.getData();
                Log.d(TAG, "uri: " + uri.getPath());

                try {
                    ContentResolver contentResolver = getContentResolver();

                    // firmware file in app local storage, for example,
                    // /storage/emulated/0/Android/data/com.intel.realsense.camera/files/Download
                    File downloadDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
                    filePath = downloadDir.getAbsolutePath() +  File.separator + "tempfw";

                    // copy firmware file from source to app local storage
                    InputStream input =  contentResolver.openInputStream(uri);
                    FileOutputStream output = new FileOutputStream(new File(filePath));

                    byte[] buffer = new byte[1024];
                    int len;
                    while ((len = input.read(buffer)) > 0) {
                        output.write(buffer, 0, len);
                    }
                    input.close();
                    output.close();
                }
                catch (Exception e){
                    final String msg = "Failed obtaining the firmware file: " + e.getMessage();
                    Log.d(TAG, msg);

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(SettingsActivity.this, msg, Toast.LENGTH_LONG).show();
                        }
                    });
                }

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

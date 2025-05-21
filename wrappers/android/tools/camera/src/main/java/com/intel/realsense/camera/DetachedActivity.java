package com.intel.realsense.camera;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.ProductLine;
import com.intel.realsense.librealsense.RsContext;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class DetachedActivity extends AppCompatActivity {
    private static final String TAG = "librs camera detached";
    private static final int PLAYBACK_REQUEST_CODE = 1;
    private static final String MINIMAL_D400_FW_VERSION = "5.10.0.0";

    private static final int REQUEST_PERMISSIONS = 1;
    private static final int REQUEST_STORAGE_PERMISSIONS = 2;
    private static final String[] PERMISSIONS = {
            Manifest.permission.CAMERA
    };
    private static final String[] STORAGE_PERMISSIONS = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE
    };

    private boolean hasStoragePermissions() {
        for (String permission : STORAGE_PERMISSIONS) {
            if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }
    private void requestStoragePermissions() {
        ActivityCompat.requestPermissions(this, STORAGE_PERMISSIONS, REQUEST_STORAGE_PERMISSIONS);
    }
    private Button mPlaybackButton;

    private Context mAppContext;
    private RsContext mRsContext = new RsContext();

    private Map<ProductLine,String> mMinimalFirmwares = new HashMap<>();
    private boolean mUpdating = false;
    private boolean mDetached = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "DetachedActivity onCreate called");

        setContentView(R.layout.activity_detached);

        mAppContext = getApplicationContext();
    }
    //check permissions request response
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult called");
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_PERMISSIONS) {
            if (grantResults.length > 0) {
                for (int result : grantResults) {
                    if (result != PackageManager.PERMISSION_GRANTED) {
                        Log.e(TAG, "Permission denied");
                        //display error to user
                        Toast.makeText(DetachedActivity.this, "Permission denied", Toast.LENGTH_LONG).show();
                        return;
                    }
                }
                Log.d(TAG, "Permissions granted");
                init(); //call init() here

            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume called");
        if (!hasPermissions()) {
            Log.d(TAG, "Requesting permissions");
            ActivityCompat.requestPermissions(this, PERMISSIONS, REQUEST_PERMISSIONS);

        }
        else {
            Log.d(TAG, "Permissions already granted");
            init();
            if (isCameraPermissionGranted()) {
                Log.d(TAG, "Camera permission granted");

                try{
                    // BREAKPOINT 1: Set a breakpoint here
                    RsContext.init(getApplicationContext());
                    // BREAKPOINT 2: Set a breakpoint here
                    mRsContext.setDevicesChangedCallback(mListener);
                    // BREAKPOINT 3: Set a breakpoint here
                    validatedDevice();
                }catch (Exception e) {
                    Log.e(TAG, "Error in the SDK init: " + e.getMessage());
                    Log.e(TAG, "Workaround: Set defaultTargetSdkVersion = 33  in build.gradle (Project:LRS)");
                }
            }
        }
    }

    private boolean isCameraPermissionGranted() {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED;
    }

    private boolean hasPermissions() {
        if (PERMISSIONS != null) {
            for (String permission : PERMISSIONS) {
                if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    return false;
                }
            }
        }
        return true;
    }

    private synchronized void init() {
        Log.d(TAG, "init: entry");
        try{
            mPlaybackButton = findViewById(R.id.playbackButton);
            Log.d(TAG, "init: playbackButton found");
            mPlaybackButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Log.d(TAG, "init: playbackButton onClick called");
                    mDetached = false;
                    finish();
                    Intent intent = new Intent(DetachedActivity.this, PlaybackActivity.class);
                    startActivityForResult(intent, PLAYBACK_REQUEST_CODE);
                    Log.d(TAG, "init: playbackButton onClick finished");
                }
            });

            Log.d(TAG, "init: runOnUiThread start");
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "init: runOnUiThread run");
                    String appVersion = "-";
                    try {
                        appVersion = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
                        Log.d(TAG, "init: runOnUiThread appVersion : " + appVersion);
                    } catch (PackageManager.NameNotFoundException e) {
                        Log.e(TAG, "init: runOnUiThread error getting app version");
                    }
                    String lrsVersion = RsContext.getVersion();
                    Log.d(TAG, "init: runOnUiThread lrsVersion: " + lrsVersion);
                    TextView versions = findViewById(R.id.versionsText);
                    Log.d(TAG, "init: runOnUiThread versions found");
                    versions.setText("librealsense version: " + lrsVersion + "\ncamera app version: " + appVersion);
                    Log.d(TAG, "init: runOnUiThread text set");
                }
            });
            Log.d(TAG, "init: runOnUiThread end");

            mMinimalFirmwares.put(ProductLine.D400, MINIMAL_D400_FW_VERSION);
            Log.d(TAG, "init: mMinimalFirmwares done");
        }catch(Exception e){
            Log.e(TAG, "init: error : " + e.getMessage());
        }
        Log.d(TAG, "init: exit");
    }

    private synchronized void validatedDevice(){
        Log.d(TAG, "validatedDevice: entry");
        if(mUpdating) {
            Log.d(TAG, "validatedDevice: updating");
            return;
        }
        Log.d(TAG, "validatedDevice: creating deviceList");
        try(DeviceList dl = mRsContext.queryDevices()){
            Log.d(TAG, "validatedDevice: deviceList created");
            if(dl.getDeviceCount() == 0) {
                Log.d(TAG, "validatedDevice: no device found");
                init();

                // kill preview activity if device disconnected
                if (mDetached) {
                    Log.d(TAG, "validatedDevice: device detached");
                    mDetached = false;

                    Intent intent = new Intent(this, PreviewActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
                    intent.putExtra("keepalive", false);
                    startActivity(intent);
                    Log.d(TAG, "validatedDevice: previewActivity killed");
                }
                Log.d(TAG, "validatedDevice: exit (no device)");
                return;
            }
            Log.d(TAG, "validatedDevice: creating device");
            try(Device d = dl.createDevice(0)){
                Log.d(TAG, "validatedDevice: device created");
                if(d == null) {
                    Log.e(TAG, "validatedDevice: error creating device");
                    return;
                }
                Log.d(TAG, "validatedDevice: checking device extension");
                if(d.is(Extension.UPDATE_DEVICE)){
                    Log.d(TAG, "validatedDevice: device is update device");
                    FirmwareUpdateProgressDialog fupd = new FirmwareUpdateProgressDialog();
                    fupd.setCancelable(false);
                    fupd.show(getFragmentManager(), null);
                    mUpdating = true;
                }
                else {
                    Log.d(TAG, "validatedDevice: device is not update device");
                    if (!validateFwVersion(d)){
                        Log.d(TAG, "validatedDevice: fw version not validated");
                        return;
                    }

                    mDetached = false;

                    // launch preview activity and keep it alive
                    // the activity is single top instance, can be killed later the same instance
                    // to prevent issues with multiple instances
                    Intent intent = new Intent(this, PreviewActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
                    intent.putExtra("keepalive", true);
                    startActivity(intent);
                }
            }
        } catch (Exception e){
            Log.e(TAG, "error while validating device, error: " + e.getMessage());
        }
    }


    private boolean validateFwVersion(Device device){
        final String currFw = device.getInfo(CameraInfo.FIRMWARE_VERSION).split("\n")[0];
        final ProductLine pl = ProductLine.valueOf(device.getInfo(CameraInfo.PRODUCT_LINE));
        if(mMinimalFirmwares.containsKey(pl)){
            final String minimalFw = mMinimalFirmwares.get(pl);
            if(!compareFwVersion(currFw, minimalFw)){
                FirmwareUpdateDialog fud = new FirmwareUpdateDialog();
                Bundle bundle = new Bundle();
                bundle.putBoolean(getString(R.string.firmware_update_required), true);
                fud.setArguments(bundle);
                fud.show(getFragmentManager(), null);
                return false;
            }
        }

        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        boolean showUpdateMessage = sharedPref.getBoolean(getString(R.string.show_update_firmware), true);
        if (!showUpdateMessage || !device.supportsInfo(CameraInfo.RECOMMENDED_FIRMWARE_VERSION))
            return true;

        final String recommendedFw = device.getInfo(CameraInfo.RECOMMENDED_FIRMWARE_VERSION);
        if(!compareFwVersion(currFw, recommendedFw)){
            FirmwareUpdateDialog fud = new FirmwareUpdateDialog();
            fud.show(getFragmentManager(), null);
            return false;
        }
        return true;
    }

    private boolean compareFwVersion(String currFw, String otherFw){
        String[] sFw = currFw.split("\\.");
        String[] sRecFw = otherFw.split("\\.");
        for (int i = 0; i < sRecFw.length; i++) {
            if (Integer.parseInt(sFw[i]) > Integer.parseInt(sRecFw[i]))
                break;
            if (Integer.parseInt(sFw[i]) < Integer.parseInt(sRecFw[i])) {
                return false;
            }
        }
        return true;
    }

    public void onFwUpdateStatus(final boolean status){
        mUpdating = false;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String msg = status ? "firmware update done" : "firmware update failed";
                Toast.makeText(DetachedActivity.this, msg, Toast.LENGTH_LONG).show();
            }
        });
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            validatedDevice();
        }

        @Override
        public void onDeviceDetach() {
            mDetached = true;
            validatedDevice();
        }
    };
}

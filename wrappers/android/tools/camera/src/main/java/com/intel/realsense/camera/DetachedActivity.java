package com.intel.realsense.camera;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
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
import com.intel.realsense.librealsense.FwLogger;
import com.intel.realsense.librealsense.ProductLine;
import com.intel.realsense.librealsense.RsContext;

import java.io.File;
import java.security.Permission;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class DetachedActivity extends AppCompatActivity {
    private static final String TAG = "librs camera detached";
    private static final int PLAYBACK_REQUEST_CODE = 1;
    private static final String MINIMAL_D400_FW_VERSION = "5.10.0.0";

    private Button mPlaybackButton;

    private Context mAppContext;
    private RsContext mRsContext = new RsContext();

    private Map<ProductLine,String> mMinimalFirmwares = new HashMap<>();
    private boolean mUpdating = false;
    private boolean mDetached = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_detached);

        mAppContext = getApplicationContext();

        requestPermissionsIfNeeded();

        mPlaybackButton = findViewById(R.id.playbackButton);
        mPlaybackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDetached = false;
                finish();
                Intent intent = new Intent(DetachedActivity.this, PlaybackActivity.class);
                startActivityForResult(intent, PLAYBACK_REQUEST_CODE);
            }
        });

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String appVersion = BuildConfig.VERSION_NAME;
                String lrsVersion = RsContext.getVersion();
                TextView versions = findViewById(R.id.versionsText);
                versions.setText("librealsense version: " + lrsVersion + "\ncamera app version: " + appVersion);
            }
        });

        mMinimalFirmwares.put(ProductLine.D400, MINIMAL_D400_FW_VERSION);
    }

    private void requestPermissionsIfNeeded() {
        ArrayList<String> permissions = new ArrayList<>();
        if (!isCameraPermissionGranted()) {
            permissions.add(Manifest.permission.CAMERA);
        }

        if (!isWritePermissionGranted()) {
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }
        if (!permissions.isEmpty())
            ActivityCompat.requestPermissions(this, permissions.toArray(new String[permissions.size()]), PermissionsUtils.PERMISSIONS_REQUEST_ALL);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mDetached = true;
        if (isCameraPermissionGranted()) {
            RsContext.init(getApplicationContext());
            mRsContext.setDevicesChangedCallback(mListener);
            validatedDevice();
        }
    }

    private boolean isCameraPermissionGranted() {
        return android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O && ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED;
    }

    private boolean isWritePermissionGranted() {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    private synchronized void validatedDevice(){
        if(mUpdating)
            return;
        try(DeviceList dl = mRsContext.queryDevices()){
            if(dl.getDeviceCount() == 0)
                return;
            try(Device d = dl.createDevice(0)){
                if(d == null)
                    return;
                if(d.is(Extension.UPDATE_DEVICE)){
                    FirmwareUpdateProgressDialog fupd = new FirmwareUpdateProgressDialog();
                    fupd.setCancelable(false);
                    fupd.show(getFragmentManager(), null);
                    mUpdating = true;
                }
                else {
                    if (!validateFwVersion(d))
                        return;
                    mDetached = false;
                    SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                    boolean fw_logging_enabled = sharedPref.getBoolean(getString(R.string.fw_logging), false);
                    String fw_logging_file_path = sharedPref.getString(getString(R.string.fw_logging_file_path), "");
                    if(fw_logging_enabled && !fw_logging_file_path.equals("")){
                        FwLogger.startFwLogging(fw_logging_file_path);
                    }
                    finish();
                    Intent intent = new Intent(this, PreviewActivity.class);
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
            if(mDetached)
                return;
            finish();
            Intent intent = new Intent(mAppContext, DetachedActivity.class);
            startActivity(intent);
        }
    };
}

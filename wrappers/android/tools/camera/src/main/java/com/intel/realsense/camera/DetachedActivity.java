package com.intel.realsense.camera;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.RsContext;

public class DetachedActivity extends AppCompatActivity {
    private static final String TAG = "librs camera detached";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;
    private static final int PLAYBACK_REQUEST_CODE = 1;

    private boolean mPermissionsGrunted = false;
    private Button mPlaybackButton;

    private Context mAppContext;
    private RsContext mRsContext = new RsContext();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_detached);

        mAppContext = getApplicationContext();

        mPlaybackButton = findViewById(R.id.playbackButton);
        mPlaybackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(mAppContext, PlaybackActivity.class);
                startActivityForResult(intent, PLAYBACK_REQUEST_CODE);
            }
        });

        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }

        mPermissionsGrunted = true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }

        mPermissionsGrunted = true;
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mPermissionsGrunted)
            init();
    }

    private void init() {
        RsContext.init(getApplicationContext());
        mRsContext.setDevicesChangedCallback(mListener);

        if(mRsContext.queryDevices().getDeviceCount() > 0){
            if(!validated_device())
                return;
            Intent intent = new Intent(mAppContext, PreviewActivity.class);
            startActivity(intent);
            finish();
        }
    }

    private boolean validated_device(){
         DeviceList devices = mRsContext.queryDevices();
        if(devices.getDeviceCount() == 0)
            return false;
        Device device = devices.createDevice(0);
        final String recFw = device.getInfo(CameraInfo.RECOMMENDED_FIRMWARE_VERSION);
        final String fw = device.getInfo(CameraInfo.FIRMWARE_VERSION);
        String[] sFw = fw.split("\\.");
        String[] sRecFw = recFw.split("\\.");
        for(int i = 0; i < sRecFw.length; i++){
            if(Integer.parseInt(sFw[i]) > Integer.parseInt(sRecFw[i]))
                break;
            if(Integer.parseInt(sFw[i]) < Integer.parseInt(sRecFw[i])){
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        TextView textView = findViewById(R.id.connectCameraText);
                        textView.setText("The FW of the connected device is:\n " + fw +
                                "\n\nThe recommended FW for this device is:\n " + recFw +
                                "\n\nPlease update your device to the recommended FW or higher");
                    }
                });
                return false;
            }
        }
        return true;
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            if(!validated_device())
                return;
            Intent intent = new Intent(mAppContext, PreviewActivity.class);
            startActivity(intent);
            finish();
        }

        @Override
        public void onDeviceDetach() {
            Intent intent = new Intent(mAppContext, DetachedActivity.class);
            startActivity(intent);
            finish();
        }
    };
}

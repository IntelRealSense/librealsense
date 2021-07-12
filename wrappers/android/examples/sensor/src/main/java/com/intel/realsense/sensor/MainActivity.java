package com.intel.realsense.sensor;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.DepthSensor;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.Sensor;

import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Extension;

import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.RsContext;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs sensor example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceView;
    private boolean mIsStreaming = false;

    private Colorizer mColorizer;
    private RsContext mRsContext;

    private Device mDevice;
    DepthSensor mDepthSensor = null;
    StreamProfile mDepthProfile = null;
    StreamProfile mIrProfile = null;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
        mBackGroundText = findViewById(R.id.connectCameraText);
        mGLSurfaceView = findViewById(R.id.glSurfaceView);
        mGLSurfaceView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }

        mPermissionsGranted = true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLSurfaceView.close();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        mPermissionsGranted = true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(mPermissionsGranted)
            init();
        else
            Log.e(TAG, "missing permissions");
    }

    @Override
    protected void onPause() {
        super.onPause();
        stop();
        releaseContext();
    }

    private FrameCallback mFrameHandler = new FrameCallback()
    {
        @Override
        public void onFrame(final Frame f) {
            try {
                if (f != null) {
                    if (f.is(Extension.DEPTH_FRAME))
                    {
                        try (Frame processed = f.applyFilter(mColorizer)) {
                            mGLSurfaceView.upload(processed);
                        }
                    }
                    else
                    {
                        mGLSurfaceView.upload(f);
                    }
                }
            } catch (Exception e) {}
        }
    };

    private void init(){
        //RsContext.init must be called once in the application lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(mAppContext);

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(mListener);

        mColorizer = new Colorizer();

        try(DeviceList dl = mRsContext.queryDevices()){
            int num_devices = dl.getDeviceCount();
            Log.d(TAG, "devices found: " + num_devices);

            if( num_devices> 0) {
                mDevice = dl.createDevice(0);
                showConnectLabel(false);
                assignDepthSensor();
                assignProfiles();
                start();
            }
        }
    }

    private void showConnectLabel(final boolean state){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBackGroundText.setVisibility(state ? View.VISIBLE : View.GONE);
            }
        });
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            showConnectLabel(false);
        }

        @Override
        public void onDeviceDetach() {
            showConnectLabel(true);
            stop();
        }
    };

    private void configAndStart() throws Exception {
        if (mDepthSensor != null) {
            if (mDepthProfile == null && mIrProfile == null) {
                Toast.makeText(this, "The requested profiles are not available in this device ", Toast.LENGTH_LONG).show();
            }
            else {
                List<StreamProfile> requested_profiles = new ArrayList<StreamProfile>();
                if (mDepthProfile != null)
                    requested_profiles.add(mDepthProfile);
                else
                    Toast.makeText(this, "The depth requested profile is not available in this device ", Toast.LENGTH_LONG).show();

                if (mIrProfile != null)
                    requested_profiles.add(mIrProfile);
                else
                    Toast.makeText(this, "The infrared requested profile is not available in this device ", Toast.LENGTH_LONG).show();

                mDepthSensor.openSensor(requested_profiles);
                mDepthSensor.start(mFrameHandler);
            }
        }
    }

    private synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            Log.d(TAG, "try start streaming");
            mGLSurfaceView.clear();
            mIsStreaming = true;

            configAndStart();

            Log.d(TAG, "streaming started successfully");
        } catch (Exception e) {
            Log.e(TAG, "failed to start streaming");
        }
    }

    private synchronized void stop() {
        if(!mIsStreaming)
            return;
        try {
            Log.d(TAG, "try stop streaming");
            mIsStreaming = false;

            if (mDepthSensor != null){
                mDepthSensor.stop();
                mDepthSensor.closeSensor();
            }

            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            Log.e(TAG, "failed to stop streaming");
        }
    }

    private void releaseContext() {
        if (mDevice != null) mDevice.close();
        if (mColorizer != null) mColorizer.close();

        if(mRsContext != null) {
            mRsContext.removeDevicesChangedCallback();
            mRsContext.close();
            mRsContext = null;
        }
    }

    private void assignDepthSensor() {
        List<Sensor> sensors = mDevice.querySensors();

        for(Sensor s : sensors)
        {
            if (s.is(Extension.DEPTH_SENSOR)) {
                mDepthSensor = s.as(Extension.DEPTH_SENSOR);
            }
        }
    }
    private void assignProfiles() {
        if (mDepthSensor != null) {
            mDepthProfile = mDepthSensor.findProfile(StreamType.DEPTH, -1, 640, 480, StreamFormat.Z16, 30);
            mIrProfile = mDepthSensor.findProfile(StreamType.INFRARED, -1, 640, 480, StreamFormat.Y8, 30);
        }
    }
}

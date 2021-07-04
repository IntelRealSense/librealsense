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

import com.intel.realsense.librealsense.VideoStreamProfile;
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
    DepthSensor depth_sensor = null;


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
    }

    private FrameCallback mDepthFrameHandler = new FrameCallback()
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
        List<Sensor> sensors = mDevice.querySensors();

        for(Sensor s : sensors)
        {
            if (s.is(Extension.DEPTH_SENSOR)) {
                depth_sensor = s.as(Extension.DEPTH_SENSOR);
            }
        }

        boolean depth_profile_found = false;
        boolean ir_profile_found = false;

        if (depth_sensor != null) {
            List<StreamProfile> stream_profiles = depth_sensor.getStreamProfiles();
            StreamProfile depth_profile = stream_profiles.get(0);
            StreamProfile ir_profile = stream_profiles.get(0);

            for (StreamProfile stream_profile : stream_profiles) {
                if (depth_profile_found && ir_profile_found)
                    break;
                if (!depth_profile_found && stream_profile.getType().compareTo(StreamType.DEPTH) == 0) {

                    if (stream_profile.is(Extension.VIDEO_PROFILE)) {
                        VideoStreamProfile video_stream_profile = stream_profile.as(Extension.VIDEO_PROFILE);

                        // After using the "as" method we can use the new data type
                        //  for additional operations:
                        StreamFormat sf = video_stream_profile.getFormat();
                        int index = stream_profile.getIndex();
                        StreamType st = stream_profile.getType();
                        int w = video_stream_profile.getWidth();
                        int h = video_stream_profile.getHeight();
                        int fps = video_stream_profile.getFrameRate();

                        if (w == 640 && h == 480 && fps == 30 && (sf.compareTo(StreamFormat.Z16) == 0)) {
                            Log.d(TAG, "depth stream: " + index + ":" + st.name() + ":" + sf.name() + ":" + w + "x" + h + "@" + fps + "HZ");

                            depth_profile = stream_profile;
                            depth_profile_found = true;
                        }
                    }
                }
                if (!ir_profile_found && stream_profile.getType().compareTo(StreamType.INFRARED) == 0 && stream_profile.getIndex() == 1 ) {

                    if (stream_profile.is(Extension.VIDEO_PROFILE)) {
                        VideoStreamProfile video_stream_profile = stream_profile.as(Extension.VIDEO_PROFILE);

                        // After using the "as" method we can use the new data type
                        //  for additional operations:
                        StreamFormat sf = video_stream_profile.getFormat();
                        int index = stream_profile.getIndex();
                        StreamType st = stream_profile.getType();
                        int w = video_stream_profile.getWidth();
                        int h = video_stream_profile.getHeight();
                        int fps = video_stream_profile.getFrameRate();

                        if (w == 640 && h == 480 && fps == 30 && (sf.compareTo(StreamFormat.Y8) == 0)) {
                            Log.d(TAG, "ir stream: " + index + ":" + st.name() + ":" + sf.name() + ":" + w + "x" + h + "@" + fps + "HZ");

                            ir_profile = stream_profile;
                            ir_profile_found = true;
                        }
                    }
                }
            }

            if (!depth_profile_found && !ir_profile_found) {
                Toast.makeText(this, "The requested profiles are not available in this device ", Toast.LENGTH_LONG).show();
            }
            else {
                List<StreamProfile> requested_profiles = new ArrayList<StreamProfile>();
                if (depth_profile_found)
                    requested_profiles.add(depth_profile);
                else
                    Toast.makeText(this, "The depth requested profile is not available in this device ", Toast.LENGTH_LONG).show();

                if (ir_profile_found)
                    requested_profiles.add(ir_profile);
                else
                    Toast.makeText(this, "The infrared requested profile is not available in this device ", Toast.LENGTH_LONG).show();

                depth_sensor.openSensor(requested_profiles);
                depth_sensor.start(mDepthFrameHandler);
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

            if (depth_sensor != null) depth_sensor.stop();

            if (mColorizer != null) mColorizer.close();
            if (depth_sensor != null) {depth_sensor.closeSensor();}

            if (mDevice != null) mDevice.close();

            if(mRsContext != null) {
                mRsContext.removeDevicesChangedCallback();
                mRsContext.close();
                mRsContext = null;
            }

            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            Log.e(TAG, "failed to stop streaming");
        }
    }
}

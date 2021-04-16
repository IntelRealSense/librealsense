package com.intel.realsense.sensor;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.DepthSensor;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.ColorSensor;

import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Extension;

import com.intel.realsense.librealsense.VideoStreamProfile;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;

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
    ColorSensor color_sensor = null;

    Thread streaming = null;

    BlockingQueue<Frame> frameQueue = new ArrayBlockingQueue<Frame>(4);
    int frames_received = 0;
    int frames_dropped = 0;
    int frames_displayed = 0;

    int color_frames_received = 0;
    int color_frames_dropped = 0;

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
        if(mRsContext != null)
            mRsContext.close();

        stop();
    }

    private FrameCallback mFrameHandler = new FrameCallback()
    {
        @Override
        public void onFrame(final Frame f) {
            frames_received++;

            Frame cf = f.clone();

            if (frameQueue.remainingCapacity() > 0) {
                frameQueue.add(cf);
            }
            else
            {
                frames_dropped++;
            }
        }
    };

    private FrameCallback mColorFrameHandler = new FrameCallback()
    {
        @Override
        public void onFrame(final Frame f) {
            color_frames_received++;

            Frame cf = f.clone();

            if (frameQueue.remainingCapacity() > 0) {
                frameQueue.add(cf);
            }
            else
            {
                color_frames_dropped++;
            }
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

    Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            synchronized(this) {
                frameQueue.clear();

                while(mIsStreaming) {
                    try {
                        Frame mFrame = frameQueue.poll(1000, TimeUnit.MILLISECONDS);
                        frames_displayed++;

                        if (mFrame != null) {
                            if (mFrame.is(Extension.DEPTH_FRAME)) {
                                Frame cf = mFrame.applyFilter(mColorizer);
                                mGLSurfaceView.upload(cf);
                                cf.close();
                            } else {
                                mGLSurfaceView.upload(mFrame);
                            }

                            mFrame.close();
                        }
                    } catch (Exception e) {
                        Log.e(TAG, "streaming, error: " + e.getMessage());
                    }
                }
            }
        }
    };

    private void configAndStart() throws Exception {
        List<Sensor> sensors = mDevice.querySensors();

        for(Sensor s : sensors)
        {
            if (s.is(Extension.DEPTH_SENSOR)) {
                depth_sensor = s.as(Extension.DEPTH_SENSOR);
            }
            else if (s.is(Extension.COLOR_SENSOR)) {
                color_sensor = s.as(Extension.COLOR_SENSOR);
            }

            List<StreamProfile> sps = s.getStreamProfiles();

            for (StreamProfile sp : sps)
            {
                int index = sp.getIndex();
                StreamType st = sp.getType();

                if (sp.is(Extension.VIDEO_PROFILE)) {
                    VideoStreamProfile video_stream_profile = sp.as(Extension.VIDEO_PROFILE);

                    // After using the "as" method we can use the new data type
                    //  for additinal operations:
                    StreamFormat sf = video_stream_profile.getFormat();
                    int w = video_stream_profile.getWidth();
                    int h = video_stream_profile.getHeight();
                    int fps = video_stream_profile.getFrameRate();

                    Log.d(TAG, "stream: " + index + ":" + st.name() + ":" + sf.name() + ":" + w + "x" + h + "@" + fps + "HZ");
                }
            }
        }

        if (depth_sensor != null) {
            List<StreamProfile> sps = depth_sensor.getStreamProfiles();
            StreamProfile sp = sps.get(0);

            for (StreamProfile sp2 : sps) {
                if (sp2.getType().compareTo(StreamType.DEPTH) == 0) {

                    if (sp2.is(Extension.VIDEO_PROFILE)) {
                        VideoStreamProfile video_stream_profile = sp2.as(Extension.VIDEO_PROFILE);

                        // After using the "as" method we can use the new data type
                        //  for additinal operations:
                        StreamFormat sf = video_stream_profile.getFormat();
                        int index = sp2.getIndex();
                        StreamType st = sp2.getType();
                        int w = video_stream_profile.getWidth();
                        int h = video_stream_profile.getHeight();
                        int fps = video_stream_profile.getFrameRate();

                        if (w == 640 && fps == 30 && (sf.compareTo(StreamFormat.Z16) == 0)) {
                            Log.d(TAG, "depth stream: " + index + ":" + st.name() + ":" + sf.name() + ":" + w + "x" + h + "@" + fps + "HZ");

                            sp = sp2;
                            break;
                        }
                    }
                }
            }

            depth_sensor.open(sp);
            depth_sensor.start(mFrameHandler);
        }

        if (color_sensor != null) {
            List<StreamProfile> sps = color_sensor.getStreamProfiles();
            StreamProfile sp = sps.get(0);

            for (StreamProfile sp2 : sps)
            {
                if (sp2.getType().compareTo(StreamType.COLOR) == 0) {

                    if (sp2.is(Extension.VIDEO_PROFILE)) {
                        VideoStreamProfile video_stream_profile = sp2.as(Extension.VIDEO_PROFILE);

                        // After using the "as" method we can use the new data type
                        //  for additinal operations:
                        StreamFormat sf = video_stream_profile.getFormat();
                        int index = sp2.getIndex();
                        StreamType st = sp2.getType();
                        int w = video_stream_profile.getWidth();
                        int h = video_stream_profile.getHeight();
                        int fps = video_stream_profile.getFrameRate();

                        if (w == 640 && fps == 30) {
                            Log.d(TAG, "color stream: " + index + ":" + st.name() + ":" + sf.name() + ":" + w + "x" + h + "@" + fps + "HZ");

                            sp = sp2;
                            break;
                        }
                    }
                }
            }

            color_sensor.open(sp);
            color_sensor.start(mColorFrameHandler);
        }
    }

    private synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            Log.d(TAG, "try start streaming");
            mGLSurfaceView.clear();
            frames_received = 0;
            frames_dropped = 0;
            frames_displayed = 0;

            mIsStreaming = true;

            streaming = new Thread(mStreaming);
            streaming.start();

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
            streaming.join(1000);

            if (color_sensor != null) color_sensor.stop();
            if (depth_sensor != null) depth_sensor.stop();

            if (mColorizer != null) mColorizer.close();
            if (depth_sensor != null) {depth_sensor.close();}
            if (color_sensor != null) {color_sensor.close();}

            if (mDevice != null) mDevice.close();

            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            Log.e(TAG, "failed to stop streaming");
        }
    }
}

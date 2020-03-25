package com.intel.realsense.multicam;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs multicam example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private GLRsSurfaceView mGLSurfaceView;
    private final Handler mHandler = new Handler();

    private DeviceList deviceList;
    private ArrayList<Pipeline> mPipelines;
    private ArrayList<Colorizer> mColorizers;
    private RsContext mRsContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
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
        //Release Context
        if(mRsContext != null)
            mRsContext.close();
        stop();
    }

    private void init(){
        //RsContext.init must be called once in the application lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(mAppContext);

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(mListener);
        mPipelines = new ArrayList<>();
        mColorizers = new ArrayList<>();

        start();
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            restart();
        }

        @Override
        public void onDeviceDetach() {
            restart();
        }
    };

    Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            try {
                for(int i = 0; i < mPipelines.size(); i++) {
                    try (FrameSet frames = mPipelines.get(i).waitForFrames(1000)) {
                        try (FrameSet processed = frames.applyFilter(mColorizers.get(i))) {
                            mGLSurfaceView.upload(processed);
                        }
                    }
                }
                if (mPipelines.size() > 0)
                    mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    private void configAndStart() throws Exception {
        for(int i = 0; i < mPipelines.size(); i++) {
            try (Config config = new Config()) {
                config.enableDevice(deviceList.createDevice(i).getInfo(CameraInfo.SERIAL_NUMBER));
                config.enableStream(StreamType.DEPTH, 640, 480);
                Pipeline pipe = mPipelines.get(i);
                // try statement needed here to release resources allocated by the Pipeline:start() method
                try (PipelineProfile pp = pipe.start(config)) {}
            }
        }
    }

    private synchronized void restart() {
        stop();
        start();
    }

    private synchronized void start() {
        deviceList = mRsContext.queryDevices();
        int devCount = deviceList.getDeviceCount();
        if( devCount > 0) {
            for (int i = 0; i < devCount; i++)
            {
                mPipelines.add(new Pipeline());
                mColorizers.add(new Colorizer());
            }
        }

        try{
            Log.d(TAG, "try start streaming");
            configAndStart();
            mHandler.post(mStreaming);
            mGLSurfaceView.clear();
            Log.d(TAG, "streaming started successfully");
        } catch (Exception e) {
            Log.d(TAG, "failed to start streaming");
            mGLSurfaceView.clear();
        }
    }

    private synchronized void stop() {
        try {
            Log.d(TAG, "try stop streaming");

            mHandler.removeCallbacks(mStreaming);

            for(Pipeline pipe : mPipelines)
                pipe.stop();

            //Release pipelines
            for (Pipeline pipeline : mPipelines) {
                pipeline.close();
            }
            //Release colorizers
            for (Colorizer colorizer : mColorizers) {
                colorizer.close();
            }

            mPipelines.clear();
            mColorizers.clear();
            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");

            deviceList.close();
            Log.d(TAG, "closed devices list successfully");
        } catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
        }
    }
}

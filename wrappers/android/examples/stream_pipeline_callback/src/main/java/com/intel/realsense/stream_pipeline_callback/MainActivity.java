package com.intel.realsense.stream_pipeline_callback;

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

import com.intel.realsense.librealsense.Colorizer;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.FrameCallback;

import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.MotionFrame;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.RsContext;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs pipeline callback";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceView;
    private boolean mIsStreaming = false;

    private Pipeline mPipeline;
    private Colorizer mColorizer;
    private RsContext mRsContext;

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

    private FrameCallback  mFrameHandler = new FrameCallback()
    {
        @Override
        public void onFrame(final Frame f) {

            try {
                if (f != null) {
                    Log.d(TAG, "frame received: " + f.getNumber() + ", " + f.getDataSize() + ", " + f.getProfile().getType().name());

                    if (f.is(Extension.MOTION_FRAME)) {
                        MotionFrame fm = f.as(Extension.MOTION_FRAME);
                        mGLSurfaceView.upload(fm);
                    }
                    else {
                        FrameSet fs = f.as(Extension.FRAMESET);

                        if (fs != null) {
                            try (FrameSet processed = fs.applyFilter(mColorizer)) {
                                mGLSurfaceView.upload(processed);
                            }
                        }
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

        mPipeline = new Pipeline();
        mColorizer = new Colorizer();

        try {
            int num_devices = mRsContext.queryDevices().getDeviceCount();
            Log.d(TAG, "devices found: " + num_devices);

            if (num_devices > 0) {
                showConnectLabel(false);
                start();
            }
        }
        catch (Exception e) {}
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
        //  try(Config config  = new Config())
        {
            // streaming pipeline with default configuration or custom configuration and user callback
            // the user callback must be a class implements the FrameCallback interface

            // default configuration
            //try statement needed here to release resources allocated by the Pipeline:start() method
            try(PipelineProfile pp = mPipeline.start(mFrameHandler)){}

            // custom configuration
            //    config.enableStream(StreamType.DEPTH, 640, 480);
            //    config.enableStream(StreamType.COLOR, 640, 480);
            //    try(PipelineProfile pp = mPipeline.start(config, mFrameHandler)){}
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
            Log.d(TAG, "failed to start streaming");
        }
    }

    private synchronized void stop() {
        if(!mIsStreaming)
            return;
        try {
            Log.d(TAG, "try stop streaming");
            mIsStreaming = false;

            if (mPipeline != null)
            {
                mPipeline.stop();
                mPipeline.close();
                mPipeline = null;
            }

            if (mColorizer != null) mColorizer.close();

            if(mRsContext != null) {
                mRsContext.removeDevicesChangedCallback();
                mRsContext.close();
                mRsContext = null;
            }

            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            mGLSurfaceView.clear();
            Log.e(TAG, "failed to stop streaming");
        }
    }
}

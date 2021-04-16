package com.intel.realsense.stream_pipeline_callback;

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

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;

import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;

import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.Extension;

import com.intel.realsense.librealsense.VideoStreamProfile;
import com.intel.realsense.librealsense.FrameCallback;

import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamType;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs stream callback";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceView;
    private boolean mIsStreaming = false;

    private Pipeline mPipeline;
    private Colorizer mColorizer;
    private RsContext mRsContext;

    private Device mDevice;
    Thread streaming = null;

    BlockingQueue<Frame> frameQueue = new ArrayBlockingQueue<Frame>(4);
    int frames_received = 0;
    int frames_dropped = 0;
    int frames_displayed = 0;

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

    private FrameCallback  mFrameHandler = new FrameCallback()
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

    private void init(){
        //RsContext.init must be called once in the application lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(mAppContext);

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(mListener);

        mPipeline = new Pipeline();
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

                        if (mFrame != null) {
                            FrameSet frames = mFrame.as(Extension.FRAMESET);
                            frames_displayed++;

                            if (frames != null) {
                                try (FrameSet processed = frames.applyFilter(mColorizer)) {
                                    mGLSurfaceView.upload(processed);
                                }

                                frames.close();
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
        try(Config config  = new Config())
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
            frames_received = 0;
            frames_dropped = 0;
            frames_displayed = 0;

            mIsStreaming = true;

            streaming = new Thread(mStreaming);
            streaming.start();

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
            streaming.join(1000);

            mPipeline.stop();

            if (mColorizer != null) mColorizer.close();
            if (mPipeline != null) mPipeline.close();
            mGLSurfaceView.clear();
            Log.d(TAG, "streaming stopped successfully");
        } catch (Exception e) {
            mGLSurfaceView.clear();
            Log.e(TAG, "failed to stop streaming");
        }
    }
}

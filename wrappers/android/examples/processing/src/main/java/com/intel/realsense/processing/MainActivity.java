package com.intel.realsense.processing;

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
import android.widget.Switch;
import android.widget.TextView;

import com.intel.realsense.librealsense.Align;
import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DecimationFilter;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameReleaser;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.HoleFillingFilter;
import com.intel.realsense.librealsense.Option;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.PipelineProfile;
import com.intel.realsense.librealsense.Pointcloud;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.SpatialFilter;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.TemporalFilter;
import com.intel.realsense.librealsense.ThresholdFilter;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs process example";
    private static final int PERMISSIONS_REQUEST_CAMERA = 0;

    private boolean mPermissionsGranted = false;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLRsSurfaceView mGLSurfaceViewOrg;
    private GLRsSurfaceView mGLSurfaceViewProcessed;
    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private Pipeline mPipeline;

    //filters
    private Align mAlign;
    private Colorizer mColorizerOrg;
    private Colorizer mColorizerProcessed;
    private DecimationFilter mDecimationFilter;
    private HoleFillingFilter mHoleFillingFilter;
    private Pointcloud mPointcloud;
    private TemporalFilter mTemporalFilter;
    private ThresholdFilter mThresholdFilter;
    private SpatialFilter mSpatialFilter;

    private RsContext mRsContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
        mBackGroundText = findViewById(R.id.connectCameraText);

        mGLSurfaceViewOrg = findViewById(R.id.glSurfaceViewOrg);
        mGLSurfaceViewOrg.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

        mGLSurfaceViewProcessed = findViewById(R.id.glSurfaceViewProcessed);
        mGLSurfaceViewProcessed.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE
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
        mGLSurfaceViewOrg.close();
        mGLSurfaceViewProcessed.close();
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
        mPipeline.close();
    }

    private void init(){
        //RsContext.init must be called once in the application lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(mAppContext);

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(mListener);

        mPipeline = new Pipeline();

        //init filters
        mAlign = new Align(StreamType.COLOR);
        mColorizerOrg = new Colorizer();
        mColorizerProcessed = new Colorizer();
        mDecimationFilter = new DecimationFilter();
        mHoleFillingFilter = new HoleFillingFilter();
        mPointcloud = new Pointcloud();
        mTemporalFilter = new TemporalFilter();
        mThresholdFilter = new ThresholdFilter();
        mSpatialFilter = new SpatialFilter();

        //config filters
        mThresholdFilter.setValue(Option.MIN_DISTANCE, 0.1f);
        mThresholdFilter.setValue(Option.MAX_DISTANCE, 0.8f);

        mDecimationFilter.setValue(Option.FILTER_MAGNITUDE, 8);

        try(DeviceList dl = mRsContext.queryDevices()){
            if(dl.getDeviceCount() > 0) {
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
            try {
                try(FrameReleaser fr = new FrameReleaser()){
                    FrameSet frames = mPipeline.waitForFrames(1000).releaseWith(fr);
                    FrameSet orgSet = frames.applyFilter(mColorizerOrg).releaseWith(fr);
                    FrameSet processedSet = frames.applyFilter(mDecimationFilter).releaseWith(fr).
                            applyFilter(mHoleFillingFilter).releaseWith(fr).
                            applyFilter(mTemporalFilter).releaseWith(fr).
                            applyFilter(mSpatialFilter).releaseWith(fr).
                            applyFilter(mThresholdFilter).releaseWith(fr).
                            applyFilter(mColorizerProcessed).releaseWith(fr).
                            applyFilter(mAlign).releaseWith(fr);
                    try(Frame org = orgSet.first(StreamType.DEPTH, StreamFormat.RGB8).releaseWith(fr)){
                        try(Frame processed = processedSet.first(StreamType.DEPTH, StreamFormat.RGB8).releaseWith(fr)){
                            mGLSurfaceViewOrg.upload(org);
                            mGLSurfaceViewProcessed.upload(processed);
                        }
                    }
                }
                mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    private void configAndStart() throws Exception {
        try(Config config  = new Config())
        {
            config.enableStream(StreamType.DEPTH, 640, 480);
            config.enableStream(StreamType.COLOR, 640, 480);
            // try statement needed here to release resources allocated by the Pipeline:start() method
            try(PipelineProfile pp = mPipeline.start(config)){}
        }
    }

    private synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            Log.d(TAG, "try start streaming");
            mGLSurfaceViewOrg.clear();
            mGLSurfaceViewProcessed.clear();
            configAndStart();
            mIsStreaming = true;
            mHandler.post(mStreaming);
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
            mHandler.removeCallbacks(mStreaming);
            mPipeline.stop();
            Log.d(TAG, "streaming stopped successfully");
            mGLSurfaceViewOrg.clear();
            mGLSurfaceViewProcessed.clear();
        }  catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
            mPipeline = null;
            mColorizerOrg.close();
            mColorizerProcessed.close();
        }
    }
}

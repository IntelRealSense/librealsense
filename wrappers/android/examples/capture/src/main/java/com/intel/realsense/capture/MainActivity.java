package com.intel.realsense.capture;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.DeviceManager;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "lrs capture example";
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;

    private Context mAppContext;
    private TextView mBackGroundText;
    private GLSurfaceView mGlView;
    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private GLRenderer mRenderer;

    private Pipeline mPipeline;
    private Config mConfig;
    private Colorizer mColorizer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mAppContext = getApplicationContext();
        mBackGroundText = findViewById(R.id.connectCameraText);
        mGlView = findViewById(R.id.glSurfaceView);
        mRenderer = new GLRenderer();
        mGlView.setRenderer(mRenderer);

        // Android 9 also requires camera permissions
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_CONTACTS}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        init();
    }

    @Override
    protected void onPause() {
        super.onPause();
        stop();
    }

    void init(){
        //DeviceManager must be initialized before any interaction with physical RealSense devices.
        DeviceManager.init(mAppContext);

        //The UsbHub provides notifications regarding RealSense devices attach/detach events via the DeviceListener.
        DeviceManager.getUsbHub().addListener(mListener);

        mPipeline = new Pipeline();
        mConfig  = new Config();
        mColorizer = new Colorizer();

        mConfig.enableStream(StreamType.DEPTH, 640, 480);
        mConfig.enableStream(StreamType.COLOR, 640, 480);

        if(DeviceManager.getUsbHub().getDeviceCount() > 0) {
            showConnectLabel(false);
            start();
        }
    }

    private void showConnectLabel(final boolean state){
        mRenderer.clean();
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBackGroundText.setVisibility(state ? View.VISIBLE : View.GONE);
            }
        });
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() throws Exception {
            showConnectLabel(false);
        }

        @Override
        public void onDeviceDetach() throws Exception {
            showConnectLabel(true);
            stop();
        }
    };

    Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frames = mPipeline.waitForFrames(1000))
                {
                    try(FrameSet processed = frames.applyFilter(mColorizer)) {
                        processed.foreach(new FrameSet.FrameCallback() {
                            @Override
                            public void onFrame(Frame f) throws Exception {
                                mRenderer.show(f.as(VideoFrame.class));
                            }
                        });
                    }
                }
                mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    private synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            Log.d(TAG, "try start streaming");
            mPipeline.start(mConfig);
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
        }  catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
            mPipeline = null;
        }
    }
}

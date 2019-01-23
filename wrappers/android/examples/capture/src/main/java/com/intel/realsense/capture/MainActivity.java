package com.intel.realsense.capture;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.Decimation;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.DeviceManager;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "lrs capture example";
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;

    private android.content.Context mContext = this;
    private Button mStartStopButton;

    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private FrameViewer mColorFrameViewer;
    private FrameViewer mDepthFrameViewer;

    private Config mConfig = new Config();
    private Pipeline mPipeline;

    private Decimation mDecimation = new Decimation();

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            mPipeline = new Pipeline();
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mStartStopButton.setVisibility(View.VISIBLE);
                }
            });
        }

        @Override
        public void onDeviceDetach() {
            mPipeline = null;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mStartStopButton.setVisibility(View.GONE);
                }
            });
            try {
                mPipeline.close();
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            }
            mPipeline = null;
            stop();
        }
    };

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_CONTACTS}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        init();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Android 9 also requires camera permissions
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        init();
    }

    void init(){
        //DeviceManager must be initialized before any interaction with physical RealSense devices.
        DeviceManager.init(mContext);

        //The UsbHub provides notifications regarding RealSense devices attach/detach events via the DeviceListener.
        DeviceManager.getUsbHub().addListener(mListener);

        mColorFrameViewer = new FrameViewer((ImageView) findViewById(R.id.colorImageView));
        mDepthFrameViewer = new FrameViewer((ImageView) findViewById(R.id.depthImageView));

        mStartStopButton = findViewById(R.id.btnStart);

        mStartStopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsStreaming) {
                    stop();
                } else {
                    start();
                }
            }
        });

        mConfig.enableStream(StreamType.DEPTH, 640, 480);
        mConfig.enableStream(StreamType.COLOR, 640, 480, StreamFormat.RGBA8);
    }

    Runnable updateBitmap = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frames = mPipeline.waitForFrames())
                {
                    try(FrameSet processed = frames.applyFilter(mDecimation)) {
                        try(Frame f = processed.first(StreamType.COLOR)) {
                            mColorFrameViewer.show(MainActivity.this, f.as(VideoFrame.class));
                        }
                        try(Frame f = processed.first(StreamType.DEPTH)) {
                            mDepthFrameViewer.show(MainActivity.this, f.as(VideoFrame.class));
                        }
                    }
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage());
                }
            } finally {
                mHandler.post(updateBitmap);
            }
        }
    };

    synchronized void start(){
        if(mIsStreaming)
            return;
        mIsStreaming = true;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mStartStopButton.setText(R.string.stream_stop);
            }
        });
        startRepeatingTask();
    }

    synchronized void stop(){
        if(mIsStreaming == false)
            return;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mStartStopButton.setText(R.string.stream_start);
            }
        });
        stopRepeatingTask();
        mIsStreaming = false;
    }

    void startRepeatingTask() {
        try {
            mPipeline.start(mConfig);
            updateBitmap.run();
        } catch (Exception e){
            Log.e(TAG, e.getMessage());
        }
    }

    void stopRepeatingTask() {
        mHandler.removeCallbacks(updateBitmap);
        if(mPipeline != null)
            mPipeline.stop();
    }
}

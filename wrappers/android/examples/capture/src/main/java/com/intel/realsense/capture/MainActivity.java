package com.intel.realsense.capture;

import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.UsbHostManager;
import com.intel.realsense.librealsense.VideoFrame;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "lrs capture example";

    private android.content.Context mContext = this;
    private Button mStartStopButton;

    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();

    private FrameViewer mColorFrameViewer;
    private FrameViewer mDepthFrameViewer;

    private Config mConfig = new Config();
    private Pipeline mPipeline;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        UsbHostManager.init(mContext);
        UsbHostManager.addListener(mListener);

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

        mConfig.enable_stream(StreamType.DEPTH, 640, 480);
        mConfig.enable_stream(StreamType.COLOR, 640, 480, StreamFormat.RGBA8);
    }

    Runnable updateBitmap = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frameset = mPipeline.waitForFrames())
                {
                    try(Frame f = frameset.first(StreamType.COLOR)) {
                        mColorFrameViewer.show(MainActivity.this, f.as(VideoFrame.class));
                    }
                    try(Frame f = frameset.first(StreamType.DEPTH)) {
                        mDepthFrameViewer.show(MainActivity.this, f.as(VideoFrame.class));
                    }
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage());
                }
            } finally {
                mHandler.post(updateBitmap);
            }
        }
    };

    private UsbHostManager.Listener mListener = new UsbHostManager.Listener() {
        @Override
        public void onUsbDeviceAttach() {
            mPipeline = new Pipeline();
        }

        @Override
        public void onUsbDeviceDetach() {
            stop();
        }
    };

    synchronized void start(){
        if(mIsStreaming)
            return;
        mIsStreaming = true;
        mStartStopButton.setText(R.string.stream_stop);
        startRepeatingTask();
    }

    synchronized void stop(){
        if(mIsStreaming == false)
            return;
        mStartStopButton.setText(R.string.stream_start);
        stopRepeatingTask();
        mIsStreaming = false;
    }

    void startRepeatingTask() {
        mPipeline.start(mConfig);
        updateBitmap.run();
    }

    void stopRepeatingTask() {
        mHandler.removeCallbacks(updateBitmap);
        mPipeline.stop();
    }
}

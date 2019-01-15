package com.intel.realsense.capture;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
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

import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {
    private android.content.Context mContext = this;
    private ImageView mColorImageView;
    private ImageView mDepthImageView;
    private Button mStartStopButton;

    private Integer mWidth = 640;
    private Integer mHeight = 480;
    private final Bitmap mColorBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
    private final Bitmap mDepthBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.RGB_565);
    private ByteBuffer mColorBuff = ByteBuffer.allocateDirect(mWidth * mHeight * 4);
    private ByteBuffer mDepthBuff = ByteBuffer.allocateDirect(mWidth * mHeight * 2);

    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();
    private Config mConfig = new Config();
    private Pipeline mPipeline;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        UsbHostManager.init(mContext);
        UsbHostManager.addListener(mListener);

        mColorImageView = findViewById(R.id.colorImageView);
        mDepthImageView = findViewById(R.id.depthImageView);
        mStartStopButton = findViewById(R.id.btnStart);

        mStartStopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsStreaming) {
                    mStartStopButton.setText(R.string.stream_start);
                    stopRepeatingTask();
                    mIsStreaming = false;
                } else {
                    mIsStreaming = true;
                    mStartStopButton.setText(R.string.stream_stop);
                    startRepeatingTask();
                }
            }
        });

        mConfig.enable_stream(StreamType.DEPTH, mWidth, mHeight);
        mConfig.enable_stream(StreamType.COLOR, mWidth, mHeight, StreamFormat.RGBA8);
    }

    Runnable updateBitmap = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frameset = mPipeline.waitForFrames())
                {
                    try(Frame cf = frameset.first(StreamType.COLOR)) {
                        cf.getData(mColorBuff.array());
                    }
                    mColorBuff.rewind();
                    mColorBitmap.copyPixelsFromBuffer(mColorBuff);

                    try(Frame df = frameset.first(StreamType.DEPTH)) {
                        df.getData(mDepthBuff.array());
                    }
                    mDepthBuff.rewind();
                    mDepthBitmap.copyPixelsFromBuffer(mDepthBuff);

                    runOnUiThread(new Runnable(){
                        public void run() {
                            mColorImageView.setImageBitmap(mColorBitmap);
                            mDepthImageView.setImageBitmap(mDepthBitmap);
                        }
                    });

                } catch (Exception e) {
                    e.printStackTrace();
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

        }
    };

    void startRepeatingTask() {
        mPipeline.start(mConfig);
        updateBitmap.run();
    }

    void stopRepeatingTask() {
        mHandler.removeCallbacks(updateBitmap);
        mPipeline.stop();
    }
}

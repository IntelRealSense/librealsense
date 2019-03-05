package com.intel.realsense.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.util.Log;

import com.intel.realsense.librealsense.Colorizer;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.StreamType;

public class Streamer {
    private static final String TAG = "librs camera streamer";

    interface Listener{
        void config(Config config);
    }

    private final Context mContext;
    private final GLRsSurfaceView mGLSurfaceView;

    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();
    private final Listener mListener;

    private Pipeline mPipeline;
    private Colorizer mColorizer = new Colorizer();

    public Streamer(Context context,GLRsSurfaceView view, Listener listener){
        mGLSurfaceView = view;
        mContext = context;
        mListener = listener;
    }

    private Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frames = mPipeline.waitForFrames(1000)) {
                    try (FrameSet processed = frames.applyFilter(mColorizer)) {
                        mGLSurfaceView.upload(processed);
                    }
                }
                mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    void configStream(Config config, StreamType streamType){
        SharedPreferences sharedPref = mContext.getSharedPreferences(mContext.getString(R.string.app_settings), Context.MODE_PRIVATE);
        if(sharedPref.getBoolean(streamType.name(), false))
            config.enableStream(streamType);
    }

    void configAndStart() throws Exception {
        try(Config config = new Config()){
            configStream(config, StreamType.COLOR);
            configStream(config, StreamType.DEPTH);
            configStream(config, StreamType.INFRARED);
            if(mListener != null)
                mListener.config(config);
            mPipeline.start(config);
        }
    }

    public synchronized void start() {
        if(mIsStreaming)
            return;
        try{
            mPipeline = new Pipeline();
            mGLSurfaceView.clear();
            Log.d(TAG, "try start streaming");
            configAndStart();
            mIsStreaming = true;
            mHandler.post(mStreaming);
            Log.d(TAG, "streaming started successfully");
        } catch (Exception e) {
            Log.d(TAG, "failed to start streaming");
        }
    }

    public synchronized void stop() {
        if(!mIsStreaming)
            return;
        try {
            Log.d(TAG, "try stop streaming");
            mIsStreaming = false;
            mHandler.removeCallbacks(mStreaming);
            mPipeline.stop();
            mPipeline.close();
            Log.d(TAG, "streaming stopped successfully");
        }  catch (Exception e) {
            Log.d(TAG, "failed to stop streaming");
            mPipeline = null;
        }
    }
}

package com.intel.realsense.camera;

import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.util.HashMap;
import java.util.Map;

public class StreamingStats {
    private static final String TAG = "librs camera streamer";

    private Map<Integer, Statistics> mStreamsMap = new HashMap<>();
    private Map<Integer, Integer> mLastFrames = new HashMap<>();

    private void initStream(StreamProfile profile){
        String resolution = "";
        if(profile.is(Extension.VIDEO_PROFILE)){
            VideoStreamProfile vsp = profile.as(Extension.VIDEO_PROFILE);
            resolution = vsp.getWidth() + "x" + vsp.getHeight() + " | ";
        }
        String name = profile.getType().name() + " | " +
                profile.getFormat().name() + " | " +
                resolution +
                profile.getFrameRate();
        mStreamsMap.put(profile.getUniqueId(), new Statistics(name));
        mLastFrames.put(profile.getUniqueId(), 0);
    }

    private FrameCallback mFrameCallback = new FrameCallback() {
        @Override
        public void onFrame(Frame f) {
            try(StreamProfile profile = f.getProfile()) {
                if (!mLastFrames.containsKey(f.getProfile().getUniqueId()))
                    initStream(profile);
                if (mLastFrames.get(f.getProfile().getUniqueId()) != f.getNumber()) {
                    mLastFrames.put(f.getProfile().getUniqueId(), f.getNumber());
                    mStreamsMap.get(f.getProfile().getUniqueId()).onFrame();
                }
                else
                    mStreamsMap.get(f.getProfile().getUniqueId()).kick();
            }
        }
    };

    public void onFrameset(FrameSet frames){
        frames.foreach(mFrameCallback);
    }

    public String prettyPrint(){
        String rv = "";
        for(Map.Entry e : mStreamsMap.entrySet()){
            rv += ((Statistics)e.getValue()).prettyPrint() + "\n\n";
        }
        return rv;
    }

    private class Statistics{
        private final String mName;
        private long mStartTime;
        private long mBaseTime;
        private float mFps;
        private long mFrameCount;
        private long mTotalFrameCount;
        private long mFirstFrameLatency;

        public void reset(){
            mStartTime = mBaseTime = System.currentTimeMillis();
            mFirstFrameLatency = 0;
        }

        public Statistics(String name) {
            mName = name;
            reset();
        }

        public float getFps(){
            return mFps;
        }
        public float getFrameCount(){
            return mTotalFrameCount;
        }
        public String prettyPrint(){
            long curr = System.currentTimeMillis();
            int diffInSeconds = (int) ((curr - mStartTime) * 0.001);
            return mName +
                    "\nFrame Rate: " + mFps +
                    "\nFrame Count: " + mTotalFrameCount +
                    "\nRun Time: " + diffInSeconds + " [sec]";
        }

        public synchronized void kick(){
            long curr = System.currentTimeMillis();
            float diffInSeconds = (float) ((curr - mBaseTime) * 0.001);
            if(diffInSeconds > 2){
                mFps = mFrameCount / diffInSeconds;
                mFrameCount = 0;
                mBaseTime = curr;
            }
        }

        public synchronized void onFrame(){
            mFrameCount++;
            mTotalFrameCount++;
            long curr = System.currentTimeMillis();
            float diffInSeconds = (float) ((curr - mBaseTime) * 0.001);
            if(mFirstFrameLatency == 0){
                mFirstFrameLatency = curr - mBaseTime;
            }
            if(diffInSeconds > 2){
                mFps = mFrameCount / diffInSeconds;
                mFrameCount = 0;
                mBaseTime = curr;
            }
        }
    }


}

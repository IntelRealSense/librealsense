package com.intel.realsense.camera;

import android.support.annotation.NonNull;

import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.util.List;

public class StreamProfileSelector implements Comparable<StreamProfileSelector> {
    private StreamFormat mFormat = StreamFormat.ANY;
    private int mWidth = 0;
    private int mHeight = 0;
    private int mFps = 0;
    private boolean mEnabled;
    private int mIndex;
    private List<VideoStreamProfile> mProfiles;

    public StreamProfileSelector(boolean enable, int index, List<VideoStreamProfile> profiles)
    {
        mIndex = index < profiles.size() ? index : 0;
        mEnabled = enable;
        mProfiles = profiles;
    }

    public boolean isEnabled() { return mEnabled; }

    public List<VideoStreamProfile> getProfiles() { return mProfiles; }

    public int getIndex() { return mIndex; }

    public String getName(){
        VideoStreamProfile p = getProfile();
        if(p.getType() == StreamType.INFRARED)
            return p.getType().name() + "-" + p.getIndex();
        return p.getType().name();
    }

    public VideoStreamProfile getProfile() {
        if(mIndex < 0 || mIndex >= mProfiles.size())
            return mProfiles.get(0);
        return mProfiles.get(mIndex);
    }

    public String getResolutionString() { return String.valueOf(getProfile().getWidth()) + "x" + String.valueOf(getProfile().getHeight()); }

    private int findIndex(StreamFormat format, int fps, int width, int height){
        int currIndex = 0;
        for(VideoStreamProfile vsp : mProfiles){
            if(vsp.getFormat() == format &&
                    vsp.getWidth() == width &&
                    vsp.getHeight() == height &&
                    vsp.getFrameRate() == fps){
                return currIndex;
            }
            currIndex++;
        }
        return -1;
    }

    public void updateFormat(String str) {
        mFormat = StreamFormat.valueOf(str);
        mIndex = findIndex(mFormat, mFps, mWidth, mHeight);
    }

    public void updateResolution(String str) {
        mWidth = Integer.parseInt(str.split("x")[0]);
        mHeight = Integer.parseInt(str.split("x")[1]);
        mIndex = findIndex(mFormat, mFps, mWidth, mHeight);
    }

    public void updateFrameRate(String str) {
        mFps = Integer.parseInt(str);
        mIndex = findIndex(mFormat, mFps, mWidth, mHeight);
    }

    public void updateEnabled(boolean state) {
        mEnabled = state;
        mIndex = findIndex(mFormat, mFps, mWidth, mHeight);
    }

    @Override
    public int compareTo(@NonNull StreamProfileSelector streamProfileSelector) {
        return getName().compareTo(streamProfileSelector.getName());
    }
}

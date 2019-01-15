package com.intel.realsense.librealsense;

public class VideoStreamProfile extends StreamProfile {

    private int mWidth;
    private int mHeight;

    VideoStreamProfile(long handle) {
        super(handle);
        int error = 0;
        nGetResolution(mHandle, mWidth, mHeight, error);
    }

    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    private native void nGetResolution(long handle, int width, int height, int error);
}

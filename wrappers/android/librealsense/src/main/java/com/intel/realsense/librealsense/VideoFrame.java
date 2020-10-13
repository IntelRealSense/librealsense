package com.intel.realsense.librealsense;

public class VideoFrame extends Frame {
    private int mWidth = -1;
    private int mHeight = -1;
    private int mStride = -1;
    private int mBitsPerPixel = -1;

    public int getWidth(){
        if(mWidth == -1)
            mWidth = nGetWidth(mHandle);
        return mWidth;
    }

    public int getHeight(){
        if(mHeight == -1)
            mHeight = nGetHeight(mHandle);
        return mHeight;
    }

    public int getStride(){
        if(mStride == -1)
            mStride = nGetStride(mHandle);
        return mStride;
    }

    public int getBitsPerPixel(){
        if(mBitsPerPixel == -1)
            mBitsPerPixel = nGetBitsPerPixel(mHandle);
        return mBitsPerPixel;
    }

    public VideoStreamProfile getProfile() {
        return new VideoStreamProfile(nGetStreamProfile(mHandle));
    }

    protected VideoFrame(long handle) {
        super(handle);
        mOwner = false;
    }

    private static native int nGetWidth(long handle);
    private static native int nGetHeight(long handle);
    private static native int nGetStride(long handle);
    private static native int nGetBitsPerPixel(long handle);
}

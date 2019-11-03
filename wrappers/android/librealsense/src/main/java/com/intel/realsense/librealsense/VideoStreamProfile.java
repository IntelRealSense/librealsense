package com.intel.realsense.librealsense;

public class VideoStreamProfile extends StreamProfile {
    ResolutionParams mResolutionParams;

    private class ResolutionParams {
        public int width;
        public int height;
    }

    VideoStreamProfile(long handle) {
        super(handle);
        mOwner = false;
        mResolutionParams = new ResolutionParams();
        nGetResolution(mHandle, mResolutionParams);
    }

    public int getWidth() {
        return mResolutionParams.width;
    }

    public int getHeight() {
        return mResolutionParams.height;
    }

    private static native void nGetResolution(long handle, ResolutionParams params);
}

package com.intel.realsense.librealsense;

public class VideoStreamProfile extends StreamProfile {
    ResolutionParams mResolutionParams;
    private Intrinsics mIntrinsics;

    private class ResolutionParams {
        public int width;
        public int height;
    }

    VideoStreamProfile(long handle) {
        super(handle);
        mOwner = false;
        mResolutionParams = new ResolutionParams();
        nGetResolution(mHandle, mResolutionParams);
        mIntrinsics = null;
    }

    public Intrinsics getIntrinsics() throws Exception {
        if(mIntrinsics == null){
            mIntrinsics = new Intrinsics();
            nGetIntrinsics(mHandle, mIntrinsics);
            mIntrinsics.SetModel();
        }
        return mIntrinsics;
    }

    public int getWidth() {
        return mResolutionParams.width;
    }

    public int getHeight() {
        return mResolutionParams.height;
    }

    private static native void nGetResolution(long handle, ResolutionParams params);
    private static native void nGetIntrinsics(long handle, Intrinsics intrinsics);
}

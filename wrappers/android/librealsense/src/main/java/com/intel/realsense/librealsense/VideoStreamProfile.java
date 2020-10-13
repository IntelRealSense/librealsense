package com.intel.realsense.librealsense;

public class VideoStreamProfile extends StreamProfile {
    ResolutionParams mResolutionParams;
    private Intrinsic mIntrinsic;

    private class ResolutionParams {
        public int width;
        public int height;
    }

    VideoStreamProfile(long handle) {
        super(handle);
        mOwner = false;
        mResolutionParams = new ResolutionParams();
        nGetResolution(mHandle, mResolutionParams);
        mIntrinsic = null;
    }

    public Intrinsic getIntrinsic() throws Exception {
        if(mIntrinsic == null){
            mIntrinsic = new Intrinsic();
            nGetIntrinsic(mHandle, mIntrinsic);
            mIntrinsic.SetModel();
        }
        return mIntrinsic;
    }

    public int getWidth() {
        return mResolutionParams.width;
    }

    public int getHeight() {
        return mResolutionParams.height;
    }

    private static native void nGetResolution(long handle, ResolutionParams params);
    private static native void nGetIntrinsic(long handle, Intrinsic intrinsic);
}

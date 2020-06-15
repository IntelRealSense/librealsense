package com.intel.realsense.librealsense;

public class MotionStreamProfile extends StreamProfile {

    private MotionIntrinsics mIntrinsics;

    MotionStreamProfile(long handle) {
        super(handle);
        mOwner = false;
        mIntrinsics = null;
    }

    public MotionIntrinsics getIntrinsics() throws Exception {
        if(mIntrinsics == null){
            mIntrinsics = new MotionIntrinsics();
            nGetIntrinsics(mHandle, mIntrinsics);
        }
        return mIntrinsics;
    }

    private static native void nGetIntrinsics(long handle, MotionIntrinsics intrinsics);
}

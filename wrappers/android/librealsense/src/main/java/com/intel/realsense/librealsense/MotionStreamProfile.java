package com.intel.realsense.librealsense;

public class MotionStreamProfile extends StreamProfile {

    private MotionIntrinsic mIntrinsic;

    MotionStreamProfile(long handle) {
        super(handle);
        mOwner = false;
        mIntrinsic = null;
    }

    public MotionIntrinsic getIntrinsic() throws Exception {
        if(mIntrinsic == null){
            mIntrinsic = new MotionIntrinsic();
            nGetIntrinsic(mHandle, mIntrinsic);
        }
        return mIntrinsic;
    }

    private static native void nGetIntrinsic(long handle, MotionIntrinsic intrinsic);
}

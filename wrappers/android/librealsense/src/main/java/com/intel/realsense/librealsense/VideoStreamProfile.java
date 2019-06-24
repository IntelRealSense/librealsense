package com.intel.realsense.librealsense;

public class VideoStreamProfile extends StreamProfile {
    IntrinsicsParams mIntrinsicsParams;

    private class IntrinsicsParams {
        public int width;
        public int height;
        public float fx;
        public float fy;
        public float ppx;
        public float ppy;
    }

    VideoStreamProfile(long handle) {
        super(handle);
        mIntrinsicsParams = new IntrinsicsParams();
        nGetIntrinsics(mHandle, mIntrinsicsParams);
    }

    public int getWidth() {
        return mIntrinsicsParams.width;
    }

    public int getHeight() {
        return mIntrinsicsParams.height;
    }

    public float getFx() {
        return mIntrinsicsParams.fx;
    }

    public float getFy() {
        return mIntrinsicsParams.fy;
    }

    public float getPpx() {
        return mIntrinsicsParams.ppx;
    }

    public float getPpy() {return mIntrinsicsParams.ppy;  }


    private static native void nGetIntrinsics(long handle, IntrinsicsParams params);
}

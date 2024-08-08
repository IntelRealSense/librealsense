package com.intel.realsense.librealsense;

public class DepthFrame extends VideoFrame {
    protected DepthFrame(long handle) {
        super(handle);
        mOwner = false;
    }

    public float getDistance(int x, int y) {
        return nGetDistance(mHandle, x, y);
    }

    public float getUnits() {
        return nGetUnits(mHandle);
    }

    private static native float nGetDistance(long handle, int x, int y);
    private static native float nGetUnits(long handle);
}

package com.intel.realsense.librealsense;

public class DepthFrame extends VideoFrame {
    protected DepthFrame(long handle) {
        super(handle);
    }

    public float getDistance(int x, int y) {
        return nGetDistance(mHandle, x, y);
    }

    private static native float nGetDistance(long handle, int x, int y);
}

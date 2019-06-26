package com.intel.realsense.librealsense;

abstract class LrsClass implements AutoCloseable {
    static {
        System.loadLibrary("realsense2");
    }

    protected long mHandle = 0;
    protected boolean mOwner = true;

    public long getHandle() { return mHandle; }
}

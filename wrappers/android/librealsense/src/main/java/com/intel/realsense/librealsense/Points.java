package com.intel.realsense.librealsense;

public class Points extends Frame {
    protected Points(long handle) {
        super(handle);
    }

    public int getCount(){
        return nGetCount(mHandle);
    }

    private static native int nGetCount(long handle);
}

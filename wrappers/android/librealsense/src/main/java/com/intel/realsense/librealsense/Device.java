package com.intel.realsense.librealsense;

public class Device extends LrsClass {

    public Device(long handle){
        mHandle = handle;
    }

    public boolean supportsInfo(CameraInfo info){
        return nSupportsInfo(mHandle, info.value());
    }

    public String getInfo(CameraInfo info){
        return nGetInfo(mHandle, info.value());
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    private static native boolean nSupportsInfo(long handle, int info);
    private static native String nGetInfo(long handle, int info);
    private static native void nRelease(long handle);
}

package com.intel.realsense.librealsense;

public class PipelineProfile extends LrsClass {

    PipelineProfile(long handle){
        mHandle = handle;
    }

    public Device getDevice() {
        return new Device(nGetDevice(mHandle));
    }

    @Override
    public void close() {
        nDelete(mHandle);
    }

    private static native void nDelete(long handle);
    private static native long nGetDevice(long handle);
}

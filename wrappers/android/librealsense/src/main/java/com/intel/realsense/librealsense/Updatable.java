package com.intel.realsense.librealsense;

public class Updatable extends Device {
    public void enterUpdateState() {
        nEnterUpdateState(mHandle);
    }

    Updatable(long handle){
        super(handle);
    }

    private static native void nEnterUpdateState(long handle);
}

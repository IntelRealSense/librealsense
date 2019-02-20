package com.intel.realsense.librealsense;

public class Decimation extends Filter {

    public Decimation(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}

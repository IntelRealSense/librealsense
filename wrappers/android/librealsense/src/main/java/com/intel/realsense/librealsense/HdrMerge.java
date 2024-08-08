package com.intel.realsense.librealsense;

public class HdrMerge extends Filter {

    public HdrMerge(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}

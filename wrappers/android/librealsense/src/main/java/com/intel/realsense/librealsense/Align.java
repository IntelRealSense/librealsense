package com.intel.realsense.librealsense;

public class Align extends Filter {

    public Align(StreamType alignTo){
        mHandle = nCreate(mQueue.getHandle(), alignTo.value());
    }

    private static native long nCreate(long queueHandle, int alignTo);
}

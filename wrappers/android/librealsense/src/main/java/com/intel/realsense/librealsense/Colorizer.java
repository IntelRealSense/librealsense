package com.intel.realsense.librealsense;

public class Colorizer extends Filter {

    public Colorizer() {
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}

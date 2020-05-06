package com.intel.realsense.librealsense;

// convert YUYV from color stream into RGB8 format
public class YuyDecoder extends Filter {
    public YuyDecoder(){
        mHandle = nCreate(mQueue.getHandle());
        setValue(Option.STREAM_FILTER, StreamType.COLOR.value());
    }

    private static native long nCreate(long queueHandle);
}

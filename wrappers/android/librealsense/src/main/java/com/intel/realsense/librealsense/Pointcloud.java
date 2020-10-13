package com.intel.realsense.librealsense;


public class Pointcloud extends Filter {

    public Pointcloud(){
        mHandle = nCreate(mQueue.getHandle());
    }

    public Pointcloud(StreamType texture){
        mHandle = nCreate(mQueue.getHandle());
        setValue(Option.STREAM_FILTER, texture.value());
    }

    public Pointcloud(StreamType texture, int index){
        mHandle = nCreate(mQueue.getHandle());
        setValue(Option.STREAM_FILTER, texture.value());
        setValue(Option.STREAM_INDEX_FILTER, index);
    }

    private static native long nCreate(long queueHandle);
}

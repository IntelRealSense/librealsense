package com.intel.realsense.librealsense;

public class Config extends LrsClass {

    public Config(){
        mHandle = nCreate();
    }

    public void enable_stream(StreamType type) {
        enable_stream(type, -1, 0, 0, StreamFormat.ANY, 0);
    }

    public void enable_stream(StreamType type, StreamFormat format){
        enable_stream(type, -1, 0, 0, format, 0);
    }

    public void enable_stream(StreamType type, int width, int height) {
        enable_stream(type, -1, width, height, StreamFormat.ANY, 0);
    }

    public void enable_stream(StreamType type, int width, int height, StreamFormat format){
        enable_stream(type, -1, width, height, format, 0);
    }

    public void enable_stream(StreamType type, int index, int width, int height, StreamFormat format, int framerate){
        nEnableStream(mHandle, type.value(), index, width, height, format.value(), framerate);
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
    }

    private static native long nCreate();
    private static native void nDelete(long handle);
    private static native void nEnableStream(long handle, int type, int index, int width, int height, int format, int framerate);
}

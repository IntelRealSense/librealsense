package com.intel.realsense.librealsense;

public class Frame extends LrsClass {

    protected Frame(long handle){
        mHandle = handle;
    }

    public static Frame create(long handle){
        if (nIsFrameExtendableTo(handle, Extension.VIDEO_FRAME.value()))
            return new VideoFrame(handle);
        return null;
    }

    public StreamProfile getProfile() {
        return new StreamProfile(nGetStreamProfile(mHandle));
    }

    public void getData(byte[] data) {
        nGetData(mHandle, data);
    }

    public <T extends Frame> T as(Class<T> type) {
        return (T) this;
    }

    public int getNumber(){
        return nGetNumber(mHandle);
    }

    @Override
    public void close() throws Exception {
        nRelease(mHandle);
    }

    private static native boolean nIsFrameExtendableTo(long handle, int extension);
    private static native void nAddRef(long handle);
    private static native void nRelease(long handle);
    protected static native long nGetStreamProfile(long handle);
    private static native void nGetData(long handle, byte[] data);
    private static native int nGetNumber(long handle);
}

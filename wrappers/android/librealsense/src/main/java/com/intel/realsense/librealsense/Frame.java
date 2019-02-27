package com.intel.realsense.librealsense;

public class Frame extends LrsClass implements Cloneable{

    protected Frame(long handle){
        mHandle = handle;
    }

    public static Frame create(long handle){
        if (nIsFrameExtendableTo(handle, Extension.POINTS.value()))
            return new Points(handle);
        if (nIsFrameExtendableTo(handle, Extension.DEPTH_FRAME.value()))
            return new DepthFrame(handle);
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

    public double getTimestamp(){
        return nGetTimestamp(mHandle);
    }

    public TimestampDomain getTimestampDomain() {
        int rv = nGetTimestampDomain(mHandle);
        return TimestampDomain.values()[rv];
    }

    public long getMetadata(FrameMetadata type) { return nGetMetadata(mHandle, type.value());}

    public Frame applyFilter(FilterInterface filter) {
        return filter.process(this);
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    @Override
    public Frame clone() {
        Frame rv = Frame.create(mHandle);
        nAddRef(mHandle);
        return rv;
    }

    private static native boolean nIsFrameExtendableTo(long handle, int extension);
    private static native void nAddRef(long handle);
    private static native void nRelease(long handle);
    protected static native long nGetStreamProfile(long handle);
    private static native void nGetData(long handle, byte[] data);
    private static native int nGetNumber(long handle);
    private static native double nGetTimestamp(long handle);
    private static native int nGetTimestampDomain(long handle);
    private static native long nGetMetadata(long handle, int metadata_type);
}

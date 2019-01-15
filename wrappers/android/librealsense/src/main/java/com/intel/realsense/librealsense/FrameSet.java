package com.intel.realsense.librealsense;

public class FrameSet extends LrsClass {
    private int mSize = 0;
    public FrameSet(long handle) {
        mHandle = handle;
        int error = 0;
//        nAddRef(mHandle, error);
        mSize = nFrameCount(mHandle, error);
    }

    public Frame first(StreamType type){
        return first(type, StreamFormat.ANY);
    }

    public Frame first(StreamType type, StreamFormat format){
        int error = 0;
        for(int i = 0; i < mSize; i++) {
            Frame f = new Frame(nExtractFrame(mHandle, i, error));
            if(f.getProfile().getType() == type && (f.getProfile().getFormat() == format || format == StreamFormat.ANY))
                return f;
            try {
                f.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    public int getSize(){ return mSize; }

    @Override
    public void close() throws Exception {
        nRelease(mHandle);
    }

    private native void nAddRef(long handle, int error);
    private native void nRelease(long handle);
    private native long nExtractFrame(long handle, int index, int error);
    private native int nFrameCount(long handle, int error);
}

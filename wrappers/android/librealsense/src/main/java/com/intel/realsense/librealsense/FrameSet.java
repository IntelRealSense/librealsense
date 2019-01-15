package com.intel.realsense.librealsense;

public class FrameSet extends LrsClass {
    private int mSize = 0;
    public FrameSet(long handle) {
        mHandle = handle;
        mSize = nFrameCount(mHandle);
    }

    public Frame first(StreamType type) throws Exception {
        return first(type, StreamFormat.ANY);
    }

    public Frame first(StreamType type, StreamFormat format) throws Exception {
        for(int i = 0; i < mSize; i++) {
            Frame f = Frame.create(nExtractFrame(mHandle, i));
            try(StreamProfile p = f.getProfile()){
                if(p.getType() == type && (p.getFormat() == format || format == StreamFormat.ANY))
                    return f;
            }
        }
        return null;
    }

    public int getSize(){ return mSize; }

    @Override
    public void close() throws Exception {
        nRelease(mHandle);
    }

    private native void nAddRef(long handle);
    private native void nRelease(long handle);
    private native long nExtractFrame(long handle, int index);
    private native int nFrameCount(long handle);
}

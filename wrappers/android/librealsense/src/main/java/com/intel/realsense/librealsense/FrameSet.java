package com.intel.realsense.librealsense;

public class FrameSet extends LrsClass {
    private int mSize = 0;

    public interface FrameCallback{
        void onFrame(Frame f) throws Exception;
    }

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
            f.close();
        }
        return null;
    }

    public void foreach(FrameCallback callback) throws Exception {
        for(int i = 0; i < mSize; i++) {
            try(Frame f = Frame.create(nExtractFrame(mHandle, i))){
                callback.onFrame(f);
            }
        }
    }

    public int getSize(){ return mSize; }

    public FrameSet applyFilter(FilterInterface filter) {
        return filter.process(this);
    }

    @Override
    public void close() throws Exception {
        nRelease(mHandle);
    }

    private static native void nAddRef(long handle);
    private static native void nRelease(long handle);
    private static native long nExtractFrame(long handle, int index);
    private static native int nFrameCount(long handle);
}

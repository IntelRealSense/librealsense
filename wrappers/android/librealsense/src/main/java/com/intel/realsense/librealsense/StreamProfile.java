package com.intel.realsense.librealsense;

public class StreamProfile extends LrsClass {
    private StreamType mType;
    private StreamFormat mFormat;

    private ProfileParams mPp;

    private class ProfileParams {
        public int type;
        public int format;
        public int index;
        public int uniqueId;
        public int frameRate;
    }

    static StreamProfile create(long handle){
        if (nIsProfileExtendableTo(handle, Extension.VIDEO_PROFILE.value()))
            return new VideoStreamProfile(handle);
        return new StreamProfile(handle);
    }

    protected StreamProfile(long handle){
        mHandle = handle;
        mPp = new ProfileParams();
        nGetProfile(mHandle, mPp);
        mType = StreamType.values()[mPp.type];
        mFormat = StreamFormat.values()[mPp.format];
    }

    public StreamType getType() {
        return mType;
    }

    public StreamFormat getFormat() {
        return mFormat;
    }

    public int getIndex() {
        return mPp.index;
    }

    public int getUniqueId() {
        return mPp.uniqueId;
    }

    public int getFrameRate() {
        return mPp.frameRate;
    }

    public <T extends StreamProfile> T as(Class<T> type) {
        return (T) this;
    }

    @Override
    public void close() {
//        nDelete(mHandle);
    }

    private static native boolean nIsProfileExtendableTo(long handle, int extension);
    private static native void nGetProfile(long handle, ProfileParams params);
    private static native void nDelete(long handle);
}

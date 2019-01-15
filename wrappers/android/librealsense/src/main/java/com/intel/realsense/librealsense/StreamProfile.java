package com.intel.realsense.librealsense;

public class StreamProfile extends LrsClass {
    private StreamType mType;
    private StreamFormat mFormat;

    private ProfileParams mPp;

    public class ProfileParams {
        public int type;
        public int format;
        public int index;
        public int uniqueId;
        public int frameRate;
        public int error;
    }

    StreamProfile(long handle){
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

    public VideoStreamProfile asVideoProfile() {
        return new VideoStreamProfile(mHandle);
    }

    @Override
    public void close() throws Exception {

    }

    protected native void nGetProfile(long handle, ProfileParams params);
}
